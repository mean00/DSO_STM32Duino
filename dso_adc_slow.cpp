/**
 
 *  This is the slow capture mode
 * i.e. we setup n Samples acquisition through DMA
 * and a timer interrupt grabs the result 
 */

#include "dso_global.h"
#include "dso_adc_priv.h"

/**
 */



extern HardwareTimer ADC_TIMER;
extern adc_reg_map *adc_Register;

enum CaptureState
{
    Capture_idle,
    Capture_armed,
    Capture_dmaDone,
    Capture_timerDone,
    Capture_complete
};
static CaptureState captureState=Capture_idle;
/**
 */
extern int                  requestedSamples;
extern DSOADC               *instance;
       int                  currentIndex=0;
extern uint32_t             convTime;
int spuriousTimer=0;

int nbSlowCapture=0;
static int skippedDma=0;
static int nbTimer=0;
static int nbDma=0;


#define DMA_OVERSAMPLING_COUNT 4
static uint32_t dmaOverSampleBuffer[DMA_OVERSAMPLING_COUNT] __attribute__ ((aligned (8)));;
extern void Oopps();


/**
 * 
 * @param fqInHz
 * @return 
 */
bool DSOADC::setSlowMode(int fqInHz)
{    
    ADC_TIMER.attachInterrupt(ADC_TIMER_CHANNEL, Timer_Event);
    ADC_TIMER.setPeriod(1000000/(fqInHz)); // in microseconds, oversampled 16 times
    return true;
}
/*
 * Can we skip that ?
 */
static void dummy_dma_interrupt_handler(void)
{
    if(captureState!=Capture_armed)
        Oopps();
    captureState=Capture_dmaDone;
    nbDma++;
}

/**
 * 
 * @param count
 * @param buffer
 * @return 
 */
bool DSOADC::startInternalDmaSampling ()
{
  setupAdcDmaTransfer( DMA_OVERSAMPLING_COUNT,dmaOverSampleBuffer, dummy_dma_interrupt_handler );
  return true;
}
  /**
  * 
  * @param count
  * @return 
  */
bool    DSOADC::prepareTimerSampling (int fq)
{      
    setTimeScale(ADC_SMPR_55_5,ADC_PRE_PCLK2_DIV_2);
    setSlowMode(fq);        
    return true;    
}
/**
 * 
 * @param count
 * @param buffer
 * @return 
 */
bool DSOADC::startTimerSampling (int count)
{

   if(count>ADC_INTERNAL_BUFFER_SIZE/2)
        count=ADC_INTERNAL_BUFFER_SIZE/2;
    requestedSamples=count;


    currentIndex=0;
    convTime=micros();
    
    noInterrupts();
    captureState=Capture_armed;
    startInternalDmaSampling();   
    
    ADC_TIMER.setCompare(ADC_TIMER_CHANNEL, 1);    
    ADC_TIMER.setMode(ADC_TIMER_CHANNEL, TIMER_OUTPUTCOMPARE); // start timer
    ADC_TIMER.refresh();
    ADC_TIMER.resume();
    
    interrupts();    
} 
/**
 * 
 */
void DSOADC::Timer_Event() 
{    
    nbTimer++;
    instance->timerCapture();
}
/**
 * 
 * @param avg
 * @return 
 */
bool   DSOADC::validateAverageSample(uint32_t &avg)
{
    switch(captureState)
    {
        case Capture_armed: // skipped one ADC DMA ?
            skippedDma++;
            return false;
        case Capture_dmaDone:
            captureState=Capture_timerDone;
            break;
        case Capture_timerDone:
            spuriousTimer++;
            //Oopps();
            return false;
            break;
        case Capture_complete:
            break;
        default:
            Oopps();
            break;
    }
    
    avg=0;
    uint16_t *ptr=((uint16_t *)dmaOverSampleBuffer)+1;
   
    for(int i=0;i<DMA_OVERSAMPLING_COUNT;i++)
    {        
        avg+=*ptr;
        ptr+=2;
    }    
    avg=(avg+DMA_OVERSAMPLING_COUNT/2+1)/DMA_OVERSAMPLING_COUNT;
    return true;
    }
/**
 * \fn timerCapture
 * \brief this is one is called by a timer interrupt
 */
void DSOADC::timerCapture()
{    
    uint32_t avg=0;
    if(! validateAverageSample(avg))
        return;
    adcInternalBuffer[currentIndex]=avg;
    currentIndex++;
    if(currentIndex>=requestedSamples)
    {                
        dma_disable(DMA1, DMA_CH1);
        ADC_TIMER.setMode(ADC_TIMER_CHANNEL,TIMER_DISABLED);
        ADC_TIMER.pause();
        captureState=Capture_complete;
        
        SampleSet one(requestedSamples,adcInternalBuffer),two(0,NULL);
        captureComplete(false,one,two);
        return;
    }
    // Ask for next set of samples
    captureState=Capture_armed;
    nextAdcDmaTransfer( DMA_OVERSAMPLING_COUNT,dmaOverSampleBuffer);
}
#include "dso_adc_slow_trigger.cpp"
// EOF

