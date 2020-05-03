/**
 * Derived from https://github.com/pingumacpenguin/STM32-O-Scope/blob/master/STM32-O-Scope.ino
 */
/*.
(c) Andrew Hull - 2015
STM32-O-Scope - aka "The Pig Scope" or pigScope released under the GNU GENERAL PUBLIC LICENSE Version 2, June 1991
https://github.com/pingumacpenguin/STM32-O-Scope
Adafruit Libraries released under their specific licenses Copyright (c) 2013 Adafruit Industries.  All rights reserved.
*/
/**
 We use PA0 as input pin
 * DMA1, channel 0
 
 * Vref is using PWM mode for Timer4/Channel 3
 * 
 */

#include "dso_adc.h"
#include "dso_adc_priv.h"
#include "fancyLock.h"
#include "dma.h"
#include "adc.h"




struct rcc_reg_map_extended {
    __IO uint32 CR;             /**< Clock control register */
    __IO uint32 CFGR;           /**< Clock configuration register */
    __IO uint32 CIR;            /**< Clock interrupt register */
    __IO uint32 APB2RSTR;       /**< APB2 peripheral reset register */
    __IO uint32 APB1RSTR;       /**< APB1 peripheral reset register */
    __IO uint32 AHBENR;         /**< AHB peripheral clock enable register */
    __IO uint32 APB2ENR;        /**< APB2 peripheral clock enable register */
    __IO uint32 APB1ENR;        /**< APB1 peripheral clock enable register */
    __IO uint32 BDCR;           /**< Backup domain control register 0x20*/
    __IO uint32 CSR;            /**< Control/status register        0x24*/
    __IO uint32_t AHBRST;       /**< AHB Reset Register             0x28/

/* Below are GD32 specific registers */
                                  
    __IO uint32_t CFG1;         /**< Clock configuration register 1 0x2c  */
    // deepsleep 0x34
    // addition: internal 48 Mhz clock 0xc0
    // addition : interrupt 0xcc
    // addition : CTC 0xe0
    // addition  : APB1 0xe4
};


HardwareTimer pwmtimer(4); // Vref PWM is Timer4 Channel3
/**
 * 
 * @return 
 */
#define NB_SAMPLE 16
float DSOADC::readVCCmv()
{
   float fvcc=0;
   adc_Register = ADC1->regs;
   adc_Register->CR2 |= ADC_CR2_TSVREFE;    // enable VREFINT and temp sensor
   adc_Register->SMPR1 =  ADC_SMPR1_SMP17;  // sample ra
   for(int i=0;i<NB_SAMPLE;i++)
   {
       delay(10);   
       fvcc+=  adc_read(ADC1, 17); 
   }
    fvcc=(1200. * 4096.*NB_SAMPLE) /fvcc;   
    //fvcc=3380;
    vcc=fvcc;
    return fvcc;
}



/**
 * 
 */
void DSOADC::setupADCs ()
{
 // 1 - Read VCC
   adc_Register = ADC1->regs;
   adc_Register->CR2 |= ADC_CR2_TSVREFE;    // enable VREFINT and temp sensor
   adc_Register->SMPR1 =  ADC_SMPR1_SMP17;  // sample ra

 // 2 - Setup ADC
    
  int pinMapADCin = PIN_MAP[_pin].adc_channel;
  adc_set_sample_rate(ADC1, ADC_SMPR_1_5); //=0,58uS/sample.  ADC_SMPR_13_5 = 1.08uS - use this one if Rin>10Kohm,
  adc_set_sample_rate(ADC2, ADC_SMPR_1_5);    // if not may get some sporadic noise. see datasheet.
  adc_set_prescaler(ADC_PRE_PCLK2_DIV_2);
   
  adc_set_reg_seqlen(ADC1, 1);
  
  adc_Register->SQR3 = pinMapADCin;
  
  cr2=ADC_CR2_EXTSEL ;  
  adc_Register->CR2=cr2;  
  cr2|=ADC_CR2_ADON;
  adc_Register->CR2=cr2;  
  
}
/**
 * 
 * @param mode
 * @return 
 */
 bool DSOADC::setTriggerMode(TriggerMode mode)
 {
    ExtIntTriggerMode m;
    _triggerMode=mode;
#if 0    
    switch(_triggerMode)
    {
        case DSOADC::Trigger_Falling: m=FALLING;break;
        case DSOADC::Trigger_Rising:  m=RISING;break;
        case DSOADC::Trigger_Both:    m=CHANGE;break;
        default: xAssert(0);break;
    }
    // Hook trigger interrupt  
    attachInterrupt(triggerPin,TriggerInterrupt,m );
#endif  
    return true;
 }
 /**
  * 
  * @param ratio
  * @return 
  */
 bool DSOADC::setVrefPWM(int ratio)
 {
     //ratio=(ratio*16384)/0xffff;
//     pwmWrite(vRefPin,(uint16)ratio);
     return true;
 }
 
 /**
  * 
  * @return 
  */
 bool     DSOADC::getTriggerState()
 {
     return false; //!!digitalRead(triggerPin);
 }
