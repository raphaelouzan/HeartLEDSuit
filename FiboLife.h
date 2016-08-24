/*
   Fibonacci v3D: https://github.com/evilgeniuslabs/fibonacci-v3d
   Copyright (C) 2014-2016 Jason Coon, Evil Genius Labs

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

class Cell {
  public:
    byte alive = 1;
    byte prev = 1;
    byte hue = 6;
    byte brightness;
};

Cell world[kMatrixWidth][kMatrixHeight];

uint8_t neighbors(uint8_t x, uint8_t y) {
  return (world[(x + 1) % kMatrixWidth][y].prev) +
         (world[x][(y + 1) % kMatrixHeight].prev) +
         (world[(x + kMatrixWidth - 1) % kMatrixWidth][y].prev) +
         (world[x][(y + kMatrixHeight - 1) % kMatrixHeight].prev) +
         (world[(x + 1) % kMatrixWidth][(y + 1) % kMatrixHeight].prev) +
         (world[(x + kMatrixWidth - 1) % kMatrixWidth][(y + 1) % kMatrixHeight].prev) +
         (world[(x + kMatrixWidth - 1) % kMatrixWidth][(y + kMatrixHeight - 1) % kMatrixHeight].prev) +
         (world[(x + 1) % kMatrixWidth][(y + kMatrixHeight - 1) % kMatrixHeight].prev);
}

void randomFillWorld() {
  static uint8_t lifeDensity = 10;

  for (uint8_t i = 0; i < kMatrixWidth; i++) {
    for (uint8_t j = 0; j < kMatrixHeight; j++) {
      if (random(100) < lifeDensity) {
        world[i][j].alive = 1;
        world[i][j].brightness = 255;
      }
      else {
        world[i][j].alive = 0;
        world[i][j].brightness = 0;
      }
      world[i][j].prev = world[i][j].alive;
      world[i][j].hue = 0;
    }
  }
}

uint8_t life() {
  static uint8_t generation = 0;

  // Display current generation
  for (uint8_t i = 0; i < kMatrixWidth; i++)
  {
    for (uint8_t j = 0; j < kMatrixHeight; j++)
    {
      setPixelXY(i, j, ColorFromPalette(palettes.getPalette(), world[i][j].hue * 4, world[i][j].brightness, LINEARBLEND));
    }
  }

  uint8_t liveCells = 0;

  // Birth and death cycle
  for (uint8_t x = 0; x < kMatrixWidth; x++)
  {
    for (uint8_t y = 0; y < kMatrixHeight; y++)
    {
      // Default is for cell to stay the same
      if (world[x][y].brightness > 0 && world[x][y].prev == 0)
        world[x][y].brightness *= 0.5;

      uint8_t count = neighbors(x, y);

      if (count == 3 && world[x][y].prev == 0)
      {
        // A new cell is born
        world[x][y].alive = 1;
        world[x][y].hue += 2;
        world[x][y].brightness = 255;
      }
      else if ((count < 2 || count > 3) && world[x][y].prev == 1)
      {
        // Cell dies
        world[x][y].alive = 0;
      }

      if (world[x][y].alive)
        liveCells++;
    }
  }

  // Copy next generation into place
  for (uint8_t x = 0; x < kMatrixWidth; x++)
  {
    for (uint8_t y = 0; y < kMatrixHeight; y++)
    {
      world[x][y].prev = world[x][y].alive;
    }
  }

  if (liveCells < 4 || generation >= 128)
  {
    fill_solid(leds, NUM_LEDS, CRGB::Black);

    randomFillWorld();

    generation = 0;
  }
  else
  {
    generation++;
  }

  return 60;
}

void heatMap(CRGBPalette16 palette, bool up)
{
  fill_solid(leds, NUM_LEDS, CRGB::Black);

  // Add entropy to random number generator; we use a lot of it.
  random16_add_entropy(random(256));

  // COOLING: How much does the air cool as it rises?
  // Less cooling = taller flames.  More cooling = shorter flames.
  // Default 55, suggested range 20-100
  uint8_t cooling = 55;

  // SPARKING: What chance (out of 255) is there that a new spark will be lit?
  // Higher chance = more roaring fire.  Lower chance = more flickery fire.
  // Default 120, suggested range 50-200.
  uint8_t sparking = 120;

  // Array of temperature readings at each simulation cell
  static byte heat[kMatrixWidth + 3][kMatrixHeight + 3];

  for (int x = 0; x < 10; x++)
  {
    // Step 1.  Cool down every cell a little
    for (int y = 0; y < 10; y++)
    {
      heat[x][y] = qsub8(heat[x][y], random8(0, ((cooling * 10) / kMatrixHeight) + 2));
    }

    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for (int y = 0; y < kMatrixHeight; y++)
    {
      heat[x][y] = (heat[x][y + 1] + heat[x][y + 2] + heat[x][y + 2]) / 3;
    }

    // Step 2.  Randomly ignite new 'sparks' of heat
    if (random8() < sparking)
    {
      heat[x][maxY] = qadd8(heat[x][maxY], random8(160, 255));
    }

    // Step 4.  Map from heat cells to LED colors
    for (int y = 0; y < kMatrixHeight; y++)
    {
      uint8_t colorIndex = 0;

      if (up)
        colorIndex = heat[x][y];
      else
        colorIndex = heat[x][(maxY) - y];

      // Recommend that you use values 0-240 rather than
      // the usual 0-255, as the last 15 colors will be
      // 'wrapping around' from the hot end to the cold end,
      // which looks wrong.
      colorIndex = scale8(colorIndex, 240);

      // override color 0 to ensure a black background
      if (colorIndex != 0)
      {
        setPixelXY10(x, y, ColorFromPalette(palette, colorIndex, 255, LINEARBLEND));
      }
    }
  }
}

uint8_t fire() {
  heatMap(HeatColors_p, true);

  return 30;
}

uint8_t wave() {
  const uint8_t scale = 256 / kMatrixWidth;

  static uint8_t rotation = 0;
  static uint8_t theta = 0;
  static uint8_t waveCount = 1;

  uint8_t n = 0;

  CRGBPalette16 currentPalette = palettes.getPalette();
  
  switch (rotation) {
    case 0:
      for (int x = 0; x < kMatrixWidth; x++) {
        n = quadwave8(x * 2 + theta) / scale;
        setPixelXY(x, n, ColorFromPalette(currentPalette, x + gHue, 255, LINEARBLEND));
        if (waveCount == 2)
          setPixelXY(x, maxY - n, ColorFromPalette(currentPalette, x + gHue, 255, LINEARBLEND));
      }
      break;

    case 1:
      for (int y = 0; y < kMatrixHeight; y++) {
        n = quadwave8(y * 2 + theta) / scale;
        setPixelXY(n, y, ColorFromPalette(currentPalette, y + gHue, 255, LINEARBLEND));
        if (waveCount == 2)
          setPixelXY(maxX - n, y, ColorFromPalette(currentPalette, y + gHue, 255, LINEARBLEND));
      }
      break;

    case 2:
      for (int x = 0; x < kMatrixWidth; x++) {
        n = quadwave8(x * 2 - theta) / scale;
        setPixelXY(x, n, ColorFromPalette(currentPalette, x + gHue));
        if (waveCount == 2)
          setPixelXY(x, maxY - n, ColorFromPalette(currentPalette, x + gHue, 255, LINEARBLEND));
      }
      break;

    case 3:
      for (int y = 0; y < kMatrixHeight; y++) {
        n = quadwave8(y * 2 - theta) / scale;
        setPixelXY(n, y, ColorFromPalette(currentPalette, y + gHue, 255, LINEARBLEND));
        if (waveCount == 2)
          setPixelXY(maxX - n, y, ColorFromPalette(currentPalette, y + gHue, 255, LINEARBLEND));
      }
      break;
  }

  dimAll(255);

  EVERY_N_SECONDS(10)
  {
    rotation = random(0, 4);
    // waveCount = random(1, 3);
  };

  EVERY_N_MILLISECONDS(7) {
    theta++;
  }

  return 8;
}



uint8_t pulse() {
  dimAll(200);

  uint8_t maxSteps = 16;
  static uint8_t step = maxSteps;
  static uint8_t centerX = 0;
  static uint8_t centerY = 0;
  float fadeRate = 0.8;

  if (step >= maxSteps)
  {
    centerX = random(kMatrixWidth);
    centerY = random(kMatrixWidth);
    step = 0;
  }

  if (step == 0)
  {
    drawCircle(centerX, centerY, step, ColorFromPalette(palettes.getPalette(), gHue, 255, LINEARBLEND));
    step++;
  }
  else
  {
    if (step < maxSteps)
    {
      // initial pulse
      drawCircle(centerX, centerY, step, ColorFromPalette(palettes.getPalette(), gHue, pow(fadeRate, step - 2) * 255, LINEARBLEND));

      // secondary pulse
      if (step > 3) {
        drawCircle(centerX, centerY, step - 3, ColorFromPalette(palettes.getPalette(), gHue, pow(fadeRate, step - 2) * 255, LINEARBLEND));
      }

      step++;
    }
    else
    {
      step = -1;
    }
  }

  return 30;
}



// ColorWavesWithPalettes by Mark Kriegsman: https://gist.github.com/kriegsman/8281905786e8b2632aeb
// This function draws color waves with an ever-changing,
// widely-varying set of parameters, using a color palette.
void colorwaves( CRGB* ledarray, uint16_t numleds, CRGBPalette16& palette, bool useFibonacciOrder)
{
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;

  // uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16;//gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 300, 1500);

  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis ;
  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88( 400, 5, 9);
  uint16_t brightnesstheta16 = sPseudotime;

  for ( uint16_t i = 0 ; i < numleds; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;
    uint16_t h16_128 = hue16 >> 7;
    if ( h16_128 & 0x100) {
      hue8 = 255 - (h16_128 >> 1);
    } else {
      hue8 = h16_128 >> 1;
    }

    brightnesstheta16  += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);

    uint8_t index = hue8;
    //index = triwave8( index);
    index = scale8( index, 240);

    CRGB newcolor = ColorFromPalette( palette, index, bri8);

    uint16_t pixelnumber = i;


    if (useFibonacciOrder) {
      pixelnumber = fibonacciToPhysicalOrder[(numleds - 1) - pixelnumber];
    }

    nblend( ledarray[pixelnumber], newcolor, 128);
  }
}

uint8_t radialPaletteShift() {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    // leds[i] = ColorFromPalette( currentPalette, gHue + sin8(i*16), brightness);
    uint8_t index = fibonacciToPhysicalOrder[(NUM_LEDS - 1) - i];

    leds[index] = ColorFromPalette(palettes.getGradientPalette(), i + gHue, 255, LINEARBLEND);
  }

  return 8;
}

uint8_t incrementalDrift() {
  uint8_t stepwidth = 256 * (20 - 1) / NUM_LEDS;
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    uint8_t bri = beatsin88(1 * 256 + (NUM_LEDS - i) * stepwidth, 0, 256);
    leds[fibonacciToPhysicalOrder[i]] = ColorFromPalette(palettes.getGradientPalette(), 2.5 * i + gHue, bri, LINEARBLEND);
  }

  return 8;
}

CRGB scrollingVerticalWashColor( uint8_t x, uint8_t y, unsigned long timeInMillis)
{
  return CHSV( y + (timeInMillis / 10), 255, 255);
}


uint8_t verticalRainbow()
{
  unsigned long t = millis();

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    uint8_t j = physicalToFibonacciOrder[i];
    leds[i] = scrollingVerticalWashColor(coordsX[j], coordsY[j], t);
  }

  return 8;
}

/*
    Animations ready for HeartLEDSuit
*/
CRGBPalette16 IceColors_p = CRGBPalette16(CRGB::Black, CRGB::Blue, CRGB::Aqua, CRGB::White);

