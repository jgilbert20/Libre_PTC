#include <LibrePTC.h>

void setup() {
  // put your setup code here, to run once:
  Serial.begin( 9600 );
  Serial.println( "Welcome to something miraculous!" );

  ptcConfigureClock();

  Serial.println( "Clock done" );

  //   from the config stuff that appears to clear out things
  // when loaded

  ptcDisable();
  *(int8_t *)0x42002c04 = *(int8_t *)0x42002c04 & ~0x8;
  *(int8_t *)0x42002c04 = *(int8_t *)0x42002c04 & ~0x4;
  *(int8_t *)0x42002c04 = *(int8_t *)0x42002c04 & ~0x3;
  waitPTCSync();
  *(int8_t *)0x42002c0c = *(int8_t *)0x42002c0c & ~0x60;
  waitPTCSync();
  *(int8_t *)0x42002c0c = *(int8_t *)0x42002c0c & ~0x10;
  waitPTCSync();
  *(int8_t *)0x42002c0c = *(int8_t *)0x42002c0c & ~0xf;
  *(int8_t *)0x42002c05 = *(int8_t *)0x42002c05 | 0x1;
  // *(int8_t *)0x42002c00 = *(int8_t *)0x42002c00 | 0x4;
  waitPTCSync();
  *(int8_t *)0x42002c00 = *(int8_t *)0x42002c00 | 0x2;

  ptcEnable();

  ptcSetAccumulation(4);
  ptcSetCompensationCap( 0x1418 ); // unclear what this really sets
  ptcSetInternalCap( 0x3f );
  ptcSetBurstMode( 0xA );

  // load prescaler. Leaving at its default settings

  if (0) {
    ptcDisable();

    waitPTCSync();
    *(uint16_t *)0x42002c04 = 0x04;

    ptcEnable();
  }

  // Don't know what WCO mode does
  // Should set to 0x04
  ptcSetWCOmode( 0x04) ;

}

void loop() {

  // Configure the pins to read to route to the "B" peripheral mux. This actually also
  // includes the ADC so we can re-use the ArduinoCore. Empirically, it appears you can
  // also just use a pinMode( 2, INPUT ) to accomplish the same end.

  pinMode( 2, PIO_ANALOG_ADC );
  pinMode( 3, PIO_ANALOG_ADC );

  ptcDisableWCOInterrupt();
  ptcDisableEOCInterrupt();

  ptcSelectXY( 0, 2 );
  ptcSetXYEnable( 0, B111 );

  while (1)
  {

    startPTCAquire();

    for (int i = 0 ; i < 10 ; i++ )
    {
      waitPTCSync();
      uint16_t r = ptcGetResult();
      Serial.println( r );
    }
  }
}