/**
 * 
 * @param timeScaleUs
 * @return 
 */
 bool    DSOADC::setTimeScale(adc_smp_rate one, DSOADC::Prescaler two)
 {
    adc_set_sample_rate(ADC1, one); //=0,58uS/sample.  ADC_SMPR_13_5 = 1.08uS - use this one if Rin>10Kohm,

    volatile rcc_reg_map_extended *regs=(rcc_reg_map_extended *)RCC_BASE;
    int val=(int)two;
    
    int mask=3<<14;
    volatile uint32_t  r=regs->CFGR;
    r&=~mask;
    r|=(val&3)<<14;
    
#ifdef     HIGH_SPEED_ADC
    #define ADC_PSC3 (1<<29)
    #define ADC_PSC2 (1<<28)
    int b2=val&4;
    int b3=val&8;
    
    // B2 is bit 28 in CFGR
    if(b2) r|=ADC_PSC2;
    else   r&=~ADC_PSC2;
    
    // B3 is bit 29 in new register CFG1 at offset 0x2c
    volatile uint32_t cfg1=regs->CFG1;
    if(b3) cfg1|=ADC_PSC3;
    else   cfg1&=~ADC_PSC3;
    regs->CFG1=cfg1;
#endif    
    
    regs->CFGR=r;
    return true;
 }
 /**
  * 
  * @param count
  * @return 
  */
bool    DSOADC::prepareDMASampling (adc_smp_rate rate,DSOADC::Prescaler scale)
{    

    setTimeScale(rate,scale);
    return true;
}
/**
 * 
 * @param count
 * @return 
 */
bool DSOADC::getSamples(FullSampleSet &fullSet)
{
    if(!dmaSemaphore->take(10)) // dont busy loop
        return false;   
//    adcInterruptStats.nbConsumed++;
    fullSet=_captured;
    return true;
}
/**
 * 
 */
void  DSOADC::clearSemaphore()
{
    dmaSemaphore->reset();    
}

/**
* @brief Enable DMA requests
* @param dev ADC device on which to enable DMA requests
*/

void DSOADC::adc_dma_enable(const adc_dev * dev) 
{
  bb_peri_set_bit(&dev->regs->CR2, ADC_CR2_DMA_BIT, 1);  
}


/**
* @brief Disable DMA requests
* @param dev ADC device on which to disable DMA requests
*/

void DSOADC::adc_dma_disable(const adc_dev * dev) 
{
  bb_peri_set_bit(&dev->regs->CR2, ADC_CR2_DMA_BIT, 0);
  dma_attach_interrupt(DMA1, DMA_CH1, SPURIOUS_INTERRUPT);
}


/**
 * 
 */
void Oopps()
{
    xAssert(0);
}
/**
 * 
 * @param count
 * @param buffer
 * @param handler
 */
