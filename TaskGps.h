
#include <SoftwareSerial.h>

#define READINGS_SIZE 10
#define NMEA_MESSAGE_BUFFER_SIZE 13

#ifdef WEMOS_D1_MINI
    #define NMEA_MESSAGE_READ_PIN D4
    #define NMEA_MESSAGE_WRITE_PIN D3
#endif
#ifdef ARDUINO_PRO_MINI
    #define NMEA_MESSAGE_READ_PIN 7
    #define NMEA_MESSAGE_WRITE_PIN 8
#endif

enum GPSFIXTYPE
{
    GPSFIXTYPE_NOFIX = 1,
    GPSFIXTYPE_2DFIX,
    GPSFIXTYPE_3DFIX
};

enum NMEA_SENTENCE
{
    NMEA_SENTENCE_Unknown = -1,
    NMEA_SENTENCE_GPRMC,
    NMEA_SENTENCE_GPVTG,
    NMEA_SENTENCE_GPGGA,
    NMEA_SENTENCE_GPGSA,
    NMEA_SENTENCE_GPGSV,
    NMEA_SENTENCE_GPGLL,
    NMEA_SENTENCE_GNGLL,
    NMEA_SENTENCE_BDGSA,
    NMEA_SENTENCE_BDGSV,
    NMEA_SENTENCE_GNRMC,
    NMEA_SENTENCE_GNGGA,
    NMEA_SENTENCE_GNVTG,
    NMEA_SENTENCE_GNGSV,
    NMEA_SENTENCE_GNGSA
};


struct GpsReading 
{
    char date[7];
    char time[11];
    char latitude[16];
    char latitudeDirection[2];
    char longitude[17];
    char longitudeDirection[2];
    char altitude[8];
    char satelliteCount[3];
};

typedef void(*GpsReadingComplete)(const GpsReading* readings, uint8_t count);
typedef void(*GpsFixChanged)(GPSFIXTYPE gpsFixType);

class TaskGps : public Task
{
public:
    TaskGps(GpsReadingComplete gpsReadingCompleteFunction, GpsFixChanged gpsFixChangedCallbackFunction) : // pass any custom arguments you need
        Task(MsToTaskTime(2)), // check every 2 ms
        gpsReadingCompleteCallback(gpsReadingCompleteFunction),
        gpsFixChangedCallback(gpsFixChangedCallbackFunction),
        gps(NMEA_MESSAGE_READ_PIN, NMEA_MESSAGE_WRITE_PIN),
        activeReadingIndex(0),
        bufferIndex(0),
        segment(-1),
        sentence(NMEA_SENTENCE_Unknown),
        gpsFixType(GPSFIXTYPE_NOFIX)
    { 
        memset(readings, 0, sizeof(readings));

        gps.begin(9600);
    };



private:
    // put member variables here that are scoped to this object
    GpsReadingComplete gpsReadingCompleteCallback;
    GpsFixChanged gpsFixChangedCallback;

    SoftwareSerial gps;
   
    GpsReading readings[READINGS_SIZE];
    uint8_t activeReadingIndex;

    char segmentBuffer[NMEA_MESSAGE_BUFFER_SIZE];
    int8_t bufferIndex;
    int8_t segment;
    NMEA_SENTENCE sentence;
    GPSFIXTYPE gpsFixType;

    virtual bool OnStart() // optional
    {
        #ifdef SIMPLE_DEBUG
            Serial.println(F("GPS go"));
        #endif

        gps.listen();

        #ifdef SERIAL_DEBUG
            Serial.println(F("GPS task started."));
        #endif

        // init state 
        activeReadingIndex = 0;
        bufferIndex = 0;
        segment = -1;
        sentence = NMEA_SENTENCE_Unknown;
        gpsFixType = GPSFIXTYPE_NOFIX;

        return true;
    }

    virtual void OnStop() // optional
    {
        #ifdef SIMPLE_DEBUG
            Serial.println(F("GPS stop"));
        #endif

        gps.stopListening();

        #ifdef SERIAL_DEBUG
            Serial.println(F("GPS task stopped."));
        #endif
        
        // if we have any readings when asked to stop
        if (activeReadingIndex)
        {
            // inform gpsReadingCompleteCallback on number of readings we have
            gpsReadingCompleteCallback(readings, activeReadingIndex);
            memset(readings, 0, sizeof(readings));
        }
    }

