#if 0
/***************************************************
 STM32 duino based firmware for DSO SHELL/150
 *  * GPL v2
 * (c) mean 2019 fixounet@free.fr
 ****************************************************/
#include "dso_includes.h"
#include "Fonts/Targ56.h"
#include "Fonts/digitLcd56.h"
#include "Fonts/FreeSansBold12pt7b.h"
//#include "gfx/dso_small_compressed.h"
#include "gfx/dso150nb_compressed.h"
#include "cpuID.h"

extern void  autoSetup();
extern void  menuManagement(void);

extern void splash(void);
static void drawGrid(void);
extern void dsoUsb_processNextCommand();
extern void dsoUsb_sendData(int count,float *data, CaptureStats &stats);
//--
extern Adafruit_TFTLCD_8bit_STM32 *tft;
extern DSOControl *controlButtons;
extern testSignal *myTestSignal;
extern DSOADC   *adc;
//
float test_samples[256];
static uint8_t waveForm[256]; // take a bit more, we have rounding issues

uint32_t  refrshDuration=0;
int       nbRefrsh=0;
CaptureStats stats;    
static    int lastTrigger=-1;
static    DSOControl::DSOCoupling oldCoupling;
static    int triggered=0; // 0 means not trigger, else it is the # of samples in the buffer
DSO_ArmingMode armingMode=DSO_CAPTURE_CONTINUOUS; // single shot or repeat capture
bool      usbCaptureRequested=false;
void dso_usbInit();

static void initMainUI(void);
void drawBackground();

void uiRequestCapture(bool v )
{
    if(v) // ask captured data
    {
        usbCaptureRequested=false;
#warning fixme : get the # of samples        
        dsoUsb_sendData(240,test_samples,stats);
    }else
    {
        usbCaptureRequested=true;
    }
}

/**
 * 
 */
void splash(void)
{
        tft->fillScreen(BLACK);   
        //tft->drawRLEBitmap(dso_small_width,dso_small_height,20,64,WHITE,BLACK,dso_small);
        tft->drawRLEBitmap(dso150nb_width,dso150nb_height,20,64,WHITE,BLACK,dso150nb);
        tft->setFontSize(Adafruit_TFTLCD_8bit_STM32::SmallFont);        
        
        tft->setTextColor(WHITE,BLACK);        
        tft->setCursor(140, 64);
        tft->myDrawString("DSO-STM32duino");              
        tft->setCursor(140, 84);
#ifdef USE_RXTX_PIN_FOR_ROTARY        
        tft->myDrawString("USB  Version");              
#else
        tft->myDrawString("RXTX Version");              
#endif
        char bf[20];
        sprintf(bf,"%d.%02d",DSO_VERSION_MAJOR,DSO_VERSION_MINOR);
        tft->setCursor(140, 64+20*2);        
        tft->myDrawString(bf);       
        tft->setCursor(140, 64+20*4);
        tft->myDrawString(cpuID::getIdAsString());         
        tft->setCursor(140, 64+20*5);
        sprintf(bf,"%d Mhz",F_CPU/1000000);
        tft->myDrawString(bf);         
}



static void redraw()
{
        DSODisplay::drawGrid();
        DSODisplay::printVoltTimeTriggerMode(capture->getVoltageRangeAsText(), capture->getTimeBaseAsText(),DSOCapture::getTriggerMode(),armingMode);
        DSODisplay::printTriggerValue(DSOCapture::getTriggerValue());
        DSODisplay::printOffset(capture->getVoltageOffset());
        //DSODisplay::drawArmingMode(armingMode,false);
}
#define STOP_CAPTURE() {DSOCapture::stopCapture();xDelay(20);}

