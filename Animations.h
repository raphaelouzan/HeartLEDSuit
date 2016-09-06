#include <FastLed.h>

#if FASTLED_VERSION < 3001000
  #error "Requires FastLED 3.1 or later; check github for latest code."
#endif

uint8_t gHue = 0;

/**
 * Animations
 */

typedef uint8_t (*Animation)(uint8_t arg1, uint8_t arg2);
typedef struct {
  Animation mPattern;
  uint8_t mArg1;
  uint8_t mArg2;
} AnimationPattern;

typedef enum delayType {
  RANDOM_DELAY = 2,
  STATIC_DELAY = 3,
  NO_DELAY     = 1,
  SYNCED_DELAY = 4
} delayType;

#define LEFT_STRIP_ONLY  1
#define RIGHT_STRIP_ONLY 2
#define BOTH_STRIPS      3

uint8_t gRenderingSettings = BOTH_STRIPS;

// Fibonacci animations, adapted from https://github.com/evilgeniuslabs/fibonacci-v3d
#include "FiboMatrix.h" 
#include "FiboLife.h"

#include "TwinkleFox.h"


uint8_t cylon(uint8_t strip, uint8_t num2) {
  static int step = 0;
  static bool goingUp = true;

  int lastStep = step;

  if (step >= NUM_LEDS) {
    goingUp = false;
  } else if (step <= 0) {
    goingUp = true;
  }

  if (goingUp)
    step++;
  else
    step--;


  leds[step] = CHSV(gHue, 255, 255);
  leds[lastStep].nscale8(128);

  switch (strip) {
    case 1: gRenderingSettings = LEFT_STRIP_ONLY; break;
    case 2: gRenderingSettings = RIGHT_STRIP_ONLY; break;
    case 3: gRenderingSettings = BOTH_STRIPS; break;
  };

  return SYNCED_DELAY;
}

uint8_t juggle(uint8_t numDots, uint8_t baseBpmSpeed) {
  // numDots colored dots, weaving in and out of sync with each other
  fadeToBlackBy(leds, NUM_LEDS, 100);
  byte dothue = 0;
  for (int i = 0; i < numDots; i++) {
    leds[beatsin16(i + baseBpmSpeed, 0, NUM_LEDS)] |= CHSV(dothue, 255, 224);
    dothue += (256 / numDots);
  }

  return STATIC_DELAY;
}


uint8_t bpm(uint8_t bpmSpeed, uint8_t stripeWidth) {
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8(bpmSpeed, 64, 255);
  for ( int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette(palette, gHue + (i * stripeWidth), beat);
  }

  return NO_DELAY;

}

uint8_t sinelon(uint8_t bpmSpeed = 13, uint8_t fadeAmount = 20) {
  // a colored dot sweeping 
  // back and forth, with 
  // fading trails
  fadeToBlackBy(leds, NUM_LEDS, fadeAmount);
  int pos = beatsin16(13, 0, NUM_LEDS);
  static int prevpos = 0;
  if(pos < prevpos) { 
    fill_solid(leds+pos, (prevpos-pos)+1, CHSV(gHue, 220, 255));
  } else { 
    fill_solid(leds+prevpos, (pos-prevpos)+1, CHSV(gHue, 220, 255));
  }
  
  prevpos = pos;

  return NO_DELAY; 
}

// An animation to play while the crowd goes wild after the big performance
uint8_t applause(uint8_t minHue, uint8_t maxHue) {
  static uint16_t lastPixel = 0;
  fadeToBlackBy(leds, NUM_LEDS, 32);
  leds[lastPixel] = CHSV(random8(minHue, maxHue), 255, 255);
  lastPixel = random16(NUM_LEDS);
  leds[lastPixel] = CRGB::White;

  return RANDOM_DELAY;
}

uint8_t confetti(uint8_t colorVariation, uint8_t fadeAmount) {
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy(leds, NUM_LEDS, fadeAmount);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV(gHue + random8(colorVariation), 200, 255);

  return RANDOM_DELAY;

}

