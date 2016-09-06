#include <FastLED.h>

/*
soundPulse, paletteDance and and glitter are largely inspired by: 
https://github.com/bartlettmic/SparkFun-RGB-LED-Music-Sound-Visualizer-Arduino-Code/blob/master/Visualizer_Program/Visualizer_Program.ino
*/

#define DC_OFFSET  0                                         // DC offset in mic signal - if unusure, leave 0
                                                              // I calculated this value by serialprintln lots of mic values
#define NOISE     10                                         // Noise/hum/interference in mic signal and increased value until it went quiet
#define SAMPLES   60                                          // Length of buffer for dynamic level adjustment
#define TOP (NUM_LEDS + 2)                                    // Allow dot to go slightly off scale
#define PEAK_FALL 4                                          // Rate of peak falling dot

byte
  peak      = 0,                                              // Used for falling dot
  dotCount  = 0,                                              // Frame counter for delaying dot-falling speed
  volCount  = 0;                                              // Frame counter for storing past volume data
  int
  vol[SAMPLES],                                               // Collection of prior volume samples
  lvl       = 10,                                             // Current "dampened" audio level
  minLvlAvg = 0,                                              // For dynamic adjustment of graph low & high
  maxLvlAvg = 512;
  
  int centerPoint = 15;  

// from VU anims
  bool bump = false;
  int bumpCount = 0;
  float avgBump = 0;
  float avgBumpTime = 0;
  float timeSinceLastBump = 0;
  
  uint8_t volume = 0;
  uint8_t lastVolume = 0;
  float avgVol = 0;
  float maxVol = 15;

  bool left = false;

  int8_t dotPos = 15; 
  uint16_t gradient = 1;
  CRGB rgb[NUM_LEDS] = {(0, 0, 0)};

#define HALF_LEDS           NUM_LEDS/2
#define NUM_SOUNDANIMATIONS 5

void updateBumps(int height) {

 volume = height; 

 if (volume) { 
  avgVol = (avgVol + volume) / 2.0; 

  if (volume > maxVol) maxVol = volume; 
  }

  avgVol = (avgVol + volume) / 2.0;

  
  if (volume - lastVolume > 10) avgBump = (avgBump + (volume - lastVolume)) / 2.0;
  bump = (volume - lastVolume > avgBump * 0.9);

  if (gradient > 255) {
    gradient %= 256;
    maxVol = (maxVol + volume) / 2.0;
  }

  if (bump) {
    // Add overflow protection here 
    bumpCount++;
    PRINTX("bump!: ", String(bumpCount));
    avgBumpTime = (((millis() / 1000.0) - timeSinceLastBump) + avgBumpTime) / 2.0;
    timeSinceLastBump = millis() / 1000.0;
    PRINTX("avgtime: ", String(avgBumpTime));
  }

  gradient++; 

  lastVolume = volume;
}

void bleed(uint8_t point) {
  for (int i = 1; i < NUM_LEDS; i++) {
    int sides[] = {point - i, point + i};
    for (int i = 0; i < 2; i++) {
      int point = sides[i];
      if (point < NUM_LEDS - 1 && point > 1) {
        leds[point].r = float((leds[point - 1].r + leds[point].r + leds[point + 1].r ) / 3.0);
        leds[point].g = float((leds[point - 1].g + leds[point].g + leds[point + 1].g ) / 3.0);
        leds[point].b = float((leds[point - 1].b + leds[point].b + leds[point + 1].b ) / 3.0);
      }
    }
  }
}

void soundPulse() {

  fadeLightBy(leds, NUM_LEDS, 48);

  if (bump) gradient += 7;

  if (volume > 0) {

    CRGB col = ColorFromPalette(palettes.getPalette(), gradient);
    int start = HALF_LEDS - (HALF_LEDS * (volume / maxVol));
    int finish = HALF_LEDS + (HALF_LEDS * (volume / maxVol)) + NUM_LEDS % 2;

    for (int i = start; i < finish; i++) {

      float damp = sin((i - start) * PI / float(finish - start));
      damp = pow(damp, 2.0);

      CRGB col2 = leds[i];
      CRGB color;

      color.r = col.r * damp * pow(volume / maxVol, 2);
      color.g  = col.g * damp * pow(volume / maxVol, 2);
      color.b = col.b * damp * pow(volume / maxVol, 2);

      float avgCol = (color.r + color.g + color.b) / 3.0;
      float avgCol2 = (col2.r + col2.g + col2.b) / 3.0;

      if (avgCol > avgCol2) leds[i] = color;
    }

  }
}

