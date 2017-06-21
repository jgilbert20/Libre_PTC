#ifndef __LIBREPTC_H
#define __LIBREPTC_H

#include <Arduino.h>

void ptcConfigureClock();
void waitPTCSync();
void startPTCAquire();
void ptcDisableWCOInterrupt();
void ptcDisableEOCInterrupt();

uint16_t ptcGetResult();

void ptcEnable();
void ptcDisable();
void ptcSelectXY( uint16_t x, uint16_t y );
void ptcSetXYEnable(  uint16_t x, uint16_t y  );
void ptcClearXYEnable(  uint16_t x, uint16_t y  );

void ptcSetWCOmode(int arg0);
void ptcSetAccumulation(int arg0);
void ptcSetCompensationCap(uint16_t c);
void ptcSetInternalCap(uint8_t c);
void ptcSetBurstMode(uint8_t mode);

#endif
