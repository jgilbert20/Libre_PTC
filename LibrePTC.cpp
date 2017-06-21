
#include <LibrePTC.h>


// Sets up the clocks needed for the PTC module.
// The clock setup is described in the touch.c implementation in the ASF examples
// I've replaced the ASF calls with their low-level equivilents
// Funny enough, the Generic Clock Generator 3 is not actually used by arduinio core
// although it is set up in various places.

void ptcConfigureClock()
{
  const unsigned int PTC_CLOCK_GCGID = 3;

  // Added for PTC - rough hack - JAG

  SYSCTRL->OSC8M.bit.PRESC = SYSCTRL_OSC8M_PRESC_2_Val ;  // recent versions of CMSIS from Atmel changed the prescaler defines
  SYSCTRL->OSC8M.bit.ONDEMAND = 1 ;
  //  SYSCTRL->OSC8M.bit.RUNINSTANDBY = 1 ;

  /* Put OSC8M as source for Generic Clock Generator 3 */
  GCLK->GENDIV.reg = GCLK_GENDIV_ID( PTC_CLOCK_GCGID) ; // Generic Clock Generator 3
  while ( (SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLRDY) == 0 );

  // This magic actually does the assignment
  GCLK->GENCTRL.reg =
    ( GCLK_GENCTRL_ID( PTC_CLOCK_GCGID ) |
      GCLK_GENCTRL_SRC_OSC8M
      | GCLK_GENCTRL_GENEN
    );
  while ( (SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLRDY) == 0 );

  // Original ASF code

  // const int GCLK_GENERATOR_3 = 3;
  // struct system_gclk_chan_config gclk_chan_conf;

  // system_gclk_chan_get_config_defaults(&gclk_chan_conf);
  // gclk_chan_conf->source_generator = GCLK_GENERATOR_0;
  // equivilent to:
  //  gclk_chan_conf.source_generator = GCLK_GENERATOR_3;

  //  system_gclk_chan_set_config(PTC_GCLK_ID, &gclk_chan_conf);
  // equivalent to
  {
    uint32_t new_clkctrl_config = (PTC_GCLK_ID << GCLK_CLKCTRL_ID_Pos);
    new_clkctrl_config |= PTC_CLOCK_GCGID << GCLK_CLKCTRL_GEN_Pos;

    //  system_gclk_chan_disable(channel);
    // this is the "disable" function
    {
      /* Select the requested generator channel */
      *((uint8_t*)&GCLK->CLKCTRL.reg) = PTC_CLOCK_GCGID;

      /* Switch to known-working source so that the channel can be disabled */
      uint32_t prev_gen_id = GCLK->CLKCTRL.bit.GEN;
      GCLK->CLKCTRL.bit.GEN = 0;

      /* Disable the generic clock */
      GCLK->CLKCTRL.reg &= ~GCLK_CLKCTRL_CLKEN;
      while (GCLK->CLKCTRL.reg & GCLK_CLKCTRL_CLKEN) {
        /* Wait for clock to become disabled */
      }

      /* Restore previous configured clock generator */
      GCLK->CLKCTRL.bit.GEN = prev_gen_id;
    }

    /* Write the new configuration */
    GCLK->CLKCTRL.reg = new_clkctrl_config;
  }

  //  system_gclk_chan_enable(PTC_GCLK_ID);
  // equivlent to:
  *((uint8_t*)&GCLK->CLKCTRL.reg) = PTC_GCLK_ID;
  GCLK->CLKCTRL.reg |= GCLK_CLKCTRL_CLKEN;


  // system_apb_clock_set_mask(SYSTEM_CLOCK_APB_APBC, PM_APBCMASK_PTC);
  // equivilant is:
  PM->APBCMASK.reg |= PM_APBCMASK_PTC;
}

// PTC apparently has a clock domain boundary like most other peripherals.
// This helper function will wait for the clocks to synchronize

#define PTC_SYNCBUSY 0x42002c01

void waitPTCSync()
{
  while ( (*(uint8_t *)(PTC_SYNCBUSY) & (~0x7f)) != 0 );
}


#define PTC_AQUISITION_CONTROL 0x42002c0d

// This triggers the acquisition you've configured to start
void startPTCAquire()
{
  waitPTCSync();
  *(uint8_t *)PTC_AQUISITION_CONTROL = *(uint8_t *)PTC_AQUISITION_CONTROL | 0xffffff80;
}


