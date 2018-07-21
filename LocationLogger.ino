//#define SERIAL_DEBUG
//#define WEMOS_D1_MINI
#define ARDUINO_PRO_MINI

#include <SdFat.h>
#include <Task.h>

#include "TaskStatusLed.h"
#include "TaskGps.h"
#include "TaskButton.h"

#ifdef WEMOS_D1_MINI
  #include "ESP8266WiFi.h"
#endif

#ifdef WEMOS_D1_MINI
  #define SAFE_EJECT_BUTTON_PIN D3
#endif
#ifdef ARDUINO_PRO_MINI
  #define SAFE_EJECT_BUTTON_PIN 2
#endif

#ifdef WEMOS_D1_MINI
  #define SD_CHIP_SELECT D8 // D8
#endif
#ifdef ARDUINO_PRO_MINI
  #define SD_CHIP_SELECT 10
#endif

// foreward declare functions passed to task constructors
void OnGpsReadingComplete(const GpsReading *readings, uint8_t count);
void OnGpsFixChanged(GPSFIXTYPE gpsFixType);
void HandleSafeEjectButtonChange(ButtonState state);

TaskManager taskManager;

TaskStatusLed taskStatusLed;
TaskGps taskGps(OnGpsReadingComplete, OnGpsFixChanged);
TaskButton AButtonTask(HandleSafeEjectButtonChange, SAFE_EJECT_BUTTON_PIN);

SdFat sd;
SdFile logFile;

void setup()
{
  #ifdef SERIAL_DEBUG
    Serial.begin(115200);
    while (!Serial) {}
    Serial.println(F("Starting..."));
  #endif

  #ifdef WEMOS_D1_MINI
    #ifdef SERIAL_DEBUG
      Serial.println(F("Turning off Wi-Fi..."));
    #endif

    WiFi.mode(WIFI_OFF);
    delay(1);

    #ifdef SERIAL_DEBUG
      Serial.println(F("Wi-Fi off."));
    #endif
  #endif

  taskManager.StartTask(&taskStatusLed);

  taskStatusLed.ShowPowerUp();

  #ifdef SERIAL_DEBUG
    Serial.println(F("Starting SD..."));
  #endif

  while (!sd.begin(SD_CHIP_SELECT, SPI_HALF_SPEED)) {};

  #ifdef SERIAL_DEBUG
    Serial.println(F("SD started."));
  #endif

  taskStatusLed.Stop();
  taskManager.StartTask(&AButtonTask);
  taskManager.StartTask(&taskGps);
}

void loop()
{
  taskManager.Loop();
}

void HandleSafeEjectButtonChange(ButtonState state)
{
  // on release only
  if (state == ButtonState_Released)
  {
    #ifdef SERIAL_DEBUG
      Serial.println(F("Button released."));
    #endif

    if (taskManager.StatusTask(&taskGps) == TaskState_Stopped)
    {
      taskManager.StartTask(&taskGps);
      taskStatusLed.ShowStartRecording();
    }
    else if (taskManager.StatusTask(&taskGps) == TaskState_Running)
    {
      taskManager.StopTask(&taskGps);
      taskStatusLed.ShowSafeToEject();
    }
  }
}

void OnGpsFixChanged(GPSFIXTYPE gpsFixType)
{
  #ifdef SERIAL_DEBUG
    Serial.print(F("GPS fix changing to "));
    Serial.println(gpsFixType);
  #endif

  switch (gpsFixType)
  {
    case GPSFIXTYPE_NOFIX:
      taskStatusLed.ShowNoFix();
      break;
    case GPSFIXTYPE_2DFIX:
    case GPSFIXTYPE_3DFIX:
      taskStatusLed.ShowFix();
      break;
  }
}

void OnGpsReadingComplete(const GpsReading *readings, uint8_t readingCount)
{
  #ifdef SERIAL_DEBUG
    Serial.println(F("Writing readings..."));
  #endif

  char lastHourWritten[2];
  lastHourWritten[0] = 'x';
  lastHourWritten[1] = 'x';

  for (int i = 0; i < readingCount; i++)
  {
    if (readings[i].time[0] != lastHourWritten[0] || readings[i].time[1] != lastHourWritten[1])
    {
      SwitchFiles(readings[i].date, readings[i].time);
      lastHourWritten[0] = readings[i].time[0];
      lastHourWritten[1] = readings[i].time[1];
    }

    #ifdef SERIAL_DEBUG
        Serial.print(F("Writing reading "));
        Serial.println(i);
    #endif

    logFile.print(readings[i].date);
    logFile.print(',');
    logFile.print(readings[i].time);
    logFile.print(',');
    logFile.print(readings[i].latitude);
    logFile.print(',');
    logFile.print(readings[i].latitudeDirection);
    logFile.print(',');
    logFile.print(readings[i].longitude);
    logFile.print(',');
    logFile.print(readings[i].longitudeDirection);
    logFile.print(',');
    logFile.print(readings[i].altitude);
    logFile.print(',');
    logFile.println(readings[i].satelliteCount);
  }

  logFile.close();

  if (taskManager.StatusTask(&taskGps) != TaskState_Running)
  {
    taskStatusLed.ShowSafeToEject();
    #ifdef SERIAL_DEBUG
      Serial.println(F("Safe to eject."));
    #endif
  }
  else
  {
    taskStatusLed.ShowFileWritten();
  }
}

bool SwitchFiles(const char *date, const char *time)
{
  char fileName[] = "000000-0.CSV";

  #ifdef SERIAL_DEBUG
    Serial.println(F("Changing files..."));
  #endif

  if (logFile.isOpen())
  {
    logFile.close();
  }

  EncodeFileName(fileName, date, time);

  #ifdef SERIAL_DEBUG
    Serial.print(F("File name: "));
    Serial.println(fileName);
  #endif

  if (!logFile.open(fileName, O_WRITE | O_CREAT | O_AT_END))
  {
    // blink red three times to indicate SD problem

    taskStatusLed.ShowFileOpenError();

    #ifdef SERIAL_DEBUG
        Serial.println(F("Error opening file."));
    #endif

    return false;
  }

  return true;
}

void EncodeFileName(char *fileName, const char *date, const char *time)
{
  fileName[0] = date[4];
  fileName[1] = date[5];
  fileName[2] = date[2];
  fileName[3] = date[3];
  fileName[4] = date[0];
  fileName[5] = date[1];
  fileName[7] = ((time[0] - '0') * 10 + time[1] - '0') + 'A';
}
