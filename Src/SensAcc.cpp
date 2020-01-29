/*************************************************************
  G-Force Sensor
  
  Sensor: ADXL 345 (GY-291)
  I2C Adrtesse 53h

  Ausrichtung der Platine:
    ^^ Flugrichtung ^^
          Z--Y
          |
          X

  G-Force +/- 16G
         positiv      negativ
  Pitch  Nase hoch    Nase runter
  Roll   Quer rechts  Quer links
  ------------------------------------------------------------
  
  Copyright (C) 2017 Thomas Lehmann
  
  Version history:
  1.00   17.07.2017  created

***************************************************************/


#include "SensAcc.h"




// ====
// Init
// ====
bool SensorAcc::Init( uint8_t Id, int8_t *_CfgCal )
{
  if( TmoRefresh )                                // bereits gestartet ?
    return( true );
  Accel = new Adafruit_ADXL345_Unified();         // standard I2C Adresse
  if( !Accel->begin() )                           // Sensor angeschlossen ?
    return( false );                              // nö
  Accel->setRange( ADXL345_RANGE_16_G );          // G-Force Bereich setzen, wir wolle das maximale ;)
  Accel->setDataRate( ADXL345_DATARATE_400_HZ );  // set measurement data rate

  // Ptr. auf Kalibrierungswerte (ACC_CAL_CNT groß)
  CfgCal = _CfgCal;

  TmoRefresh = ACC_TMO_REFRESH + Id;              // 10mS Timer asynchron zu anderen Sensoren bedienen
  return( true );
}


// ==========
// 10mS Timer
// ==========
void SensorAcc::Tim10mS()
{
  // Sensor aktiv ?
  if( --TmoRefresh == 0 )
    {
    TmoRefresh = ACC_TMO_REFRESH;

    // get a new sensor event
    sensors_event_t Evt;
    Accel->getEvent( &Evt );

#define Ndec  1
    // results (acceleration is measured in m/s^2)
    TVal.GForceXDez1 = calcGForce( Evt.acceleration.x, CfgCal[ACC_CAL_X], true, true );

/*
    gXc = (event.acceleration.x / 9.80665) - (CfgCal[ACC_CAL_X] / 100);
    gYc = (event.acceleration.y / 9.80665) + (CfgCal[ACC_CAL_Y] / 100);
    gZc = (event.acceleration.z / 9.80665) + (CfgCal[ACC_CAL_Z] / 100);
    gXc = gXc * -1.0;

if (FilterOnOff == 1) {
  gXv = round(Xfilter(gXc) * pow(10, Ndec));
  gYv = round(Yfilter(gYc) * pow(10, Ndec));
  gZv = round(Zfilter(gZc) * pow(10, Ndec));
} else {
  gXv = round(gXc * pow(10, Ndec));
  gYv = round(gYc * pow(10, Ndec));
  gZv = round(gZc * pow(10, Ndec));

    
    float GForce      = (round(Evt.acceleration.x * 1.01972) / 10.0) - (CfgCal[ACC_CAL_X] / 10);
    TVal.GForceXDez1  = GForce * -1.0;
    TVal.GForceXDez1 *= 10;                           // eine Nachkommastelle
    GForce            = (round(Evt.acceleration.y * 1.01972) / 10.0) - (CfgCal[ACC_CAL_Y] / 10);
    TVal.GForceYDez1  = GForce * -1.0;
    TVal.GForceYDez1 *= 10;                           // eine Nachkommastelle
    GForce            = (round(Evt.acceleration.z * 1.01972) / 10.0) + (CfgCal[ACC_CAL_Z] / 10);
    TVal.GForceYDez1  = GForce * 10;                  // eine Nachkommastelle

    TVal.Pitch  = atan2(Evt.acceleration.x, Evt.acceleration.z) * (180 / PI) - (CfgCal[ACC_CAL_PITCH]);
    TVal.Roll   = atan2(Evt.acceleration.y, Evt.acceleration.z) * (180 / PI) - (CfgCal[ACC_CAL_ROLL]);
    TVal.Pitch *= -1;
    TVal.Roll  *= -1;
*/

    #if HW_TERM == 1
    TmoRefresh = 30;
    DbgPrintf("X:%d Y:%d Z:%d\n", TVal.GForceXDez1, TVal.GForceYDez1, TVal.GForceZDez1);
//    DbgPrintf(" Pitch:%d  Roll:%d\n", TVal.Pitch, TVal.Roll);
    
    DbgPrintf(" xCal:%d  yCal:%d zCal:%d\n\n", CfgCal[ACC_CAL_X], CfgCal[ACC_CAL_Y], CfgCal[ACC_CAL_Z]);
//    DbgPrintf(" pCal:%d rCal:%d\n\n", CalPitch, CalRoll);
    #endif
    }
}




float SensorAcc::calcGForce( float Acceleration, float Cal, bool Invers, bool Filter )
{
 float GForce = (Acceleration / 9.80665) - (Cal / 100);

  if( Invers )                               // Achse umkehren ?
    GForce *= -1.0;
  // pow(10, 1) >  10.0
  // pow(10, 2) > 100.0
  if( Filter )
    GForce = round(Xfilter(GForce) * pow(10, Ndec));
  else
    GForce = round(GForce * pow(10, Ndec));

  return( GForce );
}


// Digital filter designed by mkfilter/mkshape/gencode   A.J. Fisher
#define NZEROS 2
#define NPOLES 2
#define GAIN  1.482463775e+01

#define C0 -0.4128015981
#define C1  1.1429805025

float Xxv[NZEROS + 1], Xyv[NPOLES + 1];
float Yxv[NZEROS + 1], Yyv[NPOLES + 1];
float Zxv[NZEROS + 1], Zyv[NPOLES + 1];


float SensorAcc::Xfilter( float Xfilterin )
{
  for (int fx = 0; fx < (NZEROS); fx++)
    Xxv[fx] = Xxv[fx + 1];

  for (int fy = 0; fy < (NPOLES); fy++)
    Xyv[fy] = Xyv[fy + 1];

  Xxv[NZEROS] = Xfilterin / GAIN;
  Xyv[NPOLES] =  (Xxv[0] + Xxv[2]) + 2 * Xxv[1]
                 + ( C0 * Xyv[0]) + (  C1 * Xyv[1]);
  return Xyv[NPOLES];
}

float SensorAcc::Yfilter( float Yfilterin )
{
  for (int fx = 0; fx < (NZEROS); fx++)
    Yxv[fx] = Yxv[fx + 1];

  for (int fy = 0; fy < (NPOLES); fy++)
    Yyv[fy] = Yyv[fy + 1];

  Yxv[NZEROS] = Yfilterin / GAIN;
  Yyv[NPOLES] = (Yxv[0] + Yxv[2]) + 2 * Yxv[1]
                + ( C0 * Yyv[0]) + (  C1 * Yyv[1]);
  return Yyv[NPOLES];
}

float SensorAcc::Zfilter( float Zfilterin )
{
  for (int fx = 0; fx < (NZEROS); fx++)
    Zxv[fx] = Zxv[fx + 1];

  for (int fy = 0; fy < (NPOLES); fy++)
    Zyv[fy] = Zyv[fy + 1];

  Zxv[NZEROS] = Zfilterin / GAIN;
  Zyv[NPOLES] =  (Zxv[0] + Zxv[2]) + 2 * Zxv[1]
                 + ( C0 * Zyv[0]) + (  C1 * Zyv[1]);
  return Zyv[NPOLES];
}



