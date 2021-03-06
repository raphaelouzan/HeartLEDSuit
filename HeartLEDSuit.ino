#include <FastLED.h>
#include <Wire.h>

/**
   Variable Components
*/
#define USE_2ND_STRIP       1
#define USE_SETTINGS        0
#define USE_MEMBRANE_SWITCH 0
#define USE_IOT             0
#define DEBUG
#include "DebugUtils.h"

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))
/**
   LEDS
*/
// Size of the strip, including both front and back of the strip
#define STRIP_SIZE      100
#define LED40_PIN       10
#define LED60_PIN        6
struct CRGB leds[STRIP_SIZE];

#if USE_2ND_STRIP
#define STRIP2_SIZE     29   // must be shorter than STRIP_SIZE
#define LED2_PIN        12
#define LED3_PIN        5
struct CRGB leds2[STRIP2_SIZE * 2];
#endif

// Number of LEDs for the front side of the suit (will be mirrored on what's left of the strip in the back if reverse led is on)
#define NUM_LEDS        100
#define REVERSE_LEDS    0

#define DEFAULT_BRIGHTNESS 200
#define FRAMES_PER_SECOND  100

/**
   Button Switcher
*/
#include "Button.h"
#define HEART_BUTTON_PIN      13
Button button(HEART_BUTTON_PIN, false);
#if USE_MEMBRANE_SWITCH
#include "XButton.h"
#define MEMBRANE_BUTTON_PIN   A1
XButton mButton(MEMBRANE_BUTTON_PIN, true);
#endif

/**
   Animations
*/

#include "PaletteMgr.h"
PaletteMgr palettes;
#include "Animations.h"

//#include "GradientPalettes.h"
// 10 seconds per color palette makes a good demo, 20-120 is better for deployment
#define SECONDS_PER_PALETTE    30
#define AUTOPLAY_ENABLED       1
#define SECONDS_PER_ANIMATION  180 // 3 mins




/*
   Settings UI
*/
#if USE_SETTINGS
#include "SettingsMode.h"
#endif

/**
   Microphone
*/
#define MIC_PIN A4
#include "SoundReactive.h"


/**
   Sequencing
*/
AnimationPattern gAnimations[] = {

   // test pulse - no
  {soundAnimate, 2, 10}, 

  {beatTriggered, 20, 100},

  {sinelon, 120, 2},

  // breathing full colors, rapid changes of color tones. #warm #powerful
  {wave, 0, 0},

  {discostrobe, 40, 2},

  // should use the general palette
  {twinkleFox, 6, 1},

  {multiFire, 70, 60},

  // [use CPT]
  {colorWaves, 1, 0}, // using Fibonacci, I think this one is the best

  // Slowercolor changes, create powerful color effects #mesmerizing [use CPT]
  {radialPaletteShift, 0, 0},

  // Fully colored, subtle changes [use CPT]
  {incrementalDrift, 0, 0},

  {pulse, 0, 0},

  {life, 0, 0},

  {breathing, 24, 33},

  {pride,    0,   0},

  // make ripple work with color palette
  {ripple,  60,  40},

  {sinelon,  13, 4},

  {juggle,   4, 8},

  // Pastel colors
  {verticalRainbow, 0, 0},

  {applause, HUE_BLUE, HUE_RED},

  {confetti, 20, 10},

  {bpm,      120, 7}
};

AnimationPattern gDropAnimations[] = {
  {aboutToDrop, 100, 200},
  {discostrobe, 120, 2}
};

// Default sequence to main animations
volatile AnimationPattern* gSequence = gAnimations;
// Index number of which pattern is current
volatile uint8_t gCurrentPatternNumber = 0;


void onClick() {
  
  showBeat(250);

  if (gSequence == gAnimations) {
    gCurrentPatternNumber =  addmod8(gCurrentPatternNumber, 1, ARRAY_SIZE(gAnimations));
  } else {
    // we're on a different animation sequence
    gCurrentPatternNumber = 0;
    gSequence = gAnimations;
  }

  PRINTX("Click - Moving to animation:", gCurrentPatternNumber);
}

void onClickFromMembrane() { PRINT("Click from membrane:"); onClick();   }
void onClickFromIoT()      { PRINT("Remote Click from IoT:"); onClick(); }

void onDoubleClick() {
  //Reseting to first animation
  PRINT("Double click");

  showBeat(100);

  if (gCurrentPatternNumber == 0 || gCurrentPatternNumber == 1) {
    // We're already at the first animation - spice things up
    palettes.moveToNextPalette();
  } else {
    gCurrentPatternNumber = 0;
  }
}

void onLongPressStart() {
  PRINT("Long press");

  gSequence = gDropAnimations;
  initDropAnimations();

  gCurrentPatternNumber = 0;
}

void onLongPressEnd() {
  PRINT("Long press end");
  // Drop the bomb
  gCurrentPatternNumber = 1;
}

