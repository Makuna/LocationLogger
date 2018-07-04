
#include <SoftwareSerial.h>

#define READINGS_SIZE 10

#define NMEA_MESSAGE_BUFFER_SIZE 13
#define NMEA_MESSAGE_READ_PIN D4
#define NMEA_MESSAGE_WRITE_PIN D5

enum NMEA_SENTENCE
{
    NMEA_Unknown = -1,
    NMEA_GPRMC,
    NMEA_GPVTG,
    NMEA_GPGGA,
    NMEA_GPGSA,
    NMEA_GPGSV,
    NMEA_GPGLL,
    NMEA_GNGLL,
    NMEA_BDGSA,
    NMEA_BDGSV,
    NMEA_GNRMC
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

typedef void(*GpsReadingComplete)(const GpsReading* readings, uint8_t count);

class TaskGps : public Task
{
public:
    TaskGps(GpsReadingComplete function) : // pass any custom arguments you need
        Task(10), // check every 10 ms
        callback(function),
        activeReadingIndex(0),
        bufferIndex(0),
        segment(-1),
        sentence(NMEA_Unknown),
        gps(NMEA_MESSAGE_READ_PIN, NMEA_MESSAGE_WRITE_PIN)
    { 
    };



private:
    // put member variables here that are scoped to this object
    GpsReadingComplete callback;
    SoftwareSerial gps;
   
    GpsReading readings[READINGS_SIZE];
    uint8_t activeReadingIndex;

    char segmentBuffer[NMEA_MESSAGE_BUFFER_SIZE];
    int8_t bufferIndex;
    int8_t segment;
    NMEA_SENTENCE sentence;

    virtual bool OnStart() // optional
    {
        // put code here that will be run when the task starts
        gps.begin(9600);

        // init state 
        activeReadingIndex = 0;
        bufferIndex = 0;
        segment = -1;
        sentence = NMEA_Unknown;

        return true;
    }

    virtual void OnStop() // optional
    {
        // put code here that will be run when the task stops
        gps.end();

        // if we have any readings when asked to stop
        if (activeReadingIndex)
        {
            // inform callback on number of readings we have
            callback(readings, activeReadingIndex);
        }
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
                        activeReadingIndex++;
                        if (activeReadingIndex == READINGS_SIZE)
                        {
                            activeReadingIndex = 0;
                            callback(readings, READINGS_SIZE);
                        }
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
        #ifdef SERIAL_DEBUG
            Serial.print("Sentence start: ");
            for (int index = 0; index < 5; index++)
            {
                Serial.print(segmentBuffer[index]);
            }
            Serial.println();
        #endif

        if (segmentBuffer[0] == 'G' && (segmentBuffer[1] == 'P' || segmentBuffer[1] == 'N'))
        {
            if (segmentBuffer[2] == 'R')
            {
                // $GPRMC: Time, date, position, course and speed data
                sentence = NMEA_GPRMC;
                #ifdef SERIAL_DEBUG
                    Serial.println("Interpreting NMEA_GPRMC");
                #endif

                return true;  // start of a new reading, for now we trigger on this
// $REVIEW - need to trigger after the last sentence we require has been fully read
            }
            else if (segmentBuffer[2] == 'G')
            {
                if (segmentBuffer[3] == 'G')
                {
                    // $GPGGA: Time, position, and fix related data of the receiver.
                    sentence = NMEA_GPGGA;
                    #ifdef SERIAL_DEBUG
                        Serial.println("Interpreting NMEA_GPGGA");
                    #endif
                }
                else if (segmentBuffer[3] == 'S')
                {
                    if (segmentBuffer[4] == 'A')
                    {
                        // $GPGSA: ID�s of satellites which are used for position fix.
                        sentence = NMEA_GPGSA;
                        #ifdef SERIAL_DEBUG
                            Serial.println("Interpreting NMEA_GPGSA");
                        #endif
                    }
                    else if (segmentBuffer[4] == 'V')
                    {
                        // $GPGSV: Satellite information about elevation, azimuth and CNR.
                        sentence = NMEA_GPGSV;
                        #ifdef SERIAL_DEBUG
                            Serial.println("Interpreting NMEA_GPGSV");
                        #endif
                    }
                }
                else if (segmentBuffer[3] == 'L')
                {
                    // $GPGLL: Position, time and fix status.
                    sentence = NMEA_GPGLL;
                    #ifdef SERIAL_DEBUG
                        Serial.println("Interpreting NMEA_GPGLL");
                    #endif
                }
            }
            else if (segmentBuffer[2] == 'V')
            {
                // $GPVTG: Course and speed relative to the ground.
                sentence = NMEA_GPVTG;
                #ifdef SERIAL_DEBUG
                    Serial.println("Interpreting NMEA_GPVTG");
                #endif
            }
        }

        return false;
    }

    void ProcessSegmentBuffer()
    {
        #ifdef SERIAL_DEBUG
            Serial.print("Sentence ");
            Serial.print(sentence);
            Serial.print(" segment ");
            Serial.print(segment);
            Serial.print(": ");
            Serial.println(segmentBuffer);
        #endif

        switch (sentence) 
        {
            case NMEA_GPRMC: 
                // $GPRMC,074318.00,A,4735.41382,N,12212.35088,W,0.030,,170617,,,A*63
                //        ^^^^^^^^^   ^^^^^^^^^^ ^ ^^^^^^^^^^^ ^        ^^^^^^
                //        time        latitude   d longitude   d        date
                switch (segment) 
                {
                    case 1: // time
                        strcpy(readings[activeReadingIndex].time, segmentBuffer);
                        #ifdef SERIAL_DEBUG
                            Serial.print("Time = ");
                            Serial.println(segmentBuffer);
                        #endif
                        break;
                    case 2: // fix quality
                        break;
                    case 3: // latitude
                        strcpy(readings[activeReadingIndex].latitude, segmentBuffer);
                        #ifdef SERIAL_DEBUG
                            Serial.print("Latitude = ");
                            Serial.println(segmentBuffer);
                        #endif
                        break;
                    case 4: // latitude direction
                        strcpy(readings[activeReadingIndex].latitudeDirection, segmentBuffer);
                        #ifdef SERIAL_DEBUG
                            Serial.print("Latitude direction = ");
                            Serial.println(segmentBuffer);
                        #endif
                        break;
                    case 5: // longitude
                        strcpy(readings[activeReadingIndex].longitude, segmentBuffer);
                        #ifdef SERIAL_DEBUG
                            Serial.print("Longitude = ");
                            Serial.println(segmentBuffer);
                        #endif
                        break;
                    case 6: // longitude direction
                        strcpy(readings[activeReadingIndex].longitudeDirection, segmentBuffer);
                        #ifdef SERIAL_DEBUG
                            Serial.print("Longitude direction = ");
                            Serial.println(segmentBuffer);
                        #endif
                        break;
                    case 7: // ?
                    case 8: // ?
                        break;
                    case 9: // date
                        strcpy(readings[activeReadingIndex].date, segmentBuffer);
                        #ifdef SERIAL_DEBUG
                            Serial.print("Date = ");
                            Serial.println(segmentBuffer);
                        #endif
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
                strcpy(readings[activeReadingIndex].satelliteCount, segmentBuffer);
                // Serial.print("Satellites = ");
                // Serial.println(segmentBuffer);
                break;
            case 8: // ?
                break;
            case 9: // altitude
                strcpy(readings[activeReadingIndex].altitude, segmentBuffer);
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
