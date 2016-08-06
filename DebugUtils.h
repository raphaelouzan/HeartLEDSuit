/*
DebugUtils.h - Simple debugging utilities.
*/

#ifndef DEBUGUTILS_H
#define DEBUGUTILS_H

#include <Arduino.h>

#if defined(ARDUINO_SAMD_ZERO) && defined(SERIAL_PORT_USBVIRTUAL)
  // Required for Serial on Zero based boards
  #define Serial SERIAL_PORT_USBVIRTUAL
#endif
  
#ifdef DEBUG

  #define DEBUG_START(baudRate) \
    Serial.begin(baudRate);
  
  #define PRINT_NOLN(str)    \
     Serial.print(millis());     \
     Serial.print(": ");    \
     Serial.print(__PRETTY_FUNCTION__); \
     Serial.print(':');      \
     Serial.print(__LINE__);     \
     Serial.print(' ');      \
     Serial.print(str); 
     
  #define PRINT(str) PRINT_NOLN(str) Serial.println("");
  
  #define PRINTX(str, obj) \
     PRINT_NOLN(str) \
     Serial.print(" "); \
     Serial.println(obj);

  extern "C" char *sbrk(int i);

  int freeRam () {
    char stack_dummy = 0;
    return &stack_dummy - sbrk(0);
  }

  #define VBATPIN A7
   
  float batteryLevel() { 
    float measuredvbat = analogRead(VBATPIN);
    measuredvbat *= 2;    // we divided by 2, so multiply back
    measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
    measuredvbat /= 1024; // convert to voltage
    return measuredvbat; 
  }
  
     
#else

  #define PRINT(str)
  #define PRINT_NOLN(str)
  #define PRINTX(str, obj)
  #define DEBUG_START(baudRate)

#endif

#endif
