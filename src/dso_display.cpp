/***************************************************
 STM32 duino based firmware for DSO SHELL/150
 *  * GPL v2
 * (c) mean 2019 fixounet@free.fr
 * 
 * /!\ All related to waveform (waveform + trigger) are automatically centered,
 * you dont need to apply offset
 * 
 * 
 ****************************************************/
#include "lnArduino.h"
#include "dso_display.h"
#include "pattern.h"
#include "simpler9341.h"
#include "math.h"
#include "dso_colors.h"
#include "DSO_portBArbitrer.h"

#define XMAX(x, y) (((x) > (y)) ? (x) : (y))
#define XMIN(x, y) (((x) < (y)) ? (x) : (y))

#define LIGHT_GREEN          ILI_MK_COLOR(0,0x2f,0) // careful the colors are also in genpattern.py
#define DARK_GREEN           ILI_MK_COLOR(0,0x1f,0)

#define AUTOCAL_BOX_WIDTH   200
#define AUTOCAL_BOX_HEIGHT  80
#define AUTOCAL_BORDER      10
#define AUTOCAL_COLOR       WHITE

#define SCALE_STEP 20
#define C_X 12
#define C_Y 10
#define CENTER_CROSS 1

#define LINE_OFFSET 4

#define DSO_CHAR_HEIGHT 20
#define DSO_HEIGHT_OFFSET 1
#define DSO_INFO_START_COLUMN (248)
#define DSO_INFO_MAX_WIDTH  (320-DSO_INFO_START_COLUMN-8)

static void prettyPrint(float x,int mx);


static ili9341 *tft;
extern DSO_portArbitrer *arbitrer;
static DSO_ArmingMode oldMode=DSO_CAPTURE_MODE_INVALIDE;
static DSODisplay::MODE_TYPE mode=DSODisplay::VOLTAGE_MODE;
/**
 */
uint8_t prevPos[256];
uint8_t prevSize[256];
static char textBuffer[24];
//


#define MIN_ROW   0
#define MAX_ROW   2
#define FREQ_ROW  4
#define AVRG_ROW  6


class AutoGfx
{
public:
    AutoGfx()
    {
        arbitrer->beginLCD();
    }
    ~AutoGfx()
    {
        arbitrer->endLCD();
    }
};
//-

static const uint16_t *getBackGround(int line)
{
    const uint16_t *bg=(uint16_t *)defaultPattern;
    if(!(line%SCALE_STEP)) 
            bg=(uint16_t *)darkGreenPattern;
    if(line==DSO_WAVEFORM_WIDTH/2)
            bg=(uint16_t *)lightGreenPattern;
    return bg;
}


/**
 * 
 * @param line
 * @param info
 */
static void printMeasurement(int line, const float f)
{
    tft->setTextColor(WHITE,BLACK);
    tft->setCursor(DSO_INFO_START_COLUMN+2, DSO_HEIGHT_OFFSET+(line+1)*DSO_CHAR_HEIGHT-5);
    prettyPrint(f,320-DSO_INFO_START_COLUMN);
}
/**
 */
static const char *fq2Text(int fq)
{
    static char buff[16];
    float  f=fq;
    const char *suff="";
#define STEP(x,t)  if(f>x)     {suff=t;f/=(float)x;}else

    STEP(1000000,"M")
    STEP(1000,"K")
    if(*suff)
        sprintf(buff,"%2.1f%s",f,suff);
    else
        sprintf(buff,"%d%s",(int)f,suff);
    return buff;
}
/**
 * 
 */
void DSODisplay::init(ili9341 *d)
{
    tft=d;
    for(int i=0;i<256;i++)
    {
        prevPos[i]=120;
        prevSize[i]=1;
    }
   // triggerWatch.elapsed(0);
}

/**
 * 
 * @param data
 */