// @param chanceOfTwinkle  The higher the number, lowers the chance for a pixel to light up. (50)
// by @atuline
uint8_t twinkle(uint8_t chanceOfTwinkle, uint8_t fadeRate) {
  if (chanceOfTwinkle > NUM_LEDS) chanceOfTwinkle = NUM_LEDS;               // Make sure we're at least utilizing ALL the LED's.
  int index = random16(0, chanceOfTwinkle);
  if (index < NUM_LEDS) {                                      // Only the lowest probability twinkles will do.
    leds[index] = random8();                                    // The idex LED is set to a random 32 bit value
  }
  for (int i = 0; i < NUM_LEDS; i++)
    leds[i].nscale8(fadeRate); // Go through the array and reduce each RGB value by a percentage.

  return RANDOM_DELAY;
}


// Wrap around the strip
int wrap(int step) {
  if (step < 0) return NUM_LEDS + step;
  if (step > NUM_LEDS - 1) return step - NUM_LEDS;
  return step;
}

// Ripple (inspired by @atuline)
#define RIPPLE_FADE_RATE 255
// Ripple effect with trailing dots (alternatively), color randomized for each ripple
// TODO Ripples should be spaced out by some sinus function instead of a static delay to make it feel more organic
uint8_t ripple(uint8_t rippleSize, uint8_t fadeToBlackRate) {

  static int step = -1;
  static int center = 0;  // Center of the current ripple
  static uint8_t color; // Ripple colour
  static boolean trailingDots; // whether to add trailing dots to the ripple
  static int maxSteps;

  fadeToBlackBy(leds, NUM_LEDS, fadeToBlackRate);

  if (step == -1) {

    // Initalizing ripple
    center = random(NUM_LEDS);
    color = gHue;
    maxSteps =  min(random(rippleSize / 2, rippleSize), NUM_LEDS); // Randomize ripple size
    trailingDots = random(0, 2) % 2;
    step = 0;

  } else if (step == 0) {

    // First pixel of the ripple
    leds[center] = CHSV(color, 255, 255);
    step++;

  } else if (step < maxSteps) {

    // In the Ripple
    uint8_t fading = RIPPLE_FADE_RATE / step * 2;
    leds[wrap(center + step)] += CHSV(color + step, 255, fading); // Display the next pixels in the range for one side.
    leds[wrap(center - step)] += CHSV(color - step, 255, fading); // Display the next pixels in the range for the other side.
    step++;

    if (trailingDots && step > 3) {
      // Add trailing dots
      leds[wrap(center + step - 3)] = CHSV(color - step, 255, fading);
      leds[wrap(center - step + 3)] = CHSV(color + step, 255, fading);
    }

  } else {
    // Ending the ripple
    step = -1;
  }

  return SYNCED_DELAY;
}

uint8_t beatCubic8x(accum88 beats_per_minute, uint8_t lowest = 0, uint8_t highest = 255, int type = 0, int offset = 0)
{
  uint8_t beat = beat8(beats_per_minute);
  beat += offset;
  uint8_t beatsin = cubicwave8(beat);
  uint8_t rangewidth = highest - lowest;
  uint8_t scaledbeat = scale8(beatsin, rangewidth);
  uint8_t result = lowest + scaledbeat;
  return result;
}


// @param COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 55, suggested range 20-100

// @param SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.

uint8_t fire(uint8_t cooling, uint8_t sparking, const CRGBPalette16& palette) {

  // Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS];

  // Step 1.  Cool down every cell a little
  for (int i = 0; i < NUM_LEDS; i++) {
    heat[i] = qsub8( heat[i],  random8(0, ((cooling * 10) / NUM_LEDS) + 2));
  }

  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for (int k = NUM_LEDS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
  }

  // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
  if (random8() < sparking ) {
    int y = random8(7);
    heat[y] = qadd8(heat[y], random8(160, 255) );
  }

  // Step 4.  Map from heat cells to LED colors
  for ( int j = 0; j < NUM_LEDS; j++) {
    // Scale the heat value from 0-255 down to 0-240
    // for best results with color palettes.
    byte colorindex = scale8(heat[j], 240);
    leds[j] = ColorFromPalette(palette, colorindex);
  }

  return STATIC_DELAY;
}




uint8_t blueFire(uint8_t cooling, uint8_t sparking) {
  static const CRGBPalette16 bluePalette = CRGBPalette16(CRGB::Black, CRGB::Blue, CRGB::Aqua,  CRGB::White);
  return fire(cooling, sparking, bluePalette);
}

