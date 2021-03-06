
#pragma once
/**
 Pin usage
 * 
 * PC13/14/15 OUTPUT LCD control 
 * PB0--PB7   OUTPUT LCD Databus
 * PA0        x      ADCIN
 * PA1..PA3   x      Gain 2nd stage SENSEL0..SENSEL2
 * PA4        x      Gain 1st stage SENSEL3
 * PA5        x      CPLSEL (DC/AC/GND)
 * PA6        OUTPUT LCD nRD
 * PA7        OUTPUT Test signal
 * PA8        INPUT  Trig
 * PB0--B1    INPUT Rotary encoder
 * PB3--B7    INPUT Buttons
 * 
 * PB8        x      TL_PWM
 * PB9        OUTPUT LCD RESET
 * PB12       OUTPUT AMPSEL
 * 
 * PB14/PB15  better rotary encoder
 
 * PA1..A4    x      SENSEL
 * PA9/PA10   INPUT Uart
 * PA5        INPUT  CPLSEL
 * 
 */


#if 0 // Use TX/RX pin for rotary encoder
#define ALT_ROTARY_LEFT   PA9
#define ALT_ROTARY_RIGHT  PA10
#define ROTARY_GPIO       GPIOA
#define ROTATY_SHIFT      9  
#else  // Use Pb14 & PB15 for rotary encoder
#define ALT_ROTARY_LEFT   PB14
#define ALT_ROTARY_RIGHT  PB15
#define ROTARY_GPIO       GPIOB  
#define ROTATY_SHIFT      14  
#endif

#define COUPLING_PIN PA5
  
#define SENSEL_PIN PA1 //(1..4)


#define PIN_TEST_SIGNAL     PA7
#define PIN_TEST_SIGNAL_AMP PB12