void paintball() {

  if ((millis() / 1000.0) - timeSinceLastBump > avgBumpTime * 2.0) fadeToBlackBy(leds, NUM_LEDS, 4);
  bleed(dotPos);
  int fadeAmount = map(pow(volume / maxVol, 2.0) * 100, 0, 100, 0, 255);
  if (bump) {
    randomSeed(micros());
    dotPos = random(0, NUM_LEDS - 1);
    CRGB dotCol = ColorFromPalette(palettes.getPalette(), random(0, 255));
    leds[dotPos] = dotCol;
    leds[dotPos].nscale8_video(fadeAmount);
    blur1d(leds, NUM_LEDS, 74);
  }
}

void paletteDance() { 

  if (bump) left = !left;

  if (volume > avgVol) {
    for (int i = 0; i < NUM_LEDS; i++) {
      float sinVal = abs(sin((i + dotPos) * (PI / float(NUM_LEDS / 1.25))));
      sinVal *= sinVal;
      sinVal *= volume / maxVol;

      unsigned int val = 256
      * (float(i + map(dotPos, -1 * (NUM_LEDS - 1), NUM_LEDS - 1, 0, NUM_LEDS - 1))
        / float(NUM_LEDS))
      + (gHue);
      val %= 256;
      CRGB col = ColorFromPalette(palettes.getPalette(), val);
      leds[i].r = col.r * sinVal;
      leds[i].g = col.g * sinVal;
      leds[i].b = col.b * sinVal;
    }
    dotPos += (left) ? -1 : 1;
  }
  else  fadeLightBy(leds, NUM_LEDS, 16);

  if (dotPos < 0) dotPos = NUM_LEDS - NUM_LEDS / 6;
  else if (dotPos >= NUM_LEDS - NUM_LEDS / 6)  dotPos = 0;
}

void glitter() {

  gradient += 4;
  for (int i = 0; i < NUM_LEDS; i++) {
    unsigned int val = 256.0 *
    (float(i) / float(NUM_LEDS))
    + (gradient);
    val %= 255;
    CRGB  col = ColorFromPalette(palettes.getPalette(), val);
    leds[i].r = col.r / 6.0,
    leds[i].g = col.g / 6.0,
    leds[i].b = col.b / 6.0;
  }
  if (bump) {
    randomSeed(micros());
    dotPos = random(NUM_LEDS - 1);
    leds[dotPos].r = 255.0 * pow(volume / maxVol, 2.0),
    leds[dotPos].g = 255.0 * pow(volume / maxVol, 2.0),
    leds[dotPos].b = 255.0 * pow(volume / maxVol, 2.0);
  }
  bleed(dotPos);
}

void snake() {

  if (bump) {
    gradient += 4;
    left = !left;
  }

  fadeLightBy(leds, NUM_LEDS, 4);

  CRGB col = ColorFromPalette(palettes.getPalette(), gradient);

  if (volume > 0) {

    int fadeAmount = map(pow(volume / maxVol, 1.5) * 100, 0, 100, 0, 255);
    leds[dotPos] = col;
    leds[dotPos].nscale8_video(fadeAmount);

    if (avgBumpTime < 0.15)                                               dotPos += (left) ? -1 : 1;
    else if (avgBumpTime >= 0.15 && avgBumpTime < 0.5 && gradient % 2 == 0)   dotPos += (left) ? -1 : 1;
    else if (avgBumpTime >= 0.5 && avgBumpTime < 1.0 && gradient % 3 == 0)    dotPos += (left) ? -1 : 1;
    else if (gradient % 4 == 0)                                       dotPos += (left) ? -1 : 1;
  }

  if (dotPos < 0) dotPos = NUM_LEDS - 1;
  else if (dotPos >= NUM_LEDS)  dotPos = 0;
}


void randomVU(int height) {

  for (int i = 0; i < NUM_LEDS; i++) {
    int distanceFromCenter = abs(centerPoint - i);
    if (distanceFromCenter >= (height/2)) {
      leds[i].setRGB(0, 0, 0);
    } else {
        //leds[i].setRGB(255, 0, 0);
      leds[i] = ColorFromPalette(palettes.getPalette(), 
        90 + map(distanceFromCenter, 0, NUM_LEDS-1, 0, 255), 100, LINEARBLEND);
    }
  }
    //move center point randomly
  if ((height == 0) && (random(2) == 1)) {
    centerPoint = random(NUM_LEDS);
  }
}