static void buttonManagement()
{
    bool dirty=false;
    DSODisplay::MODE_TYPE newMode=DSODisplay::INVALID_MODE;
    
    
    DSOControl::DSOCoupling coupling=controlButtons->getCouplingState();
    if(coupling!=oldCoupling)
    {
        oldCoupling=coupling;
        dirty=true;
    }
    // OK button
    int evt=controlButtons->getButtonEvents(DSOControl::DSO_BUTTON_OK);
    if(evt & EVENT_SHORT_PRESS)
    {
        dirty=true;
        newMode=DSODisplay::ARMING_MODE;        
    }
   
    if(evt & EVENT_LONG_PRESS)    
    {
        STOP_CAPTURE();
        autoSetup();
        drawBackground();        
        return;
    }
    // Rotary push
    evt=controlButtons->getButtonEvents(DSOControl::DSO_BUTTON_ROTARY);    
    if(evt & EVENT_SHORT_PRESS)    
    {
        if(armingMode==DSO_CAPTURE_SINGLE)
        {
            triggered=0;
        }
    }
    if(evt & EVENT_LONG_PRESS)    
    {
        STOP_CAPTURE();
        menuManagement();
        drawBackground();
        
        return;
    }
  
    int inc=controlButtons->getRotaryValue();
    
    
    
    if(controlButtons->getButtonEvents(DSOControl::DSO_BUTTON_VOLTAGE) & EVENT_SHORT_PRESS)
    {
        dirty=true;
        newMode=DSODisplay::VOLTAGE_MODE;        
    }
    if(controlButtons->getButtonEvents(DSOControl::DSO_BUTTON_TIME) & EVENT_SHORT_PRESS)
    {
        dirty=true;
        newMode=DSODisplay::TIME_MODE;
    }
    if(controlButtons->getButtonEvents(DSOControl::DSO_BUTTON_TRIGGER) & EVENT_SHORT_PRESS)
    {
        dirty=true;
        newMode=DSODisplay::TRIGGER_MODE;
    }

  
    
    if(dirty)
    {
        if((DSODisplay::getMode()&0x7f)==newMode) // switch between normal & alternate
            newMode=(DSODisplay::MODE_TYPE)(DSODisplay::getMode()^0x80);
        DSODisplay::setMode(newMode);
        redraw();
    }
    dirty=false;

    if(inc)
    {

        switch(DSODisplay::getMode())
        {
             case DSODisplay::ARMING_MODE: 
                {
                    dirty=true;
                    int v=(int)armingMode;
                    v+=inc;
                    if(v<0) v=0;
                    if(v>DSO_ArmingMode::DSO_CAPTURE_CONTINUOUS) v=DSO_ArmingMode::DSO_CAPTURE_CONTINUOUS;
                    STOP_CAPTURE();
                    armingMode=(DSO_ArmingMode)v;
                    triggered=0;
                }
                break;
            
            case DSODisplay::VOLTAGE_MODE: 
                {
                int v=capture->getVoltageRange();
                dirty=true;
                v+=inc;
                if(v<0) v=0;
                if(v>DSOCapture::DSO_VOLTAGE_MAX) v=DSOCapture::DSO_VOLTAGE_MAX;
                STOP_CAPTURE();
                capture->setVoltageRange((DSOCapture::DSO_VOLTAGE_RANGE)v);                                
                
                }
                break;
            case DSODisplay::TIME_MODE: 
                {
                    int v=capture->getTimeBase();
                    dirty=true;
                    v+=inc;
                   if(v<0) v=0;
                   if(v>DSOCapture::DSO_TIME_BASE_MAX) v=DSOCapture::DSO_TIME_BASE_MAX;
                   DSOCapture::DSO_TIME_BASE  t=(DSOCapture::DSO_TIME_BASE )v;
                   DSOCapture::clearCapturedData();
                   STOP_CAPTURE();
                   
                   capture->setTimeBase( t);
                   
                }
                break;
            case DSODisplay::TRIGGER_MODE: 
                {
                    int t=capture->getTriggerMode();
                    t+=inc;
                    while(t<0) t+=4;
                    t%=4;
                    STOP_CAPTURE();                    
                    capture->setTriggerMode((DSOCapture::TriggerMode)t);                   
                    capture->setTimeBase( capture->getTimeBase()); // this will refresh the internal indirection table
                    dirty=true;
                }
                break;
            case DSODisplay::VOLTAGE_MODE_ALT:
            {
                float v=capture->getVoltageOffset();
                v+=0.1*inc;
                STOP_CAPTURE();
                capture->setVoltageOffset(v);                
                dirty=true;
                
                break;
            }
            case DSODisplay::TRIGGER_MODE_ALT: 
                {
                 float v=capture->getTriggerValue();

                   // Adjust dependingon current voltage range in 1/2 scale steps
                   float scale=capture->getVoltageRangeAsFloat(capture->getVoltageRange());
                   scale/=2.;
                   v+=scale*(float)inc;
                   STOP_CAPTURE();
                   capture->setTriggerValue(v);    
                   dirty=true;
                   
                }
                break;
            default: 
                            break;
        }       
    if(dirty)
        redraw();
    }
}
void uiSetTriggerValue(int v)
{
    float f=(float)v;
    f-=32768;
    f/=100.; // in volt
    capture->setTriggerValue(f);    
    redraw();
}
void uiSetVoltage(int v)
{
    if(v<0) v=0;
    if(v>DSOCapture::DSO_VOLTAGE_MAX) v=DSOCapture::DSO_VOLTAGE_MAX;
    STOP_CAPTURE();
    capture->setVoltageRange((DSOCapture::DSO_VOLTAGE_RANGE)v);                          
    redraw();
}
void uiSetTimeBase(int v)
{
    if(v<0) v=0;
    if(v>DSOCapture::DSO_TIME_BASE_MAX) v=DSOCapture::DSO_TIME_BASE_MAX;
    DSOCapture::DSO_TIME_BASE  t=(DSOCapture::DSO_TIME_BASE )v;
    DSOCapture::clearCapturedData();
    STOP_CAPTURE();
    capture->setTimeBase( t);
    redraw();
}
void uiSetTriggerMode(int v)
{
    DSOCapture::TriggerMode t=(DSOCapture::TriggerMode)v;
    capture->setTriggerMode(t);                   
    capture->setTimeBase( capture->getTimeBase()); // this will refresh the internal indirection table
    redraw();
}

