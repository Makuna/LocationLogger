
#include <SoftwareSerial.h>

#define NMEA_MESSAGE_BUFFER_SIZE 13
#define NMEA_MESSAGE_READ_PIN 8
#define NMEA_MESSAGW_WRITE_PIN 12

enum NMEA_SENTENCE
{
    NMEA_Unknown = -1,
    NMEA_GPRMC,
    NMEA_GPVTG,
    NMEA_GPGGA,
    NMEA_GPGSA,
    NMEA_GPGSV,
    NMEA_GPGLL,
};


struct GpsReading 
{
    char date[7];
    char time[10];
    char latitude[16];
    char latitudeDirection[2];
    char longitude[17];
    char longitudeDirection[2];
    char altitude[8];
    char satelliteCount[3];
};

typedef void(*GpsReadingComplete)(const GpsReading& reading);

class TaskGps : public Task
{
public:
    TaskGps(GpsReadingComplete function) : // pass any custom arguments you need
        Task(10), // check every 10 ms
        callback(function),
        bufferIndex(0),
        segment(-1),
        sentence(NMEA_Unknown),
        gps(NMEA_MESSAGE_READ_PIN, NMEA_MESSAGW_WRITE_PIN)
    { 
    };

private:
    // put member variables here that are scoped to this object
    GpsReadingComplete callback;
    SoftwareSerial gps;
   
    GpsReading activeReading;

    char segmentBuffer[NMEA_MESSAGE_BUFFER_SIZE];
    int8_t bufferIndex;
    int8_t segment;
    NMEA_SENTENCE sentence;

    virtual bool OnStart() // optional
    {
        // put code here that will be run when the task starts
        gps.begin(9600);

        return true;
    }

    virtual void OnUpdate(uint32_t deltaTime)
    {
        while (gps.available()) 
        {
            char lastChar = gps.read();

            if (lastChar == ',') 
            {
                // end of segment
                if (segment == 0)
                {
                    if (IdentifiedSentence())
                    {
                        callback(activeReading);
                    }
                }
                else
                {
                    segmentBuffer[bufferIndex] = '\0'; // null terminate current segment for str use
                    ProcessSegmentBuffer();
                }

                segment++;
                bufferIndex = 0;
            }
            else if (lastChar == '$') 
            {
                // start of a new sentence
                sentence = NMEA_Unknown;
                segment = 0;
                bufferIndex = 0;
            }
            else 
            {
                // buffer chars for segment decoding
                segmentBuffer[bufferIndex] = lastChar;
                bufferIndex++;
            }
        }
    }

    bool IdentifiedSentence()
    {
        if (segmentBuffer[0] == 'G' && segmentBuffer[1] == 'P')
        {
            if (segmentBuffer[2] == 'R')
            {
                // $GPRMC: Time, date, position, course and speed data
                sentence = NMEA_GPRMC;
                return true;  // start of a new reading, for now we trigger on this
// $REVIEW - need to trigger after the last sentence we require has been fully read
            }
            else if (segmentBuffer[2] == 'G')
            {
                if (segmentBuffer[3] == 'G')
                {
                    // $GPGGA: Time, position, and fix related data of the receiver.
                    sentence = NMEA_GPGGA;
                }
                else if (segmentBuffer[3] == 'S')
                {
                    if (segmentBuffer[4] == 'A')
                    {
                        // $GPGSA: ID�s of satellites which are used for position fix.
                        sentence = NMEA_GPGSA;
                    }
                    else if (segmentBuffer[4] == 'V')
                    {
                        // $GPGSV: Satellite information about elevation, azimuth and CNR.
                        sentence = NMEA_GPGSV;
                    }
                }
                else if (segmentBuffer[3] == 'L')
                {
                    // $GPGLL: Position, time and fix status.
                    sentence = NMEA_GPGLL;
                }
            }
            else if (segmentBuffer[2] == 'V')
            {
                // $GPVTG: Course and speed relative to the ground.
                sentence = NMEA_GPVTG;
            }
        }
        return false;
    }

    void ProcessSegmentBuffer()
    {
        //Serial.println(segmentBuffer);

        switch (sentence) 
        {
        case NMEA_GPRMC: 
            // $GPRMC,074318.00,A,4735.41382,N,12212.35088,W,0.030,,170617,,,A*63
            //        ^^^^^^^^^   ^^^^^^^^^^ ^ ^^^^^^^^^^^ ^        ^^^^^^
            //        time        latitude   d longitude   d        date
            switch (segment) 
            {
            case 1: // time
                strcpy(activeReading.time, segmentBuffer);
                // Serial.print("Time = ");
                // Serial.println(segmentBuffer);
                break;
            case 2: // fix quality
                break;
            case 3: // latitude
                strcpy(activeReading.latitude, segmentBuffer);
                // Serial.print("Latitude = ");
                // Serial.println(segmentBuffer);
                break;
            case 4: // latitude direction
                strcpy(activeReading.latitudeDirection, segmentBuffer);
                // Serial.print("Latitude direction = ");
                // Serial.println(segmentBuffer);
                break;
            case 5: // longitude
                strcpy(activeReading.longitude, segmentBuffer);
                // Serial.print("Longitude = ");
                // Serial.println(segmentBuffer);
                break;
            case 6: // longitude direction
                strcpy(activeReading.longitudeDirection, segmentBuffer);
                // Serial.print("Longitude direction = ");
                // Serial.println(segmentBuffer);
                break;
            case 7: // ?
            case 8: // ?
                break;
            case 9: // date
                strcpy(activeReading.date, segmentBuffer);
                // Serial.print("Date = ");
                // Serial.println(segmentBuffer);
                break;

            default:
                break;
            }

            break;

        case NMEA_GPVTG: // $GPVTG
            break;

        case NMEA_GPGGA: // $GPGGA
            switch (segment) 
            {
            case 1: // time
            case 5: // longitude direction
            case 6: // ?
                break;
            case 7: // number of satellites
                strcpy(activeReading.satelliteCount, segmentBuffer);
                // Serial.print("Satellites = ");
                // Serial.println(segmentBuffer);
                break;
            case 8: // ?
                break;
            case 9: // altitude
                strcpy(activeReading.altitude, segmentBuffer);
                // Serial.print("Altitude = ");
                // Serial.println(segmentBuffer);
                break;
            case 10:// altitude unit
            case 11: // ?
            case 12: // ?
                break;

            default:
                break;
            }
            break;

        case NMEA_GPGSA: // $GPGSA
        case NMEA_GPGSV: // $GPGSV
        case NMEA_GPGLL: // $GPGLL
            break;

        default:
            break;
        }
    }
};
