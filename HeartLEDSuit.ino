#include <FastLED.h>                                          
#include <Wire.h>

/** 
 * Variable Components
 */
#define USE_2ND_STRIP    0
#define USE_SETTINGS     1
#define DEBUG
//#define DEBUG_ANIMATIONS
#include "DebugUtils.h"

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))
/** 
 * LEDS
 */   
// Size of the strip, including both front and back of the strip
#define STRIP_SIZE      100
#define LED40_PIN       10
#define LED60_PIN        6
struct CRGB leds[STRIP_SIZE];  

#if USE_2ND_STRIP
#define STRIP2_SIZE     40
#define LED2_PIN        6
struct CRGB leds2[STRIP2_SIZE];  
#endif

// Number of LEDs for the front side of the suit (will be mirrored on what's left of the strip in the back)
#define NUM_LEDS        100                                   
#define REVERSE_LEDS    0
      
#define DEFAULT_BRIGHTNESS 200                           
#define FRAMES_PER_SECOND  100
                           
/** 
 * Button Switcher
 */ 
#include "Button.h"
#define BUTTON_PIN      12
Button button(BUTTON_PIN, false); 

/** 
 * Animations
 */ 
#include "Animations.h"
//#include "FiboAnimations.h"
// 10 seconds per color palette makes a good demo
// 20-120 is better for deployment
#define SECONDS_PER_PALETTE 10


/* 
 * Settings UI
 */
#if USE_SETTINGS
#include "SettingsMode.h"
#endif

/**
 * Microphone
 */
#define MIC_PIN A4
#include "SoundReactive.h"
  

/**
 * Sequencing
 */
AnimationPattern gAnimations[] = {

  // excellent and warm, maybe should move a bit slower 
  {wave, 0, 0}, 

  // Pastel colors 
  {verticalRainbow, 0, 0}, 

  // Fully colored, subtle changes [use CPT]
  {incrementalDrift, 0, 0},

  // Very good pace [use CPT]
  {radialPaletteShift, 0, 0},

  {pulse, 0, 0}, 

  {life, 0, 0}, 

  {fire, 0, 0}, 

  // not sure about this one yet 
  {multiFire2, 50, 200},
  
  {water, 0, 0}, 

  {blueFire, 100, 200}, 
  
  // [use CPT]
  {colorWaves, 0, 0},
  {colorWaves, 1, 0}, // using Fibonacci, I think this one is the best 

  
  {beatTriggered, 16, 100},

  {soundAnimate, 0, 0},
  {soundAnimate, 1, 0},

  
  {breathing, 16, 64},
  
  {pride,    0,   0}, 
  
  {blueFire, 100, 200}, 
  {multiFire, 100, 100},
  
  {ripple,  60,  40},

  {sinelon,  7, 32},
  {sinelon,  7, 4},
  
  {juggle2, 3, 5}, // new animation to try
  {juggle,   2, 4},
  {juggle,   3, 7},
  {juggle,   4, 8},
  
  // TODO applause became way too fast when 2 leds on same pin
  {applause, HUE_BLUE, HUE_PURPLE},
  {applause, HUE_BLUE, HUE_RED},
  
  // TODO Should probably remove or move to lower energy
  {twinkle,  15, 100},
  
  // TODO Slow it down, like applause
  {twinkle,  50, 224},
  
  {confetti, 20, 10},
  {confetti, 16,  3}, 

  {pride,    0,   0}, 
  
  {bpm,      15,  2},
  {bpm,      62,  3},
  {bpm,      125, 7}
};

AnimationPattern gDropAnimations[] = {
  {aboutToDrop, 100, 200},
  {dropped, 100, 200}
};

// Default sequence to main animations
volatile AnimationPattern* gSequence = gAnimations; 
// Index number of which pattern is current
volatile uint8_t gCurrentPatternNumber = 0; 

/**
 * Event Handlers
 */


