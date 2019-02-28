/***************************************************
 STM32 duino based firmware for DSO SHELL/150
 *  * GPL v2
 * (c) mean 2019 fixounet@free.fr
 ****************************************************/
#include "dso_global.h"
#include "dso_adc.h"
#include "dso_capture.h"
#include "dso_capture_priv.h"
#include "transform.h"
static int      currentTimeBase=0;
static int      currentVoltageRange=0;
static bool     captureFast=true;
extern DSOADC   *adc;


/**
 * 
 * @return 
 */
DSOCapture::DSO_TIME_BASE DSOCapture::getTimeBase()
{
    if(captureFast) 
        return (DSOCapture::DSO_TIME_BASE)currentTimeBase;
    return (DSOCapture::DSO_TIME_BASE)(currentTimeBase+DSO_TIME_BASE::DSO_TIME_BASE_5MS);
}
/**
 * 
 * @param voltRange
 * @return 
 */
bool     DSOCapture::setVoltageRange(DSOCapture::DSO_VOLTAGE_RANGE voltRange)
{
    currentVoltageRange=voltRange;
    return true;
}
/**
 * 
 * @return 
 */
DSOCapture::DSO_VOLTAGE_RANGE DSOCapture::getVoltageRange()
{
     return (DSOCapture::DSO_VOLTAGE_RANGE )currentVoltageRange;
}


/**
 * 
 * @param timeBase
 * @return 
 */
bool     DSOCapture::setTimeBase(DSOCapture::DSO_TIME_BASE timeBase)
{
    if(timeBase<DSO_TIME_BASE::DSO_TIME_BASE_5MS) // fast mode
    {
        captureFast   =true;
        currentTimeBase=timeBase;
        
    }else
    {
        captureFast=false;
        currentTimeBase=timeBase-DSO_TIME_BASE::DSO_TIME_BASE_5MS;
    }
}
/**
 * 
 * @param count
 * @return 
 */
bool     DSOCapture::prepareSampling ()
{
    if(captureFast)
    {
        //      
        return adc->prepareDMASampling();
    }else
    {
        return adc->prepareTimerSampling(timerBases[currentTimeBase].fq);
    }
}
/**
 * 
 * @return 
 */
bool     DSOCapture::startSampling (int count)
{
    if(captureFast)
    {
        int ex=count*tSettings[currentTimeBase].expand4096;
        return adc->startDMASampling(ex);
    }
    return adc->startTimerSampling(count);
}

/**
 * 
 * @param count
 * @return 
 */
uint32_t *DSOCapture::getSamples(int &count)
{
    return adc->getSamples(count);
}
/**
 * 
 * @param buffer
 */
void     DSOCapture::reclaimSamples(uint32_t *buffer)
{
    adc->reclaimSamples(buffer);
}

/**
 * 
 * @param count
 * @return 
 */
int DSOCapture::oneShotCapture(int count,float *outbuffer,CaptureStats &stats)
{
    int available;
    prepareSampling();
    if(!startSampling(count)) return 0;
    uint32_t *buffer=    getSamples(available);
    
    int scale=vSettings[currentVoltageRange].inputGain;
    
    if(captureFast)
        count=transform((int32_t *)buffer,outbuffer,available,vSettings+currentVoltageRange,tSettings[currentTimeBase].expand4096,stats);
    else
        count=transform((int32_t *)buffer,outbuffer,available,vSettings+currentVoltageRange,4096,stats);
    
    reclaimSamples(buffer);
    return count;
}
/**
 * 
 * @param count
 * @param samples
 * @param waveForm
 * @return 
 */
bool DSOCapture::captureToDisplay(int count,float *samples,uint8_t *waveForm)
{
    float gain=vSettings[currentVoltageRange].displayGain;
    for(int j=0;j<count;j++)
        {
            float v=samples[j];
            v*=gain;
            v=239+120-v;             
            if(v>239) v=239;
            if(v<0) v=0;           
            waveForm[j]=(uint8_t)v;
        }
    return true;
}
/**
 * 
 * @return 
 */
const char *DSOCapture::getTimeBaseAsText()
{
    if(captureFast)
    {
        return tSettings[currentTimeBase].name;
    }
    return timerBases[currentTimeBase].name;
}
/**
 * 
 * @return 
 */
const char *DSOCapture::getVoltageRangeAsText()
{
    return vSettings[currentVoltageRange].name;
}
// EOF