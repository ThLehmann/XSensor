/*************************************************************
  Geschwindigkeits Sensor per Pitot Rohr (Druckmessung)
  
  Sensor: MPXV7002DP

  -2 to 2 kPa (-0.3 to 0.3 psi)
  0.5 to 4.5 V Output 

  angeschlossen an AnalogPort, ADC Messung max ~320km/h
------------------------------------------------------------
  
  Copyright (C) 2018 Thomas Lehmann
  
  Version history:
  1.00   25.01.2018  created

***************************************************************/


#include "SensPitot.h"


#if _SENS_PITOT_ == 1



// ====
// Init
// ====
bool SensorPitot::Init( uint8_t Id )
{
  if( TmoRefresh )                                // bereits gestartet ?
    return( true );
  TmoRefresh = PITOT_TMO_REFRESH + Id;            // 10mS Timer asynchron zu anderen Sensoren bedienen

  AnaFiltPress = new AnalogFilter();              // gefilterter Analog Port
  return( AnaFiltPress->Open(PIN_PITOT) );
}


// ==========
// 10mS Timer
// ==========
void SensorPitot::Tim10mS()
{
  // Sensor aktiv ?
  if( TmoRefresh == 0 )                           // Sensor nicht angeschlossen
    return;

  if( ReadCnt < (5000/10) )                       // die ersten 5 Sek. (a' 10mS) zum Einpegeln
    {
    ReadCnt++;
    // akt. Druck als Referenz für Speed 0
    RefPressure = AnaFiltPress->Read();
    return;
    }

  if( --TmoRefresh )
    return;
  TmoRefresh = PITOT_TMO_REFRESH;

  if( AnaFiltPress->Read() >= MAX_ANALOG_READ - 20 )   // kein Sensor angeschlossen ?
    {
    TVal.Speed = 0;
    Failure = true;
    return;
    }
  Failure = false;

  #define ABSOLUTE_0_KELVIN      273.16f
  #define PRESSURE_SEA_LEVEL     101325.0f
  #define UNIVERSAL_GAS_CONSTANT 8.3144621f
  #define DRY_AIR_MOLAR_MASS     0.0289644f

  float PitotPressure;
  float AmbientPressure;
  float Temperature;
  float Density;
  float Airspeed_ms;
  int   AirPressure = 0;
  // Mittelwert über 8 Messungen bilden
  for( int i = 0; i < 8; i++ )
    AirPressure += AnaFiltPress->Read();
  AirPressure >>= 3;
  if( AirPressure < RefPressure)                      // Rückwärts fliegen geht nicht ;)
    AirPressure = RefPressure;
  
  // differential pressure in Pa, 1 V/kPa, max 3920 Pa
  PitotPressure = 5000.0f * ((AirPressure - RefPressure) / 1024.0f) + PRESSURE_SEA_LEVEL;
  AmbientPressure = PRESSURE_SEA_LEVEL;
  Temperature = 20.0f + ABSOLUTE_0_KELVIN;
  Density = (AmbientPressure * DRY_AIR_MOLAR_MASS) / (Temperature * UNIVERSAL_GAS_CONSTANT);
  Airspeed_ms = sqrt((2 * (PitotPressure - AmbientPressure)) / Density);
  TVal.Speed = Airspeed_ms * 3.600;                 // speed in km/h
  if( TVal.Speed < 20 )
    TVal.Speed = 0;

//DbgPrintf("Pitot:%d\n", TVal.Speed);
}

#endif
