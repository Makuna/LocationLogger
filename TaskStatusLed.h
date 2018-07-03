
#include <NeoPixelBus.h>

#define NEOPIXEL_COUNT 1
#define NEOPIXEL_PIN 11
#define NEOPIXEL_BLINK_MS 5

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
        // we want emediate color change as 
        // other initialization could extend how long this stays on
        flashState = 1;
        strip.SetPixelColor(0, white);
        strip.Show();
    }

    void ShowFileWritten()
    {
        flashState = 2; // single flash
        flashColor = red;
    }

    void ShowFileOpenError()
    {
        flashState = 6; // three flashes
        flashColor = red;
    }

    void ShowNoGpsFix()
    {
        flashState = 2; // single flash
        flashColor = blue;
    }

private:
    // put member variables here that are scoped to this object
    NeoPixelBus<NeoGrbwFeature, Neo800KbpsMethod> strip;
    RgbColor flashColor;
    uint8_t flashState;

    virtual bool OnStart() // optional
    {
        // put code here that will be run when the task starts
        strip.Begin();
        strip.Show();

        return true;
    }

    virtual void OnUpdate(uint32_t deltaTime)
    {
        // if there are any flashing going on, manage it
        if (flashState)
        {
            flashState--;
            if ((flashState % 2) == 0)
            {
                // flash off
                strip.SetPixelColor(0, black);
            }
            else
            {
                // flash on
                strip.SetPixelColor(0, flashColor);
            }
            
            strip.Show();
        }
    }
};
