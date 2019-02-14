/***************************************************
 STM32 duino based firmware for DSO SHELL/150
 *  * GPL v2
 * (c) mean 2019 fixounet@free.fr
 ****************************************************/

#include <Wire.h>
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_TFTLCD_8bit_STM32.h"
#include "Fonts/Targ56.h"
#include "Fonts/digitLcd56.h"
#include "Fonts/FreeSansBold12pt7b.h"
#include "MapleFreeRTOS1000.h"
#include "MapleFreeRTOS1000_pp.h"
#include "testSignal.h"
#include "dsoControl.h"
#include "HardwareSerial.h"
#include "dso_adc.h"
extern void splash(void);

static void drawGrid(void);
//--
extern Adafruit_TFTLCD_8bit_STM32 *tft;
extern DSOControl *controlButtons;
extern int ints;
extern DSOADC    *adc;
/**
 * 
 */
void testAdc2(void)
{
    int reCounter=0;
    
    
    int samples[256];
    
    int currentScale=10;
    
    // Calibrate...
    
    int cal=0;
    int avg;
    controlButtons->setInputGain(0);
    for(int i=0;i<256;i++)
    {
        cal+=analogRead(PA0);
        xDelay(2);
    }
    cal/=256;
    
    controlButtons->setInputGain(7); // x1.4
    adc->setTimeScale(ADC_SMPR_1_5,ADC_PRE_PCLK2_DIV_2); // 10 us *1024 => 10 ms scan
    while(1)
    {
       // splash();
        tft->fillScreen(BLACK);   
        drawGrid();
        tft->setCursor(200, 160);
        //tft->print(currentScale);
        
      //  for(int i=0;i<16;i++)
        {
            int count;
            int i=0;
            adc->initiateSampling(256);
            uint32_t *xsamples=adc->getSamples(count);
            for(int j=0;j<count;j++)
            {
                samples[j]=(int)(xsamples[j] >>16);//-cal; //&0xffff;
            }
            adc->reclaimSamples(xsamples);
            int min=4095,max=-4096;
            avg=0;
            for(int j=0;j<count;j++)
            {
                if(samples[j]<min) min=samples[j];
                if(samples[j]>max) max=samples[j];
                avg+=samples[j];

            }
            avg/=count;
                        
            float last=samples[0]; // 00--4096
            if(last>239) last=239;
            if(last<0) last=0;
            last/=20;
            for(int j=1;j<count;j++)
            {
                float next=samples[j]; // 00--4096
                next/=20; // 0..200
                if(next>239) next=239;
                if(next<0) next=0;
                
                if(next==last) last++;
                if(next>last)
                    tft->drawFastVLine(16*i+j,last,1+next-last,YELLOW);
                else
                    tft->drawFastVLine(16*i+j,next,1+last-next,YELLOW);
                last=next;
            }
        }
        tft->setCursor(200, 200);
        //tft->print((int)avg);
        xDelay(300);
        int inc=controlButtons->getRotaryValue();
        if(inc)
        {
            currentScale+=inc;
            if(currentScale<0) currentScale=0;
            currentScale&=0xf;
             controlButtons->setInputGain(currentScale);;
        }
        if(controlButtons->getButtonEvents(DSOControl::DSO_BUTTON_TIME)&EVENT_SHORT_PRESS)
        {            
         //   adc->setTimeScale(currentScale); 
        }
    }
}
//-
#define SCALE_STEP 24
#define C 10
#define CENTER_CROSS 1
void drawGrid(void)
{
    uint16_t fgColor=(0xF)<<5;
    for(int i=0;i<=C;i++)
    {
        tft->drawFastHLine(0,SCALE_STEP*i,SCALE_STEP*C,fgColor);
        tft->drawFastVLine(SCALE_STEP*i,0,SCALE_STEP*C,fgColor);
    }
    tft->drawFastHLine(0,239,SCALE_STEP*C,fgColor);
    
    tft->drawFastHLine(SCALE_STEP*(C/2-CENTER_CROSS),SCALE_STEP*5,SCALE_STEP*CENTER_CROSS*2,WHITE);
    tft->drawFastVLine(SCALE_STEP*5,SCALE_STEP*(C/2-CENTER_CROSS),SCALE_STEP*CENTER_CROSS*2,WHITE);
}
    