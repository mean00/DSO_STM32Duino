/***************************************************
 STM32 duino based firmware for DSO SHELL/150
 *  * GPL v2
 * (c) mean 2019 fixounet@free.fr
 ****************************************************/
#pragma once
class DSOCalibrate
{
public:
  static bool zeroCalibrate();
  static bool decalibrate();
  static bool voltageCalibrate();
protected:
  static bool voltageCalibrate_();
  static bool zeroCalibrate_();
};