void  DSODisplay::drawWaveForm(int count,const uint8_t *data)
{
    AutoGfx autogfx;
    
    //tft->fillScreen(0);
    int last=data[0];
    last=XMIN(last,DSO_WAVEFORM_HEIGHT-1);
    int next;
    int start,sz;
    
    if(count<3) return;
    for(int j=1;j<count-1;j++)
    {
        
        int color=YELLOW;
        int next=data[j]; // in pixel
        if(next==0)
        {
            next=1;
            color=RED;
        }
        if(next>DSO_WAVEFORM_HEIGHT-1)
        {
            next=DSO_WAVEFORM_HEIGHT-1;
            color=RED;
        }
        start=XMIN(last,next);                
        sz=next-last;
        if(sz<0) sz=-sz;
        if(!sz)
        {            
            sz=1;
        }

        const uint16_t *bg=getBackGround(j);
        // cleanup prev draw
        tft->setAddress(    j,
                            prevPos[j]+DSO_WAVEFORM_OFFSET,
                            1,
                            prevSize[j]);        
        tft->pushColors(prevSize[j],((uint16_t *)bg)+prevPos[j]);
        // Now draw the real one
        
        tft->VLine(j,start+DSO_WAVEFORM_OFFSET,sz,color);
        prevSize[j]=sz;
        prevPos[j]=start;
        last=next;
    }    
} 
/**
 * 
 * @param mn
 * @param mx
 */
void  DSODisplay::drawMinMax(float mn, float mx)
{
    AutoGfx autogfx;
    
    printMeasurement(MIN_ROW+1, mn);
    printMeasurement(MAX_ROW+1, mx);    
}
void  DSODisplay::drawFq(int f)
{
    AutoGfx autogfx;
    if(f==0)
    {
        tft->square(0,
            DSO_INFO_START_COLUMN,             DSO_HEIGHT_OFFSET+(FREQ_ROW+1)*DSO_CHAR_HEIGHT+3-5,
            320-DSO_INFO_START_COLUMN,         DSO_CHAR_HEIGHT);
        tft->setCursor(DSO_INFO_START_COLUMN+2, DSO_HEIGHT_OFFSET+(FREQ_ROW+1+1)*DSO_CHAR_HEIGHT-5);
        tft->print("---");
        return;            
    }
    const char *t= fq2Text(f)  ;
    tft->setTextColor(WHITE,BLACK);
    int line=FREQ_ROW+1;
    tft->setCursor(DSO_INFO_START_COLUMN+2, DSO_HEIGHT_OFFSET+(line+1)*DSO_CHAR_HEIGHT-5);
    tft->printUpTo(t,320-DSO_INFO_START_COLUMN);
}

/**
 * 
 */
void DSODisplay::cleanup()
{
     AutoGfx autogfx;
     tft->setTextColor(LIGHT_GREEN,0);
     tft->fillScreen(0);
}

/**
 * 
 */
void DSODisplay::drawGrid(void)
{
    AutoGfx autogfx;
    uint16_t fgColor=DARK_GREEN;
    uint16_t hiLight=LIGHT_GREEN;
    for(int i=0;i<=C_Y;i++)
    {
        tft->HLine(0,
                SCALE_STEP*i+DSO_WAVEFORM_OFFSET,
                SCALE_STEP*(C_X),
                fgColor);
    }
    for(int i=0;i<=C_X;i++)
    {

        tft->VLine(SCALE_STEP*i,DSO_WAVEFORM_OFFSET,SCALE_STEP*C_Y,fgColor);
    }
    // Draw internal + cross in bright green
    tft->HLine(0,DSO_WAVEFORM_OFFSET,SCALE_STEP*C_X,hiLight);    
    tft->HLine(0,DSO_WAVEFORM_OFFSET+DSO_WAVEFORM_HEIGHT-1,SCALE_STEP*C_X,hiLight);
    tft->HLine(0,DSO_WAVEFORM_OFFSET+DSO_WAVEFORM_HEIGHT/2,SCALE_STEP*C_X,hiLight);
    
    
    tft->VLine(0,DSO_WAVEFORM_OFFSET,SCALE_STEP*C_Y,hiLight);
    tft->VLine(C_X*SCALE_STEP,DSO_WAVEFORM_OFFSET,SCALE_STEP*C_Y,hiLight);   
    tft->VLine((C_X*SCALE_STEP)/2,DSO_WAVEFORM_OFFSET,SCALE_STEP*C_Y,hiLight);
        
    //tft->fillCircle(LED_X +LED_R, LED_Y+LED_R, LED_R, GREEN);
}
/**
 * 
 * @param drawOrErase
 */    
