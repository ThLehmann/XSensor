/*************************************************************
  Geschwindigkeits Sensor per Pitot Rohr (Druckmessung)
  
  ------------------------------------------------------------
  
  Copyright (C) 2018 Thomas Lehmann
  
  Version history:
  1.00   25.01.2018  created

***************************************************************/

#ifndef SENS_PITOT_H
#define SENS_PITOT_H


#include "Def.h"
#include "Filter.h"


#if _SENS_PITOT_ == 1


class SensorPitot
{
public:
	//SensorPitot();
	
	bool Init( uint8_t Id );
  void Tim10mS();

  bool Failure;                                   // TRUE == Fehler, Druck Sensor nicht angeschlossen

  // Telemetriewerte
  struct
    {
    uint16_t Speed;                               // Geschwindigkeit in km/h
    }TVal;

private:

  AnalogFilter *AnaFiltPress;                     // gefilterter Analog Port für Druck

  uint16_t ReadCnt = 0;
  float    RefPressure;                           // Referenz Druck

  uint8_t TmoRefresh;
    #define PITOT_TMO_REFRESH  (200/TIM_BASE)     // alle 200mS eine Messung vornehmen
};

#endif

#endif