void baseVU(int height) { 
  // Color pixels based on rainbow gradient
  for (int i = 0; i < NUM_LEDS; i++) {
    if (i >= height)  leds[i].setRGB(0, 0, 0);
    else leds[i] = ColorFromPalette(palettes.getPalette(), 
      90 + map(i, 0, NUM_LEDS-1, 0, 255), 100, LINEARBLEND);
  }

  if (peak > 0 && peak <= NUM_LEDS-1) leds[peak] = CHSV(map(peak,0,NUM_LEDS-1,30,150), 255, 255);

    // Every few frames, make the peak pixel drop by 1:
  if (++dotCount >= PEAK_FALL) {                            // fall rate 
    if(peak > 0) peak--;
    dotCount = 0;
  }
}

/*
  animIndex = animation to play if nextAnimTimeout is 0
  nextAnimTimeout = number of seconds to wait before advancing to the next animation (0 to stay)
*/
uint8_t soundAnimate(uint8_t animIndex, uint8_t nextAnimTimeout = 0) {

  uint8_t  i;
  uint16_t minLvl, maxLvl;
  int      n, height;

  n = analogRead(MIC_PIN);                                    // Raw reading from mic
  n = abs(n - 512 - DC_OFFSET);                               // Center on zero
  
  n = (n <= NOISE) ? 0 : (n - NOISE);                         // Remove noise/hum
  lvl = ((lvl * 7) + n) >> 3;                                 // "Dampened" reading (else looks twitchy)

  // Calculate bar height based on dynamic min/max levels (fixed point):
  height = TOP * (lvl - minLvlAvg) / (long)(maxLvlAvg - minLvlAvg);

  if (height < 0L)       height = 0;                          // Clip output
  else if (height > TOP) height = TOP;
  if (height > peak)     peak   = height;                     // Keep 'peak' dot at top

  static uint8_t autoQueueIndex = 0;

  if (nextAnimTimeout != 0) { 
    EVERY_N_SECONDS(nextAnimTimeout) { 

      autoQueueIndex = addmod8(autoQueueIndex, 1, NUM_SOUNDANIMATIONS); 
      PRINTX("Move to next animation", autoQueueIndex);

      // Give palette dance a random starting position 
      if (autoQueueIndex == 3) dotPos = random(NUM_LEDS);
      
      // Reset for fresh experience
      maxVol = avgVol; 
      avgBump = 0;
      bumpCount = 0; 
      avgBumpTime = 0; 
     }
   } else { 
      PRINT("ALERT: NO SOUND INTERNAL TIMER!!")
      autoQueueIndex = animIndex; 
   }

  // run custom sampling for SparkFun based animations
  if (autoQueueIndex > 1) updateBumps(n); 

   if (autoQueueIndex == 0) {
    // VU from the base
    baseVU(height);
  } else if (autoQueueIndex == 1) {
    // random position VU
    randomVU(height); 
  } else if (autoQueueIndex == 2) { 
    soundPulse();
  } else if (autoQueueIndex == 3) { 
    paletteDance();
  } else if (autoQueueIndex == 4) { 
    glitter();
  }

  vol[volCount] = n;                                          // Save sample for dynamic leveling
  if (++volCount >= SAMPLES) volCount = 0;                    // Advance/rollover sample counter

  // Get volume range of prior frames
  minLvl = maxLvl = vol[0];
  for (i=1; i< SAMPLES; i++) {
    if (vol[i] < minLvl)      minLvl = vol[i];
    else if (vol[i] > maxLvl) maxLvl = vol[i];
  }
  // minLvl and maxLvl indicate the volume range over prior frames, used
  // for vertically scaling the output graph (so it looks interesting
  // regardless of volume level).  If they're too close together though
  // (e.g. at very low volume levels) the graph becomes super coarse
  // and 'jumpy'...so keep some minimum distance between them (this
  // also lets the graph go to zero when no sound is playing):
  if((maxLvl - minLvl) < TOP) maxLvl = minLvl + TOP;
  minLvlAvg = (minLvlAvg * 63 + minLvl) >> 6;                 // Dampen min/max levels
  maxLvlAvg = (maxLvlAvg * 63 + maxLvl) >> 6;                 // (fake rolling average)
  
  return NO_DELAY;
}