void  DSODisplay::drawVerticalTrigger(bool drawOrErase,int column)
{
    if(drawOrErase)
     tft->VLine(column,DSO_WAVEFORM_OFFSET+1,DSO_WAVEFORM_HEIGHT-1,RED);
    else
    {
        const uint16_t *bg=getBackGround(column);
        tft->setAddress(column,1+DSO_WAVEFORM_OFFSET,
                        1,DSO_WAVEFORM_HEIGHT-1);
        tft->pushColors(DSO_WAVEFORM_HEIGHT,((uint16_t *)bg));
    }
}
/**
 * 
 * @param drawOrErase
 * @param line
 */

void  DSODisplay::drawVoltageTrigger(bool drawOrErase, int line)
{
    if(line<1) line=1;
    if(line>DSO_WAVEFORM_HEIGHT-1) line=DSO_WAVEFORM_HEIGHT-1;
    
    if(drawOrErase)
        tft->HLine(1,1+line,DSO_WAVEFORM_WIDTH-2,BLUE);
    else
    {
        const uint16_t *bg=getBackGround(line);
        tft->setAddress( 0,                  1+line,
                            DSO_WAVEFORM_WIDTH, 1);
        tft->pushColors(DSO_WAVEFORM_WIDTH,((uint16_t *)bg));
    }
}

/**
 * 
 * @param stats
 */

void prettyPrint(float x,int maxW=0)
{
    float a=fabs(x);
  
    if(a<0.8)
    {
        if(a<0.001)
            sprintf(textBuffer,"%03du",(int)(1000000.*x));
        else
            sprintf(textBuffer,"%03dm",(int)(1000.*x));
    }
    else
        sprintf(textBuffer,"%02.2f",x);
    if(maxW)
        tft->printUpTo(textBuffer,maxW);
    else
        tft->print(textBuffer);
}


char tmpBuf[10];
    
#define AND_ONE_A(x,y) { tft->print(DSO_INFO_START_COLUMN+2, DSO_HEIGHT_OFFSET+(y+1)*DSO_CHAR_HEIGHT,x/*,DSO_INFO_MAX_WIDTH*/);}        
#define AND_ONE_T(x,y) { tft->print(DSO_INFO_START_COLUMN+4, DSO_HEIGHT_OFFSET+(y+1)*DSO_CHAR_HEIGHT,x/*,DSO_INFO_MAX_WIDTH*/);}    
#define AND_ONE_F(x,y) { snprintf(tmpBuf,9,"%2.2f",x);tft->print(DSO_INFO_START_COLUMN+4, DSO_HEIGHT_OFFSET+(y+1)*DSO_CHAR_HEIGHT,tmpBuf/*,DSO_INFO_MAX_WIDTH*/);}    
#if 0
void DSODisplay::drawStats(CaptureStats &stats)
{

    if(stats.saturation)
    {
           tft->setTextColor(RED,BLACK);
    }else
    {
           tft->setTextColor(GREEN,BLACK);
    }
    AND_ONE_F(stats.xmin,3);
    AND_ONE_F(stats.xmax,5);
    AND_ONE_F(stats.avg,1);      
    tft->setTextColor(GREEN,BLACK);
    if(stats.frequency>0)
    {
        AND_ONE_T(fq2Text(stats.frequency),7);
    }else
    {
        AND_ONE_T("--",7);
    }
}
#endif
/**
 * 
 */
static void drawInfoHeader(int line, const char *info,int color)
{

    tft->square(color,
            DSO_INFO_START_COLUMN,             DSO_HEIGHT_OFFSET+(line)*DSO_CHAR_HEIGHT+3-LINE_OFFSET,
            320-DSO_INFO_START_COLUMN,         DSO_CHAR_HEIGHT);
    tft->print(DSO_INFO_START_COLUMN+2, DSO_HEIGHT_OFFSET+(line+1)*DSO_CHAR_HEIGHT-LINE_OFFSET,info/*,DSO_INFO_MAX_WIDTH*/);
}
/**
 * 
 */