void onTripleClick() {
#if USE_SETTINGS
  SettingsMode settings = SettingsMode(&mButton);
  settings.showSettings();

  uint8_t brightness = settings.getUserBrightness();
  FastLED.setBrightness(brightness);
#endif
}

// Showing Battery level

#define BATT_MIN_MV 3350 // Some headroom over battery cutoff near 2.9V
#define BATT_MAX_MV 4200 // And little below fresh-charged battery near 4.1V

void showBatteryLevel() {

  float mV = getBatteryLevel() * 1000;
  PRINTX("Battery level:", mV);

  uint8_t  lvl = (mV >= BATT_MAX_MV) ? NUM_LEDS : // Full (or nearly)
                 (mV <= BATT_MIN_MV) ?        1 : // Drained
                 1 + ((mV - BATT_MIN_MV) * NUM_LEDS + (NUM_LEDS / 2)) /
                 (BATT_MAX_MV - BATT_MIN_MV + 1); // # LEDs lit (1-NUM_LEDS)

  PRINTX("Lvl", lvl);

  for (uint8_t i = 0; i < lvl; i++) {             // Each LED to batt level

    uint8_t g = (i * 5 + 2) / NUM_LEDS;           // Red to green

    leds[i].r = 4 - g;
    leds[i].g = g;
    leds[i].b = 0;
    FastLED.setBrightness(255);
    FastLED.show();
    delay(500 / NUM_LEDS);
  }
  delay(1500);
}

/* 
  IoT Wifi Client
*/ 
#if USE_IOT
#include "IOTClient.h"
#define IOT_LOOP_BLOCKING_TIME 3 // 3 ms per loop to fetch updates
#endif 

/**
   Setup
*/


void setup() {

#if USE_IOT
  setupIoTConnection();
#else 
  delay(1000);
#endif  

  DEBUG_START(57600)

  PRINT("HeartLEDSuit starting...");

  // LEDs
  FastLED.addLeds<NEOPIXEL, LED40_PIN>(leds, 40).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<NEOPIXEL, LED60_PIN>(leds, 40, 60).setCorrection(TypicalLEDStrip);


#if USE_2ND_STRIP
  FastLED.addLeds<NEOPIXEL, LED2_PIN>(leds2, STRIP2_SIZE).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<NEOPIXEL, LED3_PIN>(leds2, STRIP2_SIZE, STRIP2_SIZE).setCorrection(TypicalLEDStrip);
#endif

  FastLED.setBrightness(DEFAULT_BRIGHTNESS);

  showBatteryLevel();

  // FastLED power management set at (default: 5V, 500mA)
  set_max_power_in_volts_and_milliamps(5, 1000);

  // Button
  button.attachClick(onClick);
  button.attachDoubleClick(onDoubleClick);
  button.attachLongPressStart(onLongPressStart);
  button.attachLongPressStop(onLongPressEnd);
  button.setClickTicks(100);

#if USE_MEMBRANE_SWITCH
  mButton.attachClick(onClickFromMembrane);
  //  mButton.attachDoubleClick(onDoubleClick);
  mButton.attachLongPressStart(onLongPressStart);
  mButton.attachLongPressStop(onLongPressEnd);
  mButton.attachTripleClick(onTripleClick);
  mButton.setClickTicks(600);
#endif


}

static void delayToSyncFrameRate(uint8_t framesPerSecond) {
  static uint32_t msprev = 0;
  uint32_t mscur = millis();
  uint16_t msdelta = mscur - msprev;
  uint16_t mstargetdelta = 1000 / framesPerSecond;
  if (msdelta < mstargetdelta) {
    delay_at_max_brightness_for_power(mstargetdelta - msdelta);
  }
  msprev = mscur;
}


/**
   Loop and LED management
*/

#define FIRST_2_RINGS_NUM_LEDS  40
#define FADING_RATE 5
void mirrorLedsToSecondaryStrips() {

#if USE_2ND_STRIP

  // Assumes STRIP2_SIZE is shorter than NUM_LEDS and STRIP_SIZE
  for (int left = 0, right = STRIP2_SIZE; left < STRIP2_SIZE; left++, right++) {

    // Copy one for one for the left strip
    leds2[left] = leds[left];
    // Copy later in the ring for the right strip
    leds2[right] = leds[FIRST_2_RINGS_NUM_LEDS + left];

    leds2[left].fadeToBlackBy(FADING_RATE);
    leds2[right].fadeToBlackBy(FADING_RATE);

    //Randomly re-fuel some of the LEDs that are currently lit (1% chance per cycle)
    //This enhances the twinkling effect.
    if (leds2[left].r > 10) {
      if (random8(100) < 1) {
        //Set the red channel to a value of 80
        leds2[left].r = 80;
        //Increase the green channel to 20 - to add to the effect
        leds2[left].g = 20;
      }
    }
    if (leds2[right].b > 10) {
      if (random8(100 < 1)) {
        leds2[right].b = 80;
        leds2[right].g = 20;
      }
    }

  }

#endif
}