// Disables the WCO interrupt. I can't figure out how the interrupt works
// so there is no method to enable it. Instead we'll use polling.

void ptcDisableWCOInterrupt() {
  waitPTCSync();
  *(uint8_t *)0x42002c08 = *(int8_t *)0x42002c08 | 0x2;
  return;
}



// Disables the interrupt on "end of conversion". I can't figure out how the interrupt works
// so there is no method to enable it. Instead we'll use polling.
void ptcDisableEOCInterrupt() {
  waitPTCSync();
  *(uint8_t *)0x42002c08 = *(int8_t *)0x42002c08 | 0x1;
  return;
}

#define PTC_RESULT 0x42002c1c

// Returns the result of the most recent conversion
// TODO: Wait for some flag to indicate the conversion is done
// Results are a 16bit unsigned integer
uint16_t ptcGetResult() {
  return *(uint16_t *)0x42002c1c;
}

#define PTC_CTRLA 0x42002c00

// Enables the PTC peripheral. PTC seems to work similiar to other peripherals on the SAMD
// and some things cannot be set properly when the PTC is active. What is probably happening
// is that the data is latched from RAM only when this is disabled/re-enabled.
void ptcEnable() {
  waitPTCSync();
  *(int8_t *)PTC_CTRLA = *(int8_t *)PTC_CTRLA | 0x02;
  return;
}

void ptcDisable() {
  waitPTCSync();
  *(int8_t *)PTC_CTRLA = *(int8_t *)PTC_CTRLA & ~0x02;
  return;
}

#define PTC_SELECTY 0x42002c10
#define PTC_SELECTX 0x42002c12

// Appears to select which X and Y line the PTC controller will read
// I am guessing about which is X and which is Y

void ptcSelectXY( uint16_t x, uint16_t y )
{
  waitPTCSync();
  *(uint16_t *)PTC_SELECTY = y;
  waitPTCSync();
  *(uint16_t *)PTC_SELECTX = x;
}


// seems to be something used to configure which pins will be
// charged or discharged? This is apparently a bitfield

void ptcSetXYEnable(  uint16_t x, uint16_t y  )
{
  ptcDisable();

  waitPTCSync();
  *(int16_t *)0x42002c16 = *(int16_t *)0x42002c16 | x;
  waitPTCSync();
  *(int16_t *)0x42002c14 = *(int16_t *)0x42002c14 | y;

  ptcEnable();
}


// seems to be something used to configure which pins will be
// charged or discharged

void ptcClearXYEnable(  uint16_t x, uint16_t y  )
{
  ptcDisable();

  waitPTCSync();
  *(int16_t *)0x42002c16 = *(int16_t *)0x42002c16 & ~x;
  waitPTCSync();
  *(int16_t *)0x42002c14 = *(int16_t *)0x42002c14 & ~y;

  ptcEnable();
}

// Don't know what WCO mode does

void ptcSetWCOmode(int arg0) {  waitPTCSync();
  *(uint8_t *)0x42002c21 = (arg0 & 0x07) | (*(uint8_t *)0x42002c21 & ~0x07);
  return;
}



// Sets the numbers of accumulations.. Guessing this works like the ADC.
// apparently the accumulation can be between 0..7.
// in inspecting the setup left by the official library example reading a Y line
// , I found mine was set to 4
//

void ptcSetAccumulation(int arg0) {
  waitPTCSync();
  *(uint8_t *)0x42002c0d = arg0 & 0x7 | (*(uint8_t *)0x42002c0d & ~0x07);

}


// In practice appears to be 0x1418. No idea where this comes from!
void ptcSetCompensationCap(uint16_t c) {
  waitPTCSync();
  *(uint16_t *)0x42002c18 = c;

}

// In practice mine was set to 0x3f, but not sure why
void ptcSetInternalCap(uint8_t c) {
  waitPTCSync();
  *(uint16_t *)0x42002c1A = c;

}

// Burst mode. Does this mean it automatically repeats and averages?
//
// apparently you get to specify a number 0..15
// This lives in the high order word of 0x42002c20
// Most of the time I'm reading a 0xA..? what does that mean?

void ptcSetBurstMode(uint8_t mode) {
  waitPTCSync();
  int lowOrder = *(uint8_t *)0x42002c20 & 0x0f;
  *(uint8_t *)0x42002c20 = lowOrder | (mode & 0x0f) << 4;
}