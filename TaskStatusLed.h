
#include <NeoPixelBus.h>

#define NEOPIXEL_COUNT 1
#define NEOPIXEL_BLINK_MS 62

#ifdef WEMOS_D1_MINI
    #define NEOPIXEL_PIN D2
#endif
#ifdef ARDUINO_PRO_MINI
    #define NEOPIXEL_PIN 4
#endif

#define COLOR_SATURATION 64

RgbColor angryRed(255, 0, 0);
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
        Task(MsToTaskTime(NEOPIXEL_BLINK_MS)),
        strip(NEOPIXEL_COUNT, NEOPIXEL_PIN), // initialize members here
        flashColor(black),
        patternIndex(-1),
        pattern(0),
        repeat(false)
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
        if (!repeat) // do not reset any repeating states
        {
            pattern = 0b1100000000000000;
            repeat = false;
            patternIndex = 15;
            flashColor = red;
        }
    }

    void ShowFileOpenError()
    {
        if (!repeat) // do not reset any repeating states
        {
            pattern = 0b1100011000000000;
            repeat = false;
            patternIndex = 15;
            flashColor = angryRed;
        }
    }

    void ShowSafeToEject()
    {
        pattern = 0b1100000000000000;
        repeat = true;
        patternIndex = 15;
        flashColor = green;
    }

    void ShowNoFix()
    {
        pattern = 0b1100011000110000;
        repeat = true;
        patternIndex = 15;
        flashColor = blue;
    }

    void ShowFix()
    {
        pattern = 0b1100011000110000;
        repeat = false;
        patternIndex = 15;
        flashColor = green;
    }

    void ShowStartRecording()
    {
        pattern = 0b1100110011001100;
        repeat = false;
        patternIndex = 15;
        flashColor = red;
    }

    void StopShowing()
    {
        pattern = 0;
        repeat = false;
        patternIndex = 0;
        flashColor = black;
    }

private:
    // put member variables here that are scoped to this object
    NeoPixelBus<NeoGrbwFeature, NeoWs2813Method> strip;
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
        flashColor = black;

        return true;
    }

    virtual void OnUpdate(uint32_t deltaTime)
    {
        if (patternIndex >= 0)
        {
            RgbColor color;

            if (((pattern >> patternIndex) & 1) > 0)
            {
                color = flashColor;
            }
            else
            {
                color = black;
            }

            strip.SetPixelColor(0, color);
            strip.Show();
            
            patternIndex--;

            if (repeat && patternIndex == -1)
            {
                patternIndex = 15;
            }
        }
    }
};