uint8_t multiFire(uint8_t cooling, uint8_t sparking) {
  CRGB darkcolor  = CHSV(gHue, 255, 192); // pure hue, three-quarters brightness
  CRGB lightcolor = CHSV(gHue, 128, 255); // half 'whitened', full brightness
  const CRGBPalette16 pal = CRGBPalette16(CRGB::Black, darkcolor, lightcolor, CRGB::White);

  return fire(cooling, sparking, pal);
}

uint8_t multiFire2(uint8_t cooling, uint8_t sparking) { 
  return fire(cooling, sparking, palettes.getGradientPalette());
}

// From Marks Kriegman's https://gist.github.com/kriegsman/964de772d64c502760e5
uint8_t pride(uint8_t a, uint8_t b) {
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;

  uint8_t sat8 = beatsin88(87, 220, 250);
  uint8_t brightdepth = beatsin88(341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88(203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16;//gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 1, 3000);

  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis ;
  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88( 400, 5, 9);
  uint16_t brightnesstheta16 = sPseudotime;

  for (uint16_t i = 0 ; i < NUM_LEDS; i++) {

    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;

    brightnesstheta16  += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);

    CRGB newcolor = CHSV(hue8, sat8, bri8);

    uint16_t pixelnumber = i;
    pixelnumber = (NUM_LEDS - 1) - pixelnumber;

    nblend(leds[pixelnumber], newcolor, 64);
  }

  return STATIC_DELAY;
}



/*
   Touch Animations
*/

uint8_t fadeOut(uint8_t fadeAmount, uint8_t b) {
  fadeToBlackBy(leds, NUM_LEDS, fadeAmount);
  return STATIC_DELAY;
}

// drawRainbowDashes - draw rainbow-colored 'dashes' of light along the led strip:
//   starting from 'startpos', up to and including 'lastpos'
//   with a given 'period' and 'width'
//   starting from a given hue, which changes for each successive dash by a 'huedelta'
//   at a given saturation and value.
//
//   period = 5, width = 2 would be  _ _ _ X X _ _ _ Y Y _ _ _ Z Z _ _ _ A A _ _ _
//                                   \-------/       \-/
//                                   period 5      width 2
//
static void drawRainbowDashes(
  uint8_t startpos, uint16_t lastpos, uint8_t period, uint8_t width,
  uint8_t huestart, uint8_t huedelta, uint8_t saturation, uint8_t value)
{
  uint8_t hue = huestart;
  for ( uint16_t i = startpos; i <= lastpos; i += period) {
    CRGB color = CHSV( hue, saturation, value);

    // draw one dash
    uint16_t pos = i;
    for ( uint8_t w = 0; w < width; w++) {
      leds[ pos ] = color;
      pos++;
      if ( pos >= NUM_LEDS) {
        break;
      }
    }

    hue += huedelta;
  }
}

// discoWorker updates the positions of the dashes, and calls the draw function
//
void discoWorker(
  uint8_t dashperiod, uint8_t dashwidth, int8_t  dashmotionspeed,
  uint8_t stroberepeats,
  uint8_t huedelta)
{
  static uint8_t sRepeatCounter = 0;
  static int8_t sStartPosition = 0;
  static uint8_t sStartHue = 0;

  // Always keep the hue shifting a little
  sStartHue += 1;

  // Increment the strobe repeat counter, and
  // move the dash starting position when needed.
  sRepeatCounter = sRepeatCounter + 1;
  if ( sRepeatCounter >= stroberepeats) {
    sRepeatCounter = 0;

    sStartPosition = sStartPosition + dashmotionspeed;

    // These adjustments take care of making sure that the
    // starting hue is adjusted to keep the apparent color of
    // each dash the same, even when the state position wraps around.
    if ( sStartPosition >= dashperiod ) {
      while ( sStartPosition >= dashperiod) {
        sStartPosition -= dashperiod;
      }
      sStartHue  -= huedelta;
    } else if ( sStartPosition < 0) {
      while ( sStartPosition < 0) {
        sStartPosition += dashperiod;
      }
      sStartHue  += huedelta;
    }
  }

  // draw dashes with full brightness (value), and somewhat
  // desaturated (whitened) so that the LEDs actually throw more light.
  const uint8_t kSaturation = 208;
  const uint8_t kValue = 255;

  // call the function that actually just draws the dashes now
  drawRainbowDashes( sStartPosition, NUM_LEDS - 1,
                     dashperiod, dashwidth,
                     sStartHue, huedelta,
                     kSaturation, kValue);
}


uint8_t discostrobe(uint8_t zoomBPM = 120, uint8_t strobeCycleLength = 4) {
  // First, we black out all the LEDs
  fill_solid(leds, NUM_LEDS, CRGB::Black);

  // To achive the strobe effect, we actually only draw lit pixels
  // every Nth frame (e.g. every 4th frame).
  // sStrobePhase is a counter that runs from zero to strobeCycleLength-1,
  // and then resets to zero.

  // strobeCycleLength = 4; //  light every Nth frame
  static uint8_t sStrobePhase = 0;
  sStrobePhase = sStrobePhase + 1;
  if ( sStrobePhase >= strobeCycleLength ) {
    sStrobePhase = 0;
  }

  // We only draw lit pixels when we're in strobe phase zero;
  // in all the other phases we leave the LEDs all black.
  if (sStrobePhase == 0) {

    // The dash spacing cycles from 4 to 9 and back, 8x/min (about every 7.5 sec)
    uint8_t dashperiod = beatsin8( 8/*cycles per minute*/, 4, 10);
    // The width of the dashes is a fraction of the dashperiod, with a minimum of one pixel
    uint8_t dashwidth = (dashperiod / 4) + 1;

    // The distance that the dashes move each cycles varies
    // between 1 pixel/cycle and half-the-dashperiod/cycle.
    // At the maximum speed, it's impossible to visually distinguish
    // whether the dashes are moving left or right, and the code takes
    // advantage of that moment to reverse the direction of the dashes.
    // So it looks like they're speeding up faster and faster to the
    // right, and then they start slowing down, but as they do it becomes
    // visible that they're no longer moving right; they've been
    // moving left.  Easier to see than t o explain.
    //
    // The dashes zoom back and forth at a speed that 'goes well' with
    // most dance music, a little faster than 120 Beats Per Minute.  You
    // can adjust this for faster or slower 'zooming' back and forth.
    int8_t  dashmotionspeed = beatsin8( (zoomBPM / 2), 1, dashperiod);
    // This is where we reverse the direction under cover of high speed
    // visual aliasing.
    if ( dashmotionspeed >= (dashperiod / 2)) {
      dashmotionspeed = 0 - (dashperiod - dashmotionspeed );
    }


    // The hueShift controls how much the hue of each dash varies from
    // the adjacent dash.  If hueShift is zero, all the dashes are the
    // same color. If hueShift is 128, alterating dashes will be two
    // different colors.  And if hueShift is range of 10..40, the
    // dashes will make rainbows.
    // Initially, I just had hueShift cycle from 0..130 using beatsin8.
    // It looked great with very low values, and with high values, but
    // a bit 'busy' in the middle, which I didnt like.
    //   uint8_t hueShift = beatsin8(2,0,130);
    //
    // So instead I layered in a bunch of 'cubic easings'
    // (see http://easings.net/#easeInOutCubic )
    // so that the resultant wave cycle spends a great deal of time
    // "at the bottom" (solid color dashes), and at the top ("two
    // color stripes"), and makes quick transitions between them.
    uint8_t cycle = beat8(2); // two cycles per minute
    uint8_t easedcycle = ease8InOutCubic( ease8InOutCubic( cycle));
    uint8_t wavecycle = cubicwave8( easedcycle);
    uint8_t hueShift = scale8( wavecycle, 130);


    // Each frame of the animation can be repeated multiple times.
    // This slows down the apparent motion, and gives a more static
    // strobe effect.  After experimentation, I set the default to 1.
    uint8_t strobesPerPosition = 1; // try 1..4


    // Now that all the parameters for this frame are calculated,
    // we call the 'worker' function that does the next part of the work.
    discoWorker(dashperiod, dashwidth, dashmotionspeed, strobesPerPosition, hueShift);
  }

  return SYNCED_DELAY;
}

void fadeAndTwinkleBlood(int fadeVal) {
  for (int i = 0; i < NUM_LEDS; i++) {
    //Fade every LED by the fadeVal amount
    leds[i].fadeToBlackBy(fadeVal);

    //Randomly re-fuel some of the LEDs that are currently lit (1% chance per cycle)
    //This enhances the twinkling effect.
    if (leds[i].r > 10) {
      if (random8(100) < 1) {
        //Set the red channel to a value of 80
        leds[i].r = 80;
        //Increase the green channel to 20 - to add to the effect
        leds[i].g = 20;
      }
    }
  }
}

// Watch out: this function calls "show/show_at_max_brightness" itself 
// to be an independent, blocking animation
void showBeat(int animLength) {

  const uint8_t numLedsForFirstPhase = 40; 
  int timeForBloodCells = animLength / NUM_LEDS;
  
  fadeAndTwinkleBlood(100);

  for (int i = 0; i < NUM_LEDS; i++) {

    if (i % 2 == 0) {
      fadeAndTwinkleBlood(5);
    }
    
    // First phase
    if (i <= numLedsForFirstPhase) {
      leds[i].r = random8();
      if (i == numLedsForFirstPhase) {
        // End of first phase, show it at once
        show_at_max_brightness_for_power();
        delay_at_max_brightness_for_power(timeForBloodCells * numLedsForFirstPhase);
      }
    } else {
      // Second phase (show cell by cell)
      leds[i].b = random8(120);
      show_at_max_brightness_for_power();
      delay_at_max_brightness_for_power(timeForBloodCells);
    }
     
  }
}


uint8_t beatTriggered(uint8_t delayBetweenHeartbeats /*multiplied by 100, default: 16 */, uint8_t delayBetweenPhases /*100*/) {
  
  // Heartbeat is usually around 60-90 for adults
  // Systole - X (1/3 of the time)
  // Diastole - relaxation (2/3 of the time)

  static uint8_t step = -1;

  static boolean beatInProgress = true;
  uint8_t wantedDelay = NO_DELAY;

  // TODO Check that this works since it's using a macro
  EVERY_N_MILLISECONDS(delayBetweenHeartbeats * 100) {
    beatInProgress = true;
  }

  if (beatInProgress) {

    step++;

    if (step <= 40) {
      // Systole paint with redish blood
      //leds[step] = ColorFromPalette(gCurrentGradientPalette, 90 + map(step, 0, NUM_LEDS-1, 0, 255), 100, LINEARBLEND);
      leds[step] = ColorFromPalette(palettes.getPalette(), 90 + map(step, 0, NUM_LEDS-1, 0, 255), 100, LINEARBLEND);
      if (random8(2) % 2) leds[step].r = random8();
      
      if (step == 40) {
        wantedDelay = delayBetweenPhases;
      }
    } else if (step <= 100) {
      // Diastole painted with blueish blood
      leds[step] = ColorFromPalette(palettes.getPalette(), 180 + map(step, 0, NUM_LEDS-1, 0, 255), 100, LINEARBLEND);
      if (random8(2) % 2) leds[step].b = random8(120);
      
      if (step == 100) {
        // Finished heart beat
        beatInProgress = false;
        wantedDelay = delayBetweenPhases;
        step = -1;
      }
    }

    if (step % 3 == 0) {
      fadeAndTwinkleBlood(5);
    }

  } else {
    fadeAndTwinkleBlood(5);
  }


  return wantedDelay;
}

uint8_t breathing(uint8_t bpmSpeed, uint8_t fadeAmount) {

  int length = beatCubic8x(bpmSpeed, 0, NUM_LEDS, 3);
  int light = beatCubic8x(bpmSpeed, 100, 200, 3);

  static bool inhale = true;

  if (inhale) {
    gRenderingSettings = BOTH_STRIPS;
    leds[length] = CHSV(HUE_BLUE, 255, light);
    leds[length].b = random8(120);
  } else {
    gRenderingSettings = RIGHT_STRIP_ONLY;
    for (int i = 0; i < length; i++) {
      leds[i] = CHSV(HUE_RED, 255, light);
      leds[length].r = random8();
    }

    if (length % 3) {
      for (int i = 0; i < length; i++) {
        if (leds[i].r > 10) {
          if (random8(100) < 20) {
            leds[i].r = 80;
            leds[i].g = 20;
          }
        }
      }
    }

    fadeAndTwinkleBlood(light); 
    //fadeToBlackBy(leds, NUM_LEDS, light);
  }

  
  if (length == 0) {
    inhale = false;
  } else if (length == NUM_LEDS - 1) {
    inhale = true;
  }

  return STATIC_DELAY;
}



// new animation to try

uint8_t juggle2(uint8_t a, uint8_t b)
{
  static uint8_t    numdots =   4; // Number of dots in use.
  static uint8_t   faderate =   2; // How long should the trails be. Very low value = longer trails.
  static uint8_t     hueinc =  255 / numdots - 1; // Incremental change in hue between each dot.
  static uint8_t    thishue =   0; // Starting hue.
  static uint8_t     curhue =   0; // The current hue
  static uint8_t    thissat = 255; // Saturation of the colour.
  static uint8_t thisbright = 255; // How bright should the LED/display be.
  static uint8_t   basebeat =   5; // Higher = faster movement.

  static uint8_t lastSecond =  99;  // Static variable, means it's only defined once. This is our 'debounce' variable.
  uint8_t secondHand = (millis() / 1000) % 30; // IMPORTANT!!! Change '30' to a different value to change duration of the loop.

  if (lastSecond != secondHand) { // Debounce to make sure we're not repeating an assignment.
    lastSecond = secondHand;
    switch (secondHand) {
      case  0: numdots = 1; basebeat = 20; hueinc = 16; faderate = 2; thishue = 0; break; // You can change values here, one at a time , or altogether.
      case 10: numdots = 4; basebeat = 10; hueinc = 16; faderate = 8; thishue = 128; break;
      case 20: numdots = 8; basebeat =  3; hueinc =  0; faderate = 8; thishue = random8(); break; // Only gets called once, and not continuously for the next several seconds. Therefore, no rainbows.
      case 30: break;
    }
  }

  // Several colored dots, weaving in and out of sync with each other
  curhue = thishue; // Reset the hue values.
  fadeToBlackBy(leds, NUM_LEDS, faderate);
  for ( int i = 0; i < numdots; i++) {
    //beat16 is a FastLED 3.1 function
    leds[beatsin16(basebeat + i + numdots, 0, NUM_LEDS)] += CHSV(gHue + curhue, thissat, thisbright);
    curhue += hueinc;
  }

  return 8;
}


uint8_t twinkleFox(uint8_t defaultSpeed = 6, uint8_t defaultDensity = 1) {
  
  EVERY_N_MILLISECONDS( 10 ) {
    nblendPaletteTowardPalette( currentTwinklePalette, targetTwinklePalette, 12);
  }

  static int tspeed = defaultSpeed;
  static int tdensity = defaultDensity;
  
  EVERY_N_SECONDS(10) { 
    chooseNextColorPalette( targetTwinklePalette );
    
    tspeed = max((tspeed + 1) % 9, 6);
    tdensity = max((tdensity + 1) % 9, 1); 
    PRINTX("Speed:", tspeed); 
    PRINTX("Density:", tdensity); 
  }
  drawTwinkles(tspeed, tdensity);

  return NO_DELAY;
}


/*
   Drop Animations
*/

uint8_t aboutToDropBPM = 1; 
uint8_t aboutToDropDots = 255;

void initDropAnimations() { 
  // reset 
  aboutToDropBPM = 1;
  aboutToDropDots = 30; 
}

// TODO Placeholder animation. Need real progress bar action
// TODO Maybe try CRGB HeatColor(uint8_t temperature) with a rising temp
// or focus on the middle and expand from there
uint8_t aboutToDrop(uint8_t a, uint8_t b) {

  // increase BPM
  EVERY_N_MILLISECONDS(500) { aboutToDropDots = min(aboutToDropDots+1, 100); PRINTX("dots:", aboutToDropDots); } 
  EVERY_N_MILLISECONDS(500) { aboutToDropBPM += 1; PRINTX("bpm", aboutToDropBPM);}
  
  return juggle(aboutToDropDots, aboutToDropBPM);
}

uint8_t dropped(uint8_t a, uint8_t b) {
  discostrobe(120, 2); 
  return NO_DELAY;
}


// animation routine wrapper 
uint8_t testPalette(uint8_t secondsPerPalette = 1, uint8_t gradientPalette = 0) { 
  
  static int gradientPaletteIndex = 0; 
  EVERY_N_SECONDS(secondsPerPalette) { 

#ifdef CPT
    if (gradientPalette) { 
      palettes.getGradientPalette(gradientPaletteIndex); 
      gradientPaletteIndex = addmod8(gradientPaletteIndex, 1, palettes.getGradientPaletteCount());
    }
#endif

    palettes.testPalette(leds, NUM_LEDS);
    PRINT("Next palette");
    palettes.moveToNextPalette();
  }

  return STATIC_DELAY; 
}



