/**
 
 
 
 */
#pragma once


/**
 */
class DSOInputGain
{
public:
    enum InputGainRange
    {
      MAX_VOLTAGE_GND=0,    // 0
      
      MAX_VOLTAGE_20MV=1,     // 1
      MAX_VOLTAGE_40MV=2,     // 2
      MAX_VOLTAGE_80MV=3,     // 3
      MAX_VOLTAGE_200MV=4,    // 4
      MAX_VOLTAGE_250MV=5,    // In theory it is 400MV but the TL084 saturates
      // do not use MAX_VOLTAGE_400MV=5,    // 5
      // do not use MAX_VOLTAGE_800MV=6,    // 6
      
      MAX_VOLTAGE_2V=7,      // 7
      MAX_VOLTAGE_4V=8,      // 8
      MAX_VOLTAGE_8V=9,      // 9
      MAX_VOLTAGE_20V=10,     // 10
    // do not use   MAX_VOLTAGE_40V=11,     // 11
    // do not use   MAX_VOLTAGE_80V=12   // 12
    };
    static bool                         setGainRange(InputGainRange range);
    static DSOInputGain::InputGainRange getGainRange();
    static int                          getOffset(int dc0ac1);
    static float                        getMultiplier();
    static bool                         readCalibrationValue();
};