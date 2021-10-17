/***************************************************
 * Basic zero calibration UI
 * Switch to all input gain source and mesure the ADC output
 * It will be used later to convert ADC => real voltage
 * (c) mean 2021 fixounet@free.fr
 ****************************************************/

#include "lnArduino.h"
#include "dso_control.h"
#include "dso_adc_gain.h"
#include "dso_calibrate.h"
#include "dso_adc_gain.h"
#include "dso_gfx.h"
#include "lnADC.h"
#include "dso_capture_input.h"
#include "gd32/nvm_gd32.h"


static void printCalibrationTemplate( const char *st1, const char *st2);
static void doCalibrate(uint16_t *array,int color, const char *txt,DSOControl::DSOCoupling target);

//
extern lnTimingAdc              *_adc;
extern DSOControl               *control;


/**
 * 
 */
static void waitOk()
{
    while(1)
    {
        int evt=control->getQButtonEvent();
        control->getRotaryValue(); // purge
        if(evt==DSO_EVENT_Q(DSOControl::DSO_BUTTON_OK,EVENT_SHORT_PRESS))
            return;
        xDelay(10);
    }
}


/**
 * 
 * @return 
 */
bool DSOCalibrate::zeroCalibrate()
{
    // Catch control callback
    DSOControl::ControlEventCb *oldCb=control->getCb();
    bool r=zeroCalibrate_();
    control->changeCb(oldCb);
    return r;    
}

/**
 * 
 * @return 
 */
bool DSOCalibrate::zeroCalibrate_()
{        
    DSO_GFX::setBigFont(false);
    DSO_GFX::setTextColor(WHITE,BLACK);
              
    printCalibrationTemplate("Connect the 2 crocs","together");
    waitOk();    
    doCalibrate(calibrationDC,YELLOW,"",DSOControl::DSO_COUPLING_DC);       
    doCalibrate(calibrationAC,GREEN, "",DSOControl::DSO_COUPLING_AC);    
          
    DSO_GFX::clear(0);    
    DSO_GFX::printxy(20,100,"Restart the unit.");
    while(1) {};
    return true;        
}
/**
  */
void printCalibrationTemplate( const char *st1, const char *st2)
{
    DSO_GFX::newPage("CALIBRATION");    
    DSO_GFX::center(st1,4);
    DSO_GFX::center(st2,5);
    DSO_GFX::bottomLine("and press @OK@");    
}

/**
 * 
 * @param cpl
 */
static void printCoupling(DSOControl::DSOCoupling cpl)
{
    static const char *coupling[3]={"currently : GND","currently : DC ","currently : AC "};    
    DSO_GFX::center(coupling[cpl],5);      
}
/**
 * 
 * @param target
 */
static void  waitForCoupling(DSOControl::DSOCoupling target)
{
    DSOControl::DSOCoupling   cpl=(DSOControl::DSOCoupling)-1;    
    const char *st="Set input to DC";
    if(target==DSOControl::DSO_COUPLING_AC) st="Set input to AC";
    DSO_GFX::center(st,4);
    while(1)
    {
        DSOControl::DSOCoupling   newcpl=control->getCouplingState(); 
        if(newcpl==target) 
            DSO_GFX::setTextColor(GREEN,BLACK);
        else  
            DSO_GFX::setTextColor(RED,BLACK);

        if(cpl!=newcpl)
        {
            printCoupling(newcpl);
            cpl=newcpl;
        }
        control->getRotaryValue();
        if(cpl==target)
        {
            if(control->getQButtonEvent()==DSO_EVENT_Q(DSOControl::DSO_BUTTON_OK,EVENT_SHORT_PRESS))
            {               
                return;
            }
        }
    }
}
/**
 * 
 * @param color
 * @param txt
 * @param target
 */
void header(int color,const char *txt,DSOControl::DSOCoupling target)
{
    printCalibrationTemplate(txt,"");
    waitForCoupling(target);
    DSO_GFX::center("@- processing -@",6);
    return;
}
#define NB_SAMPLES 64
/**
 * 
 * @return 
 */
static int averageADCRead()
{
#define NB_POINTS 16
    uint16_t samples[NB_POINTS];
   _adc->multiRead(16,samples);
   int sum=0;
   for(int i=0;i<NB_POINTS;i++)
   {
       sum+=samples[i];
   }
   sum=(sum+(NB_POINTS/2-1))/NB_POINTS;
   return sum;
}

/**
 * 
 * @param array
 * @param color
 * @param txt
 * @param target
 */
void doCalibrate(uint16_t *array,int color, const char *txt,DSOControl::DSOCoupling target)
{
    header(color,txt,target);     
    for(int range=DSOInputGain::MAX_VOLTAGE_GND;range<=DSOInputGain::MAX_VOLTAGE_20V;range++)
    {
        DSOInputGain::setGainRange( (DSOInputGain::InputGainRange) range );
        xDelay(10);
        array[range]=averageADCRead();
        Logger("Range : %d val=%d\n",range,array[range]);
    }
}

/**
 * 
 * @return 
 */
bool DSOCalibrate::decalibrate()
{        
    return true;        
}
// EOF