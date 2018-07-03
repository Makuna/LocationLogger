


#include <SdFat.h>
#include <Task.h>

#include "TaskStatusLed.h"  
#include "TaskGps.h"

#define READINGS_SIZE 3

#define LOGGING_SWITCH_PIN 9

#define SD_CHIP_SELECT 4

// #define SERIAL_DEBUG


// foreward delcare functions passed to task constructors
void OnGpsReadingComplete(const GpsReading& reading); 

TaskManager taskManager;

TaskStatusLed taskStatusLed;
TaskGps taskGps(OnGpsReadingComplete);


GpsReading readings[READINGS_SIZE];
uint8_t readingIndex = 0;

char fileName[] = "000000-0.CSV";

SdFat sd;
SdFile logFile;

void setup() {
  taskManager.StartTask(&taskStatusLed);
  
  taskStatusLed.ShowPowerUp();

#ifdef SERIAL_DEBUG
  Serial.begin(9600);
  while (!Serial) {
  }

  Serial.println(F("Starting..."));
#endif

  pinMode(LOGGING_SWITCH_PIN, INPUT); // ??

  sd.begin(SD_CHIP_SELECT, SPI_HALF_SPEED);

#ifdef SERIAL_DEBUG
  Serial.println(F("SD started."));
#endif
  
  taskManager.StartTask(&taskGps);

#ifdef SERIAL_DEBUG
  Serial.println(F("Serial port to GPS started."));
#endif
}

void loop() 
{
  taskManager.Loop();
}

void OnGpsReadingComplete(const GpsReading& reading)
{
	readings[readingIndex] = reading;
	readingIndex++;

	if (readingIndex == READINGS_SIZE)
	{
		readingIndex = 0;
		LogPositions(READINGS_SIZE);
	}
}

void LogPositions(int readingCount) 
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

  taskStatusLed.ShowFileWritten();
}

bool SwitchFiles(char* date, char* time) 
{
#ifdef SERIAL_DEBUG
  Serial.println(F("Changing files..."));
#endif

  if (logFile.isOpen()) 
  {
    logFile.close();
  }

  SetFileName(date, time);

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

void SetFileName(char* date, char* time) 
{
  fileName[0] = readings[0].date[4];
  fileName[1] = readings[0].date[5];
  fileName[2] = readings[0].date[2];
  fileName[3] = readings[0].date[3];
  fileName[4] = readings[0].date[0];
  fileName[5] = readings[0].date[1];
  fileName[7] = ((readings[0].time[0] - '0') * 10 + readings[0].time[1] - '0') + 'A';
}




