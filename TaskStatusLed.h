
#include <NeoPixelBus.h>

#define NEOPIXEL_COUNT 1
#define NEOPIXEL_BLINK_MS 20

#ifdef WEMOS_D1_MINI
    #define NEOPIXEL_PIN D2
#endif
#ifdef ARDUINO_PRO_MINI
    #define NEOPIXEL_PIN 8
#endif

#define COLOR_SATURATION 128

RgbColor red(COLOR_SATURATION, 0, 0);
RgbColor yellow(COLOR_SATURATION, COLOR_SATURATION, 0);
RgbColor green(0, COLOR_SATURATION, 0);
RgbColor blue(0, 0, COLOR_SATURATION);
RgbColor white(COLOR_SATURATION);
RgbColor black(0);

class TaskStatusLed : public Task
{
public:
    TaskStatusLed() : // pass any custom arguments you need
        Task(NEOPIXEL_BLINK_MS),
        strip(NEOPIXEL_COUNT, NEOPIXEL_PIN) // initialize members here
    { };

    void ShowPowerUp()
    {
        // we want immediate color change as 
        // other initialization could extend how long this stays on
        pattern = 0b0;
        repeat = false;
        patternIndex = 15;
        strip.SetPixelColor(0, white);
        strip.Show();
    }

    void ShowFileWritten()
    {
        pattern = 0b1000000000000000;
        repeat = false;
        patternIndex = 15;
        flashColor = red;
    }

    void ShowFileOpenError()
    {
        pattern = 0b101010000000000;
        repeat = false;
        patternIndex = 15;
        flashColor = red;
    }

    void ShowSafeToEject()
    {
        pattern = 0b1000000000000000;
        repeat = true;
        patternIndex = 15;
        flashColor = green;
    }

    void ShowNoFix()
    {
        pattern = 0b1010100000000000;
        repeat = true;
        patternIndex = 15;
        flashColor = blue;
    }

    void ShowFix()
    {
        pattern = 0b1000100010000000;
        repeat = false;
        patternIndex = 15;
        flashColor = green;
    }

    void ShowStartRecording()
    {
        pattern = 0b100010001000000;
        repeat = false;
        patternIndex = 15;
        flashColor = red;
    }

    void Stop()
    {
        pattern = 0b0;
        repeat = false;
        patternIndex = 0;
        flashColor = black;
    }

private:
    // put member variables here that are scoped to this object
    NeoPixelBus<NeoGrbwFeature, Neo800KbpsMethod> strip;
    RgbColor flashColor;
    int8_t patternIndex;
    uint16_t pattern;
    bool repeat;

    virtual bool OnStart() // optional
    {
        // put code here that will be run when the task starts
        strip.Begin();
        strip.Show();

        pattern = 0;
        patternIndex = -1;
        repeat = false;

        return true;
    }

    virtual void OnUpdate(uint32_t deltaTime)
    {
        if (patternIndex >= 0)
        {
		    bool indexState = ((pattern >> patternIndex) & 0b1) > 0;

            strip.SetPixelColor(0, indexState ? flashColor : black);
            strip.Show();
            
            if (patternIndex > 0 && (pattern << (16 - patternIndex)) == 0) {
                patternIndex = 1;
            }
		
            patternIndex--;

            if (repeat && patternIndex == -1)
            {
                patternIndex = 15;
            }
        }
    }
};