void uiSetArmingMode(int v)
{
 
    if(v<0) v=0;
    if(v>DSO_ArmingMode::DSO_CAPTURE_CONTINUOUS) v=DSO_ArmingMode::DSO_CAPTURE_CONTINUOUS;
    DSO_ArmingMode mode=(DSO_ArmingMode)v;
    STOP_CAPTURE();
    armingMode=mode;
    triggered=0; 
    redraw();
}

/**
 * 
 */
void drawBackground()
{
    tft->fillScreen(BLACK);
    tft->setFontSize(Adafruit_TFTLCD_8bit_STM32::SmallFont);   
    tft->setTextSize(2);
    DSODisplay::drawGrid();
    DSODisplay::drawStatsBackGround();
    
    DSODisplay::printVoltTimeTriggerMode(capture->getVoltageRangeAsText(), capture->getTimeBaseAsText(),DSOCapture::getTriggerMode(),armingMode);
    DSODisplay::printTriggerValue(DSOCapture::getTriggerValue());
    DSODisplay::printOffset(capture->getVoltageOffset());
    //DSODisplay::drawArmingMode(armingMode,false);

}
/**
 * 
 */
void initMainUI(void)
{
    DSOCapture::setTimeBase(    DSOCapture::DSO_TIME_BASE_1MS);
    DSOCapture::setVoltageRange(DSOCapture::DSO_VOLTAGE_1V);
    DSOCapture::setTriggerValue(1.);
    drawBackground();    
    
    float f=DSOCapture::getTriggerValue();
    DSODisplay::printTriggerValue(f);
    
    lastTrigger=DSOCapture::voltageToPixel(f);
    DSODisplay::drawVoltageTrigger(true,lastTrigger);
    
}
/**
 *  called when nothing was captured, just in case the trigger value was changed
 */
int triggerLine;
void refreshTriggerIfNeedBe()
{
    float lastVoltageTrigger=DSOCapture::getTriggerValue()+DSOCapture::getVoltageOffset();            
    buttonManagement();
    float f=DSOCapture::getTriggerValue()+DSOCapture::getVoltageOffset();
    // did it change ?
    if(f!=lastVoltageTrigger)
    {
        triggerLine=DSOCapture::voltageToPixel(lastVoltageTrigger);            
        DSODisplay::drawVoltageTrigger(false,triggerLine);   
        lastVoltageTrigger=f;                
        triggerLine=DSOCapture::voltageToPixel(lastVoltageTrigger);     
        if(triggered)
            DSODisplay::drawWaveForm(triggered,waveForm);
        DSODisplay::drawVoltageTrigger(true,triggerLine);   
    }    
}
/**
 * 
 * @param count
 * @param stats
 */
