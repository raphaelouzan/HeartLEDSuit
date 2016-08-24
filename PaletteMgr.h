#ifndef PALETTE_MGR
#define PALETTE_MGR

#include <FastLED.h>
#include "DebugUtils.h"

// make it based on a vector/linkedlist 
// include palettes from TwinkleFox and make TwinkleFox use the manager
// include CPC palettes

DEFINE_GRADIENT_PALETTE( gr64_hult_gp ) {
    0,   1,124,109,
   66,   1, 93, 79,
  104,  52, 65,  1,
  130, 115,127,  1,
  150,  52, 65,  1,
  201,   1, 86, 72,
  239,   0, 55, 45,
  255,   0, 55, 255}; 



// A mostly (dark) green palette with red berries.
#define Holly_Green 0x00580c
#define Holly_Red   0xB00402
const TProgmemRGBPalette16 Holly_p2 FL_PROGMEM =
{ Holly_Green, Holly_Green, Holly_Green, Holly_Green,
  Holly_Green, Holly_Green, Holly_Green, Holly_Green,
  Holly_Green, Holly_Green, Holly_Green, Holly_Green,
  Holly_Green, Holly_Green, Holly_Green, Holly_Red
};

// A red and white striped palette
// "CRGB::Gray" is used as white to keep the brightness more uniform.
const TProgmemRGBPalette16 RedWhite_p2 FL_PROGMEM =
{ CRGB::Red,  CRGB::Red,  CRGB::Red,  CRGB::Red,
  CRGB::Gray, CRGB::Gray, CRGB::Gray, CRGB::Gray,
  CRGB::Red,  CRGB::Red,  CRGB::Red,  CRGB::Red,
  CRGB::Gray, CRGB::Gray, CRGB::Gray, CRGB::Gray
};

// should be const
CRGBPalette16 _palettes2[] = {
  Holly_p2,
  RedWhite_p2
};

CRGBPalette16 _palettes[] = {RainbowColors_p, RainbowStripeColors_p, LavaColors_p, HeatColors_p,
                             CloudColors_p, OceanColors_p, ForestColors_p, PartyColors_p
                            };
                            
class PaletteMgr {

  public:

    PaletteMgr() : _paletteIndex(0), 
                   _paletteCount(sizeof(_palettes) / sizeof(_palettes[0])) {
                              
      _currentPalette = &_palettes[0]; 
      _targetPalette = &_palettes[getNextPaletteIndex()];
    }

    void blendPalettes() {
      nblendPaletteTowardPalette(*_currentPalette, *_targetPalette, 16);
    }

    void queueNextPalette() {
      _paletteIndex = getNextPaletteIndex();
      _targetPalette = &_palettes[_paletteIndex];
      PRINTX("Queueing to palette: ", _paletteIndex);
    }

    void moveToNextPalette() { 
      _currentPalette = &_palettes[getNextPaletteIndex()];  
    }


    CRGBPalette16& getPalette() {
      return *_currentPalette;
    }

    // stub
    CRGBPalette16& getGradientPalette() { 
      return getPalette(); 
    }


  private:

    uint8_t getNextPaletteIndex() {
      return addmod8(_paletteIndex, 1, _paletteCount);
    }


    CRGBPalette16* _currentPalette;
    CRGBPalette16* _targetPalette;
    uint8_t _paletteIndex; 
    uint8_t _paletteCount;
};



#endif