uint8_t colorWaves(uint8_t useFibonacci, uint8_t dummy) {
  colorwaves(leds, NUM_LEDS, palettes.getGradientPalette(), useFibonacci ? true : false);
  return 20;
}

// way too fast
uint8_t life(uint8_t dummy, uint8_t dummy2) {
  return life();
}

uint8_t pulse(uint8_t dummy, uint8_t dummy2) { 
  return pulse() * 4;
}

uint8_t wave(uint8_t dummy, uint8_t dummy2) { 

  // TODO Should got a bit slower // Try different delays
  return wave(); 
}

uint8_t incrementalDrift(uint8_t dummy, uint8_t dummy2) { 
  return incrementalDrift();
}

uint8_t verticalRainbow(uint8_t dummy, uint8_t dummy2) { 
  return verticalRainbow();
}

uint8_t radialPaletteShift(uint8_t dummy, uint8_t dummy2) { 
  return radialPaletteShift();
}
uint8_t fiboFire(uint8_t dummy, uint8_t dummy2) { 
  heatMap(HeatColors_p, true);  
  return RANDOM_DELAY;
}

// Way too fast
uint8_t water(uint8_t dummy, uint8_t dummy2) { 
  heatMap(IceColors_p, false); 
  return SYNCED_DELAY; 
}