#define BRIGHTNESS_FALLRATE  40

void decreaseBrightness() { 

  static uint8_t easeOutVal = 0;
  static uint8_t easeInVal  = 0;
  static uint8_t newBrightness    = 0;

  easeOutVal = ease8InOutQuad(easeInVal);
  easeInVal++;

  newBrightness = lerp8by8(0, 255/*max brightness*/, easeOutVal);

  PRINTX("Lowering brightness to:", newBrightness); 

  FastLED.setBrightness(newBrightness); 
}

// Experimental: make the animation fade and glitter to a BPM 
void bpmFilter(uint8_t bpm = 60) { 

  static int state = 0; 
  const uint8_t numStates = 4; 

  static uint8_t prevBeat = 0; 


  EVERY_N_SECONDS(10) {state = addmod8(state, 1, numStates); PRINTX("BPM - Moving to state", state);}

  EVERY_N_SECONDS(BRIGHTNESS_FALLRATE) { decreaseBrightness(); }

  uint8_t x = 0; 

  if (state == 0) { 
    x = beat8(bpm); 
    fadeLightBy(leds, NUM_LEDS, 255-x);
  } else if (state == 1) { 
    x = beatsin8(bpm); 
    fadeLightBy(leds, NUM_LEDS, 255-x);
  } else if (state == 2) { 
    x = beat8(bpm); 

    if (x < prevBeat) { 
      // beat lowering
      fadeLightBy(leds, NUM_LEDS, 255-x);
    }    
  } else if (state == 3) { 
    x = beat8(bpm);
    // Alternatively turn off the first 2 rings then the large ring
    if (x < prevBeat) { 
      fadeLightBy(leds, 40, 255-x);
    }  else { 
      // rising
      fadeLightBy(&leds[40], 60, 255-x);
    }
  }

  prevBeat = x;

  /*
  Try to play with the two rings, when going down use the center rings then up use the outer ring 

  or even with the colors: hue = map8( sin8( myValue), HUE_BLUE, HUE_RED);
  */
}

void loop() {
  random16_add_entropy(random8());

  button.tick();
#if USE_MEMBRANE_SWITCH
  mButton.tick();
#endif

  uint8_t arg1 = gSequence[gCurrentPatternNumber].mArg1;
  uint8_t arg2 = gSequence[gCurrentPatternNumber].mArg2;
  Animation animate = gSequence[gCurrentPatternNumber].mPattern;

  uint8_t animDelay = animate(arg1, arg2);

  mirrorLedsToSecondaryStrips();

  //bpmFilter(); 

  uint32_t delayTimeDelta = 0; 

#if USE_IOT
  uint32_t curr_time = millis(); 
  loopMQTT(IOT_LOOP_BLOCKING_TIME);
  delayTimeDelta = millis() - curr_time; 
#endif  

  uint8_t delayToWait = animDelay; 

  if (animDelay != NO_DELAY) { 
    if (animDelay == SYNCED_DELAY) { 
      delayToSyncFrameRate(FRAMES_PER_SECOND);
    } else { 
      if (animDelay == RANDOM_DELAY) { 
        // TODO Try to sync on actual BPM 
        delayToWait = beatsin8(gCurrentPatternNumber, 100, 255); 
      } 
      delayToWait = max(delayToWait-delayTimeDelta, delayTimeDelta); 
      delay_at_max_brightness_for_power(delayToWait);
    } 
  }

  show_at_max_brightness_for_power();

  // Autoplay (5 mins)
#if AUTOPLAY_ENABLED
  EVERY_N_SECONDS(SECONDS_PER_ANIMATION) {

    // Don't run autoplay on drop squence or first animation
    if (gSequence != gAnimations || gCurrentPatternNumber == 0) return;

    gCurrentPatternNumber =  addmod8(gCurrentPatternNumber, 1, ARRAY_SIZE(gAnimations));
    gSequence = gAnimations;
    PRINTX("AUTOPLAY - Moving to the next animation", gCurrentPatternNumber);
  }
#endif

  // blend the current palette to the next
  EVERY_N_MILLISECONDS(40) {
    gHue++;  // slowly cycle the "base color" through the rainbow
    palettes.blendPalettes();
  }

  // slowly change to a new palette
  EVERY_N_SECONDS(SECONDS_PER_PALETTE) {
    palettes.queueNextPalette();
  };

#ifdef DEBUG
  EVERY_N_MILLISECONDS(3000) {
    Serial.print("FPS: ");
    Serial.print(FastLED.getFPS());
    Serial.print(" ||  BATTERY LEVEL: ");
    Serial.print(getBatteryLevel());
    Serial.print(" || AnimationIndex: ");
    Serial.print(gCurrentPatternNumber);
    Serial.println("");
 
  }
#endif
}