void processCapture(int count, CaptureStats &stats)
{
    static uint32_t lastRefresh=0;
    // So we captured something
    // refresh the display

    if(usbCaptureRequested)
    {
        usbCaptureRequested=false;
        dsoUsb_sendData(count,test_samples,stats);
    }
    
    DSOCapture::captureToDisplay(count,test_samples,waveForm);  
    // Remove trigger
    DSODisplay::drawVoltageTrigger(false,triggerLine);        
    DSODisplay::drawWaveForm(count,waveForm);
    DSODisplay::drawTriggeredState(armingMode,triggered);        
    
    if(lastTrigger!=-1)
    {
         DSODisplay::drawVerticalTrigger(false,lastTrigger);
         lastTrigger=-1;
    }

    if(stats.trigger!=-1)
    {
        lastTrigger=stats.trigger;
        DSODisplay::drawVerticalTrigger(true,lastTrigger);
    }
    buttonManagement();        

    float f=DSOCapture::getTriggerValue()+DSOCapture::getVoltageOffset();
    triggerLine=DSOCapture::voltageToPixel(f);
    DSODisplay::drawVoltageTrigger(true,triggerLine);

    // refresh the stats once in a while        
    uint32_t m=millis();
    if(m<lastRefresh)
    {
        m=lastRefresh+101;
    }
    if((m-lastRefresh)>250)
    {
        lastRefresh=m;
        DSODisplay::drawStats(stats);
        refrshDuration+=(millis()-m);
        nbRefrsh++;
    }
}        

/**
 * 
 */
extern void testCoupling();
void mainDSOUI(void)
{
   // DSOCalibrate::voltageCalibrate();
    //testCoupling();
    DSODisplay::init();
    initMainUI();
    DSOADC::readVCCmv();
    float f=DSOCapture::getTriggerValue()+DSOCapture::getVoltageOffset();
    triggerLine=DSOCapture::voltageToPixel(f);
    DSOControl::DSOCoupling oldCoupling=controlButtons->getCouplingState();
    float lastVoltageTrigger=-999;
    dso_usbInit();
    while(1)
    {        
        int count=0;  
        dsoUsb_processNextCommand();
        switch(armingMode)
        {
            case DSO_CAPTURE_MULTI:
            case DSO_CAPTURE_CONTINUOUS:
                // this will retrigger a capture automatically if needed                
                // and does nothing if a capture is already running
                count=DSOCapture::capture(240,test_samples,stats);  
                if(!count) // Nothing captured, i.e. no trigger
                {     
                    refreshTriggerIfNeedBe(); // this will call button management
                    DSODisplay::drawTriggeredState(armingMode,triggered); // does nothing if no change
                    continue;
                }
                // capture successful !
                // only update coupling when we have no capture running (i.e. we got something)
                controlButtons->updateCouplingState();
                // display it
                processCapture(count,stats);
                break;
            case DSO_CAPTURE_SINGLE:                
                // if triggered >0, it means we got a capture and wait to be rearmed
                if(triggered)
                {
                    controlButtons->updateCouplingState(); // no capture running, we can update
                    refreshTriggerIfNeedBe(); // this will call button management
                    DSODisplay::drawTriggeredState(armingMode,triggered); // does nothing if no change
                    // no need to redraw the actual capture
                    xDelay(10); // yield a bit
                    continue;
                }else
                { // We are waiting for a valid capture in single mode
                    xDelay(1); // yield a bit
                    count=DSOCapture::capture(240,test_samples,stats);   // this will do nothing if a capture is already running                    
                    if(!count) // Nothing captured, i.e. no trigger
                    {     
                        refreshTriggerIfNeedBe(); // this will call button management
                        DSODisplay::drawTriggeredState(armingMode,triggered); // does nothing if no change
                        xDelay(1); // yield a bit
                        continue;
                    }
                    //
                    controlButtons->updateCouplingState();
                    triggered=count; // got something, switch to waiting to be rearmed mode
                    DSODisplay::drawTriggeredState(armingMode,triggered); // does nothing if no change
                    processCapture(count,stats);
                    break;
                }
                break;
            default:
                xAssert(0);
                break;
        }
     
    }        
} 
// EOF    

#endif