void DSODisplay::drawStatsBackGround()
{
    AutoGfx autogfx;

#define BG_COLOR LIGHT_GREEN    
    tft->VLine(DSO_INFO_START_COLUMN, 0,240,BG_COLOR);
    tft->VLine(319, 0,240,BG_COLOR);
        

    tft->setTextColor(BLACK,BG_COLOR);
    drawInfoHeader(AVRG_ROW ,   "Avrg",BG_COLOR);
    drawInfoHeader(MIN_ROW,     "Min",BG_COLOR);
    drawInfoHeader(MAX_ROW,     "Max",BG_COLOR);  
    drawInfoHeader(FREQ_ROW,    "Freq",BG_COLOR);
    drawInfoHeader(8,           "Trigg",BG_COLOR);
    drawInfoHeader(10,          "Offst",BG_COLOR);
    tft->setTextColor(BG_COLOR,BLACK);
    oldMode=DSO_CAPTURE_MODE_INVALIDE;
    
}

/**
 * 
 * @param volt
 */
void  DSODisplay::printOffset(float volt)
{
    AND_ONE_F(volt,11);      
}

/**
 * 
 * @param mode
 * @param volt
 */
void DSODisplay::printTriggerValue( float volt)
{    
    
    AND_ONE_F(volt,9);      
}

void lowBarPrint(int column, const char *st)
{
    AutoGfx autogfx;
    
    tft->setCursor((column-1)*64+1, 240-2); 
    tft->square(0,(column-1)*64+1, 240-20,64,20);
    tft->print(st);
}
#define LOWER_BAR_PRINT(x,y) { tft->setCursor(x*64, 240-18); tft->print(y); /*myDrawString(x,64);*/}            
#define LOWER_BAR_PRINT_NCHARS(x,y,n) { tft->setCursor(y*64, 240-18); tft->myDrawString(x,n*18);}            
    
#define HIGHER_BAR_PRINT(x,y) { tft->print(y*64, 1,x/*,64*/);}            

#define SELECT(md)   { if(md==mode) tft->setTextColor(BLACK,BG_COLOR); else  tft->setTextColor(BG_COLOR,BLACK);}
#define LOWER_BAR(mode,st,column) {SELECT(mode);    LOWER_BAR_PRINT(st,column);}
#define LOWER_BAR_NCHAR(mode,st,column) {SELECT(mode);    LOWER_BAR_PRINT_NCHARS(st,column,4);}    
   
#define HIGHER_BAR(mode,st,column) {SELECT(mode);HIGHER_BAR_PRINT(st,column);}

static void genericDraw(int column,const char *v,bool highlight)
{
    if(highlight)
    {
       tft->setTextColor(BLACK, LIGHT_GREEN );
    }else
    {
        tft->setTextColor(LIGHT_GREEN,BLACK);
    }    
    lowBarPrint(column,v);
}
/**
 * 
 * @param v
 * @param highlight
 */
void DSODisplay::drawVolt(const char *v, bool highlight)    { genericDraw(VOLTAGE_MODE,v,highlight);}
void DSODisplay::drawTrigger(const char *v, bool highlight) { genericDraw(TRIGGER_MODE,v,highlight);}
void DSODisplay::drawTime(const char *v, bool highlight)    { genericDraw(TIME_MODE,   v,highlight);}
void DSODisplay::drawCoupling(const char *v, bool highlight){ genericDraw(ARMING_MODE, v,highlight);}
/**
 * 
 */
