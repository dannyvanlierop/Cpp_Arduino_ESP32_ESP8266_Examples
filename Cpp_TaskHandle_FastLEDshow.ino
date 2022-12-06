/**
 * @author Danny van Lierop
 * @description Cpp_TaskHandle_FastLEDshow.ino
 * @license MIT
 */

#include "FastLED.h"

/**
    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX: 
                                            GLOBAL:DEFINE:
    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX:
**/

#define LED_DATA_PIN            18
#define LED_CLOCK_PIN   	    19
#define LED_TYPE                LPD8806
#define LED_RGB_ORDER           BRG
#define LED_TOTAL               590
#define BRIGHTNESS              64
#define FRAMES_PER_SECOND       120

/**
    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX: 
                                            GLOBAL:VAR:
    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX:
**/

CRGB leds[LED_TOTAL];

void confetti() { fadeToBlackBy( leds, LED_TOTAL, 10);  int pos = random16(LED_TOTAL);  leds[pos] += CHSV( random8(64) + random8(64), 200, 255);}
void sinelon(){ fadeToBlackBy( leds, LED_TOTAL, 20);  int pos = beatsin16(13,0,LED_TOTAL);  leds[pos] += CHSV( random8(64), 255, 192);}

/**
    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX: 
                                            FUNCTIONHANDLE:
    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX:
**/
//All functions need to loaded before this class.
void (*pFunction[])()={confetti,sinelon}; // List of function 
class cFunctionHandler
{
private:
    #define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))
public:
         uint8_t iCurrentFunction = 0; // Index number of which pattern is current
          String sFunctionName[ARRAY_SIZE(pFunction)]; // List of function names

        void nextFunction()
        {
            iCurrentFunction = (iCurrentFunction + 1) % ARRAY_SIZE(pFunction);
            Serial.print("\nnextFunction:"+String(iCurrentFunction));
        };
        void loop()
        {
            pFunction[iCurrentFunction](); // Call the current function once
            EVERY_N_SECONDS( 3 ){ nextFunction(); // change function periodically (every NEXT_FUNCTION_SECOND)
        };
    };
}FunctionHandler;

/**
    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX: 
                                            TASKHANDLE:
    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX:
**/
// -- Task handles for use in the notifications
static TaskHandle_t taskFastLEDshow = 0;
static TaskHandle_t taskUser = 0;
/** TASKHANDLE:SETUP:
 * @xPortGetCoreID          - Main code running on coreID
 * @FASTLED_SHOW_CORE       - The core to run FastLED.show()
 * @xTaskCreatePinnedToCore - Create the FastLED show task
********************************************************************************************/
void TaskHandle_Setup()
{
    // -- The core to run FastLED.show()
    #define FASTLED_SHOW_CORE 0
    // -- Create the FastLED show task
    int core = xPortGetCoreID();
    Serial.print("Main code running on core ");
    Serial.println(core);
    xTaskCreatePinnedToCore(TaskHandle_FastLEDshow_Emitter, "TaskHandle_FastLEDshow_Emitter", 2048, NULL, 2, &taskFastLEDshow, FASTLED_SHOW_CORE);
};
/** show() for ESP32
 *  Call this function instead of FastLED.show(). It signals core 0 to issue a show, then waits for a notification that it is done.
 */
void TaskHandle_FastLEDshow_Trigger()
{
    if (taskUser == 0){//Only run 1 task a time
        taskUser = xTaskGetCurrentTaskHandle();                 // Store current task handle, so that the show task can notify it when it's done
        xTaskNotifyGive(taskFastLEDshow);                       // Trigger the show task emitter to start FastLED.show() procedure
        const TickType_t xMaxBlockTime = pdMS_TO_TICKS( 200 );  // Wait (limited) for emitter,
        ulTaskNotifyTake(pdTRUE, xMaxBlockTime);                // to callback its done with the show task
        taskUser = 0;                                           // Remove current task.
    };
};
/** show Task
 *  This function runs on core 0 and just waits for requests to call FastLED.show()
 */
void TaskHandle_FastLEDshow_Emitter(void *pvParameters)
{
    // Run forever...
    for(;;) {
        // -- Wait for the trigger
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        // -- Do the show (synchronously), send the 'leds' array out to the actual LED strip
        #if defined (FRAMES_PER_SECOND)
            FastLED.delay(1000/FRAMES_PER_SECOND); //FastLED.delay will utterly call FastLED.show() at least once.
        #elif
            FastLED.show();
        #endif
        // -- Notify the calling task
        xTaskNotifyGive(taskUser);
    };
};

/**
    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX: 
                                             SETUP:
    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX:
**/
void setup() {
    set_max_power_in_volts_and_milliamps(5, 500);
    Serial.begin(115200);
    delay(1000); // 1 second delay for recovery
    FastLED.addLeds<LED_TYPE, LED_DATA_PIN, LED_CLOCK_PIN, LED_RGB_ORDER>(leds, LED_TOTAL).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(BRIGHTNESS);
    TaskHandle_Setup();
};
/**
    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX: 
    XXX:                                      LOOP:
    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX:
**/
void loop()
{
    FunctionHandler.loop();
    TaskHandle_FastLEDshow_Trigger();
};