void onClick() { 
  //Next animation
  PRINT("ONCLICK!");

  showBeat(600); 

  gCurrentPatternNumber = (gCurrentPatternNumber+1) % 
    (sizeof(gAnimations) / sizeof(gAnimations[0]));

  PRINTX("Moving to animation:", gCurrentPatternNumber);
  
  // Make sure we're on the main animation sequence
  gSequence = gAnimations;
}   

void onDoubleClick() { 
  //Reseting to first animation
  PRINT("Double click");
  
  // Assumes first two animations are using palettes
  if (gCurrentPatternNumber == 0 || gCurrentPatternNumber == 1) { 
    // We're already at the first animation - spice things up 
    gCurrentPaletteIndex = (gCurrentPaletteIndex + 1) 
        % (sizeof(gPalettes) / sizeof(gPalettes[0]));
    // TODO should use nblendPaletteTowardPalette(currentPalette, targetPalette, maxChanges);     
        
  } else {
    gCurrentPatternNumber = 0; 
  }
}

void onLongPressStart() { 
  PRINT("Long press");
  
  gSequence = gDropAnimations;
  gCurrentPatternNumber = 0;
}

void onLongPressEnd() { 
  PRINT("Long press end");
  // Drop the bomb
  gCurrentPatternNumber = 1;
}

void onTripleClick() { 
#if USE_SETTINGS  
  SettingsMode settings = SettingsMode(&button);
  settings.showSettings();

  uint8_t brightness = settings.getUserBrightness();
  FastLED.setBrightness(brightness); 
#endif  
}

// Showing Battery level 

#define BATT_MIN_MV 3350 // Some headroom over battery cutoff near 2.9V
#define BATT_MAX_MV 4200 // And little below fresh-charged battery near 4.1V

void showBatteryLevel() { 

  float mV = batteryLevel() * 1000; 
  PRINTX("Battery level:", mV); 
  
   uint8_t  lvl = (mV >= BATT_MAX_MV) ? NUM_LEDS : // Full (or nearly)
                 (mV <= BATT_MIN_MV) ?        1 : // Drained
                 1 + ((mV - BATT_MIN_MV) * NUM_LEDS + (NUM_LEDS / 2)) /
                 (BATT_MAX_MV - BATT_MIN_MV + 1); // # LEDs lit (1-NUM_LEDS)

  PRINTX("Lvl", lvl); 
  
  for(uint8_t i=0; i<lvl; i++) {                  // Each LED to batt level
  
    uint8_t g = (i * 5 + 2) / NUM_LEDS;           // Red to green
    
    leds[i].r = 4-g; 
    leds[i].g = g; 
    leds[i].b = 0; 
    FastLED.setBrightness(255);
    FastLED.show(); 
    delay(500/ NUM_LEDS);
  }
  delay(1500); 
}
/**
 * Setup
 */ 
void setup() {
  
  delay(2000); DEBUG_START(57600)

  PRINT("HeartLEDSuit starting...");

  // LEDs
  FastLED.addLeds<NEOPIXEL, LED40_PIN>(leds, 40).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<NEOPIXEL, LED60_PIN>(leds, 40, 60).setCorrection(TypicalLEDStrip);
  
  
#if USE_2ND_STRIP
  FastLED.addLeds<NEOPIXEL, LED2_PIN>(leds2, STRIP2_SIZE).setCorrection(TypicalLEDStrip);
#endif

  FastLED.setBrightness(DEFAULT_BRIGHTNESS);

  showBatteryLevel(); 
  
  // FastLED power management set at (default: 5V, 500mA)
  set_max_power_in_volts_and_milliamps(5, 1000);               
  
  // Button
  button.attachClick(onClick);
  button.setClickTicks(200); 
  button.attachDoubleClick(onDoubleClick); 
  button.attachLongPressStart(onLongPressStart);
  button.attachLongPressStop(onLongPressEnd);
  //button.attachTripleClick(onTripleClick);

  // Remove
  PRINTX("Gradient Palette count:", gGradientPaletteCount);
} 
  