    virtual void OnUpdate(uint32_t deltaTime)
    {
        while (gps.available()) 
        {
            char lastChar = gps.read();

            if (lastChar == ',' || lastChar == '*') 
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
                            gpsReadingCompleteCallback(readings, READINGS_SIZE);
                            memset(readings, 0, sizeof(readings));
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
                sentence = NMEA_SENTENCE_Unknown;
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
            Serial.print(F("Sentence start: "));
            for (int index = 0; index < 5; index++)
            {
                Serial.print(segmentBuffer[index]);
            }
            Serial.println();
        #endif

        if (segmentBuffer[0] == 'B')
        {
            // BD sentence
            if (segmentBuffer[1] == 'N')
            {

            }
        }
        else if (segmentBuffer[0] == 'G')
        {
            // GPS sentence
            if (segmentBuffer[1] == 'P')
            {
                if (segmentBuffer[2] == 'R')
                {
                    // $GPRMC: Time, date, position, course and speed data
                    sentence = NMEA_SENTENCE_GPRMC;
                    #ifdef SERIAL_DEBUG
                        Serial.println(F("Interpreting NMEA_SENTENCE_GPRMC"));
                    #endif
                }
                else if (segmentBuffer[2] == 'G')
                {
                    if (segmentBuffer[3] == 'G')
                    {
                        // $GPGGA: Time, position, and fix related data of the receiver.
                        sentence = NMEA_SENTENCE_GPGGA;
                        #ifdef SERIAL_DEBUG
                            Serial.println(F("Interpreting NMEAIAL_DEBUG_GPGGA"));
                        #endif
                    }
                    else if (segmentBuffer[3] == 'S')
                    {
                        if (segmentBuffer[4] == 'A')
                        {
                            // $GPGSA: IDs of satellites which are used for position fix.
                            sentence = NMEA_SENTENCE_GPGSA;
                            #ifdef SERIAL_DEBUG
                                Serial.println(F("Interpreting NMEA_SENTENCE_GPGSA"));
                            #endif
                        }
                        else if (segmentBuffer[4] == 'V')
                        {
                            // $GPGSV: Satellite information about elevation, azimuth and CNR.
                            sentence = NMEA_SENTENCE_GPGSV;
                            #ifdef SERIAL_DEBUG
                                Serial.println(F("Interpreting NMEA_SENTENCE_GPGSV"));
                            #endif
                        }
                    }
                    else if (segmentBuffer[3] == 'L')
                    {
                        // $GPGLL: Position, time and fix status.
                        sentence = NMEA_SENTENCE_GPGLL;
                        #ifdef SERIAL_DEBUG
                            Serial.println(F("Interpreting NMEA_SENTENCE_GPGLL"));
                        #endif
                    }
                }
                else if (segmentBuffer[2] == 'V')
                {
                    // $GPVTG: Course and speed relative to the ground.
                    sentence = NMEA_SENTENCE_GPVTG;
                    #ifdef SERIAL_DEBUG
                        Serial.println(F("Interpreting NMEAIAL_DEBUG_GPVTG"));
                    #endif
                }
            }
            else if (segmentBuffer[1] == 'N')
            {
                if (segmentBuffer[2] == 'R')
                {
                    // $GPRMC: Time, date, position, course and speed data
                    sentence = NMEA_SENTENCE_GNRMC;
                    #ifdef SERIAL_DEBUG
                        Serial.println(F("Interpreting NMEA_SENTENCE_GPRMC"));
                    #endif
                }
                else if (segmentBuffer[2] == 'G')
                {
                    if (segmentBuffer[3] == 'G')
                    {
                        // $GPGGA: Time, position, and fix related data of the receiver.
                        sentence = NMEA_SENTENCE_GNGGA;
                        #ifdef SERIAL_DEBUG
                            Serial.println(F("Interpreting NMEAIAL_DEBUG_GPGGA"));
                        #endif
                        return true;  // start of a new reading, for now we trigger on this
// $REVIEW - need to trigger after the last sentence we require has been fully read

                    }
                    else if (segmentBuffer[3] == 'S')
                    {
                        if (segmentBuffer[4] == 'A')
                        {
                            // $GPGSA: IDs of satellites which are used for position fix.
                            sentence = NMEA_SENTENCE_GNGSA;
                            #ifdef SERIAL_DEBUG
                                Serial.println(F("Interpreting NMEA_SENTENCE_GPGSA"));
                            #endif
                        }
                        else if (segmentBuffer[4] == 'V')
                        {
                            // $GPGSV: Satellite information about elevation, azimuth and CNR.
                            sentence = NMEA_SENTENCE_GNGSV;
                            #ifdef SERIAL_DEBUG
                                Serial.println(F("Interpreting NMEA_SENTENCE_GPGSV"));
                            #endif
                        }
                    }
                    else if (segmentBuffer[3] == 'L')
                    {
                        // $GPGLL: Position, time and fix status.
                        sentence = NMEA_SENTENCE_GNGLL;
                        #ifdef SERIAL_DEBUG
                            Serial.println(F("Interpreting NMEA_SENTENCE_GPGLL"));
                        #endif
                    }
                }
                else if (segmentBuffer[2] == 'V')
                {
                    // $GPVTG: Course and speed relative to the ground.
                    sentence = NMEA_SENTENCE_GNVTG;
                    #ifdef SERIAL_DEBUG
                        Serial.println(F("Interpreting NMEAIAL_DEBUG_GPVTG"));
                    #endif
                }
            }
        }

        return false;
    }

    void ProcessSegmentBuffer()
    {
        #ifdef SERIAL_DEBUG
            Serial.print(F("Sentence "));
            Serial.print(sentence);
            Serial.print(F(" segment "));
            Serial.print(segment);
            Serial.print(F(": "));
            Serial.println(segmentBuffer);
        #endif

        switch (sentence) 
        {
            case NMEA_SENTENCE_GNRMC:
            case NMEA_SENTENCE_GPRMC: 
                // $GPRMC,074318.000,A,4735.41382,N,12212.35088,W,0.030,,170617,,,A*63
                //        ^^^^^^^^^^   ^^^^^^^^^^ ^ ^^^^^^^^^^^ ^        ^^^^^^
                //        time        latitude   d longitude   d        date
                switch (segment) 
                {
                    case 1: // time
                        strncpy(readings[activeReadingIndex].time, segmentBuffer, 6);
                        #ifdef SERIAL_DEBUG
                            Serial.print(F("Time = "));
                            Serial.println(segmentBuffer);
                        #endif
                        break;
                    case 2: // fix quality
                        break;
                    case 3: // latitude
                        strcpy(readings[activeReadingIndex].latitude, segmentBuffer);
                        #ifdef SERIAL_DEBUG
                            Serial.print(F("Latitude = "));
                            Serial.println(segmentBuffer);
                        #endif
                        break;
                    case 4: // latitude direction
                        strcpy(readings[activeReadingIndex].latitudeDirection, segmentBuffer);
                        #ifdef SERIAL_DEBUG
                            Serial.print(F("Latitude direction = "));
                            Serial.println(segmentBuffer);
                        #endif
                        break;
                    case 5: // longitude
                        strcpy(readings[activeReadingIndex].longitude, segmentBuffer);
                        #ifdef SERIAL_DEBUG
                            Serial.print(F("Longitude = "));
                            Serial.println(segmentBuffer);
                        #endif
                        break;
                    case 6: // longitude direction
                        strcpy(readings[activeReadingIndex].longitudeDirection, segmentBuffer);
                        #ifdef SERIAL_DEBUG
                            Serial.print(F("Longitude direction = "));
                            Serial.println(segmentBuffer);
                        #endif
                        break;
                    case 7: // ?
                    case 8: // ?
                        break;
                    case 9: // date
                        strcpy(readings[activeReadingIndex].date, segmentBuffer);
                        #ifdef SERIAL_DEBUG
                            Serial.print(F("Date = "));
                            Serial.println(segmentBuffer);
                        #endif
                        break;

                    default:
                        break;
                }

            break;

        case NMEA_SENTENCE_GNVTG:
            break;

        case NMEA_SENTENCE_GNGGA:
        case NMEA_SENTENCE_GPGGA:
            switch (segment) 
            {
                case 1: // time
                case 5: // longitude direction
                case 6: // ?
                    break;
                case 7: // number of satellites
                    strcpy(readings[activeReadingIndex].satelliteCount, segmentBuffer);
                    #ifdef SERIAL_DEBUG
                        Serial.print(F("Satellites = "));
                        Serial.println(segmentBuffer);
                    #endif
                    break;
                case 8: // ?
                    break;
                case 9: // altitude
                    strcpy(readings[activeReadingIndex].altitude, segmentBuffer);
                    #ifdef SERIAL_DEBUG
                        Serial.print(F("Altitude = "));
                        Serial.println(segmentBuffer);
                    #endif
                    break;
                case 10:// altitude unit
                case 11: // ?
                case 12: // ?
                    break;

                default:
                    break;
            }
            break;

        case NMEA_SENTENCE_GPGSA:
            switch (segment)
            {
                case 2: // Fix type
                {
                    #ifdef SERIAL_DEBUG
                        Serial.print(F("Fix type = "));
                        Serial.println(segmentBuffer[0]);
                    #endif

                    GPSFIXTYPE newGpsFixType = static_cast<GPSFIXTYPE>(segmentBuffer[0] - '0');

                    if (newGpsFixType != gpsFixType)
                    {
                        gpsFixChangedCallback(newGpsFixType);
                        gpsFixType = newGpsFixType;
                    }
                    break;
                }
            }
            break;

        case NMEA_SENTENCE_GPGSV:
        case NMEA_SENTENCE_GPGLL:
            break;

        default:
            break;
        }
    }
};