#if 0
void DSODisplay::printVoltTimeTriggerMode(const char *volt, const char *time,DSOCapture::TriggerMode tmode,DSO_ArmingMode arming)
{
    const char *st="????";
    switch(tmode)
    {
        case DSOCapture::Trigger_Rising:    st="UP  ";break;
        case DSOCapture::Trigger_Falling:   st="DOWN";break;
        case DSOCapture::Trigger_Both:      st="BOTH";break;
        case DSOCapture::Trigger_Run:       st="NONE ";break;
        default:            xAssert(0);            break;
    }
    const char *coupling="??";
     switch(controlButtons->getCouplingState())
    {
        case DSOControl::DSO_COUPLING_GND:  coupling="GND";break;
        case DSOControl::DSO_COUPLING_DC:   coupling=" DC"; break;
        case DSOControl::DSO_COUPLING_AC:   coupling=" AC"; break;
        default:            xAssert(0);            break;
    }    
    LOWER_BAR(VOLTAGE_MODE,volt,0);
    LOWER_BAR(TIME_MODE,time,1);
    LOWER_BAR_NCHAR(TRIGGER_MODE,st,2);
    
    SELECT(ARMING_MODE);
    drawArmingMode(arming);
   
    tft->setTextColor(BG_COLOR,BLACK);
    LOWER_BAR_PRINT_NCHARS(coupling,3,3);
    
    if(mode&0x80)
    {
        tft->setTextColor(BLACK,BLUE);
        switch(mode & 0x7f)
        {
            case TRIGGER_MODE:    AND_ONE_A("Trigg",8);break;
            case VOLTAGE_MODE:    AND_ONE_A("Offst",10);break;
        }
        tft->setTextColor(BG_COLOR,BLACK);
    }
        
}
#endif
/**
 * 
 * @return 
 */
DSODisplay::MODE_TYPE DSODisplay::getMode() 
{
    return mode;
}
/**
 * 
 * @param t
 */
void  DSODisplay::setMode(MODE_TYPE t) 
{
    if((mode&0x80) ^ !(t&0x80)) // leaving or entering alt
    {
        drawStatsBackGround();
    }
    mode=t;
}

 void DSODisplay::drawAutoSetup(void )
 {
     tft->fillRoundRect(320/2-AUTOCAL_BOX_WIDTH/2,
                         240/2-AUTOCAL_BOX_HEIGHT/2, 
                         AUTOCAL_BOX_WIDTH,AUTOCAL_BOX_HEIGHT,
                         4,AUTOCAL_COLOR);      
 }
void DSODisplay::drawAutoSetupStep(int i )
 {
    int pg=((AUTOCAL_BOX_WIDTH-AUTOCAL_BORDER*2)*i)/3;
     tft-> fillRoundRect(320/2-AUTOCAL_BOX_WIDTH/2+AUTOCAL_BORDER,
                         240/2-AUTOCAL_BOX_HEIGHT/2+AUTOCAL_BORDER, 
                         pg,
                         AUTOCAL_BOX_HEIGHT-AUTOCAL_BORDER*2,
                         4,
                         0);    
     
 }

/**
 * 
 * @param mode
 */
void  DSODisplay::drawArmingMode(DSO_ArmingMode arming)
{       
    const char *armingString="?";
    switch(arming)
    {
        case DSO_CAPTURE_SINGLE: armingString="SING";break;
        case DSO_CAPTURE_MULTI: armingString="REPT";break;
        case DSO_CAPTURE_CONTINUOUS: armingString="CONT";break;
            default:            xAssert(0);            break;
    }    
    HIGHER_BAR_PRINT(armingString,0);
}
/**
 * 
 * @param triggered
 */
DSO_ArmingMode lastMode=DSO_CAPTURE_MODE_INVALIDE;
bool lastTriggered=false;

void  DSODisplay::drawTriggeredState(DSO_ArmingMode mode, bool triggered)
{
    if(lastMode==mode && lastTriggered==triggered) return; // nothing to do
    lastMode=mode;
    lastTriggered=triggered;
    
    if(mode!=DSO_CAPTURE_SINGLE)
    {
       tft->setTextColor(BLACK,BLACK);  
       tft->setCursor(90, 0); 
       tft->print("Triggd"/*,90*/);
       tft->setTextColor(BG_COLOR,BLACK);  
       return;
    }
    const char *s="Wait...";
    
    int bg=BLACK;
    int fg=BG_COLOR;;
        
    if(triggered)
    {
        s="Triggd";
        tft->setTextColor(bg,fg);
    }else
    {
         tft->setTextColor(fg,bg);
    }
    tft->print(90, 0,s/*,90*/);
    tft->setTextColor(fg,bg);
}


// EOF