void DSOADC::setupAdcDmaTransfer(   int count,uint16_t *buffer, void (*handler)(void) )
{
  dma_init(DMA1);
  dma_attach_interrupt(DMA1, DMA_CH1, handler); 
  dma_setup_transfer(DMA1, DMA_CH1, &ADC1->regs->DR, DMA_SIZE_32BITS, (uint32_t *)buffer, DMA_SIZE_16BITS, (DMA_MINC_MODE | DMA_TRNS_CMPLT));// Receive buffer DMA
  dma_set_num_transfers(DMA1, DMA_CH1, count );
  adc_dma_enable(ADC1);
  dma_enable(DMA1, DMA_CH1); // Enable the channel and start the transfer.

}

void DSOADC::nextAdcDmaTransfer( int count,uint16_t *buffer)
{
    dma_setup_transfer(DMA1, DMA_CH1, &ADC1->regs->DR, DMA_SIZE_32BITS, (uint32_t *)buffer, DMA_SIZE_16BITS, (DMA_MINC_MODE | DMA_TRNS_CMPLT));// Receive buffer DMA
    dma_set_num_transfers(DMA1, DMA_CH1, count );
    dma_enable(DMA1, DMA_CH1); // Enable the channel and start the transfer.
}

/**
 * 
 * @param interruptMask
 */
void DSOADC::enableDisableIrqSource(bool onoff, int interrupt)
{
    if(onoff)
    {
        // Enable Watchdog or EndOfCapture interrupt flags
        switch(interrupt)
        {
            case ADC_AWD:        
            {
                int channel=0;
                uint32_t cr1=ADC1->regs->CR1;
                cr1 &=~ 0x1f;
                cr1|= (channel & ADC_CR1_AWDCH) | 0*ADC_CR1_AWDSGL ;
                cr1|= ADC_CR1_AWDEN  | ADC_CR1_AWDIE;                
                SetCR1(cr1);
            }
                break;
            case ADC_EOC:
                {
                  uint32_t cr1=ADC1->regs->CR1;
                  cr1 |= ADC_CR1_EOCIE;
                  SetCR1(cr1);
                }
                 break;
            default:
                xAssert(0);
                break;
        }
    }else // disable
    {
        switch(interrupt)
        {
            case ADC_AWD:        
            {
                int channel=0;
                uint32_t cr1=ADC1->regs->CR1;
                cr1 &=~ 0x1f;
                cr1&= ~(ADC_CR1_AWDEN  | ADC_CR1_AWDIE);
                SetCR1(cr1);
            }
                break;
            case ADC_EOC:
                {
                  uint32_t cr1=ADC1->regs->CR1;
                  cr1 &= ~ADC_CR1_EOCIE;
                  SetCR1(cr1);
                }
                 break;
            default:
                xAssert(0);
                break;
        }
    }
}
/**
 * 
 * @param onoff
 */
void DSOADC::enableDisableIrq(bool onoff)
{
    if(onoff)
    {
        nvic_irq_enable(ADC1->irq_num);
    }
    else
        nvic_irq_disable(ADC1->irq_num);
}
/**
 * 
 */
extern void watchDog();
void DSOADC::defaultAdcIrqHandler()
{    
//    adcInterruptStats.rawWatchdog++;
    if(adcIrqHandler)
        adcIrqHandler();
}
        
/**
 * 
 * @param handler
 */
void DSOADC::attachWatchdogInterrupt(voidFuncPtr handler)
{
    adcIrqHandler=handler;
    adc_attach_interrupt(ADC1,ADC_AWD,defaultAdcIrqHandler);
}

/**
 * 
 * @param high
 * @param low
 */
void DSOADC::setWatchdogTriggerValue(uint32_t high, uint32_t low)
{
    ADC1->regs->HTR=high;
    ADC1->regs->LTR=low;
}
/**
 * 
 * @param pin
 * @return 
 */
 uint16_t directADC2Read(int pin)
 {
#if 1
    adc_reg_map *regs=  ADC2->regs; //PIN_MAP[COUPLING_PIN].adc_device.regs;
    adc_set_reg_seqlen(ADC2, 1);

    regs->SQR3 = pin;
    regs->CR2 |= ADC_CR2_SWSTART;
    while (!(regs->SR & ADC_SR_EOC))
        ;
    return regs->DR&0xffff;
#endif
    return 0;
 }
 
//