static void delayToSyncFrameRate(uint8_t framesPerSecond) {
  static uint32_t msprev = 0;
  uint32_t mscur = millis();
  uint16_t msdelta = mscur - msprev;
  uint16_t mstargetdelta = 1000 / framesPerSecond;
  if(msdelta < mstargetdelta) {
    delay_at_max_brightness_for_power(mstargetdelta - msdelta);
  }
  msprev = mscur;
}


/** 
 * Loop and LED management
 */ 
void loop() {
  random16_add_entropy(random8());
  
  button.tick();
  
  uint8_t arg1 = gSequence[gCurrentPatternNumber].mArg1;
  uint8_t arg2 = gSequence[gCurrentPatternNumber].mArg2;
  Animation animate = gSequence[gCurrentPatternNumber].mPattern;
  
  uint8_t animDelay = animate(arg1, arg2);

 // mirrorLeds();
  
  #if REVERSE_LEDS
    reverseLeds();
  #endif
  
  switch(animDelay) { 
    
    case RANDOM_DELAY: {
      // Sync random delay to an increasing BPM as the animations progress 
      uint8_t bpmDelay = beatsin8(gCurrentPatternNumber, 100, 255);
      delay_at_max_brightness_for_power(bpmDelay);
      break;
    }

    case SYNCED_DELAY: delayToSyncFrameRate(FRAMES_PER_SECOND); break;
    
    case STATIC_DELAY: delay_at_max_brightness_for_power(70); break;

    default: delay_at_max_brightness_for_power(animDelay);
  };

  show_at_max_brightness_for_power();      

  EVERY_N_MILLISECONDS(40) {
    gHue++;  // slowly cycle the "base color" through the rainbow
  }

  // blend the current palette to the next
  EVERY_N_MILLISECONDS(40) {
    
    nblendPaletteTowardPalette(currentPalette, targetPalette, 16);
    nblendPaletteTowardPalette(gCurrentGradientPalette, gTargetGradientPalette, 16);
  }
  
  // slowly change to a new palette
  EVERY_N_SECONDS(SECONDS_PER_PALETTE) {

    
    //FastLed Palettes
    gCurrentPaletteIndex = addmod8(gCurrentPaletteIndex, 1, ARRAY_SIZE(gPalettes)); 
    targetPalette = gPalettes[gCurrentPaletteIndex];

    PRINT("Change CTC Palette"); 
    // dummy code
    gTargetGradientPalette = gPalettes[gCurrentPaletteIndex];
    
    //CPC Gradient Palettes
//    gGradientPaletteIndex = addmod8(gGradientPaletteIndex, 1, gGradientPaletteCount);
//    gTargetGradientPalette = gGradientPalettes[gGradientPaletteIndex];

    PRINTX("New gradient palette", gGradientPaletteIndex);
  };


  #ifdef DEBUG_ANIMATIONS
  EVERY_N_MILLISECONDS(500)  {Serial.print("FPS: ");Serial.println(FastLED.getFPS());}
  #endif
  
  #ifdef DEBUG
  EVERY_N_MILLISECONDS(3000) {PRINTX("BATTERY LEVEL: ", batteryLevel());}
  #endif
} 


void mirrorLeds() { 
  
  PRINTX("Rendering:", gRenderingSettings);
  
  for (int i = 0; i < STRIP_SIZE; i++) { 
    if (i < NUM_LEDS) { 
      
#if USE_2ND_STRIP
      if (gRenderingSettings != LEFT_STRIP_ONLY) {
        leds2[i] = leds[i];
        
        if (gRenderingSettings == RIGHT_STRIP_ONLY) { 
          leds[i] = CRGB::Black;
        }
      
      } else { 
        leds2[i] = CRGB::Black;
      }
#endif 

      if (i < STRIP_SIZE - NUM_LEDS) { 
        // Copy to the front side
        leds[STRIP_SIZE-i-1] = leds[i];
        // Dim the back by max 50%
        leds[i].fadeLightBy(128*(1/i+1));
      }
   }

  }

  // Go back to default
  //gRenderingSettings = BOTH_STRIPS;
  
}


