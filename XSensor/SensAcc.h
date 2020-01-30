/*************************************************************
  Acceleration / G-Force Sensor
  
  Sensor: ADXL 345
  angeschlossen über I2C, A4=SDA A5=SCL
  ------------------------------------------------------------
  
  Copyright (C) 2017 Thomas Lehmann
  
  Version history:
  1.00   17.07.2017  created

***************************************************************/

#ifndef SENS_ACC_H
#define SENS_ACC_H


#include "Def.h"

// ADXL Lib
#include "Adafruit_ADXL345_U.h"


// Index in Kalibrier Array
enum
  {
  ACC_CAL_X,
  ACC_CAL_Y,
  ACC_CAL_Z,
  ACC_CAL_PITCH,
  ACC_CAL_ROLL,

  ACC_CAL_CNT,
  };

class SensorAcc
{
public:
	
  bool Init( uint8_t Id, int8_t *CfgCal );
  void Tim10mS();

  // Telemetriewerte
  struct
    {
    int GForceXDez1;                        // G-Force X-Achse / Querruder
    int GForceYDez1;                        // G-Force Y-Achse / Höhenruder
    int GForceZDez1;                        // G-Force Z-Achse / Seitenruder
    int Pitch;                              // 
    int Roll;                               // 
    }TVal;

private:

  Adafruit_ADXL345_Unified  *Accel;
  int8_t *CfgCal;                           // Ptr. auf Kalibrierungswerte (ACC_CAL_CNT groß)

  uint8_t TmoRefresh;
    #define ACC_TMO_REFRESH  (100/TIM_BASE) // alle 100mS eine Messung vornehmen


  float calcGForce( float Acceleration, float Cal, bool Invers, bool Filter );
  float Xfilter( float Xfilterin );
  float Yfilter( float Yfilterin );
  float Zfilter( float Zfilterin );

};

#endif




