/*************************************************************
  Temperatur Sensor
  - NTC Widerstand am Analog Input
  - MLX90614, Infrarot / Berührungslos am i2C

  ------------------------------------------------------------
  
  Copyright (C) 2017 Thomas Lehmann
  
  Version history:
  1.00   22.11.2017  created
  1.01   05.12.2017  one sensor for both types

***************************************************************/

#ifndef SENS_TEMP_H
#define SENS_TEMP_H


#include "Def.h"
#include "Filter.h"


#if _SENS_TEMP_ == 1

enum
  {
  STYP_TEMP_NONE,
  STYP_TEMP_IR,
  STYP_TEMP_NTC,
  
  CNT_STYP_TEMP
  };
#define STYP_TEMP_TXT { "--", "IR", "NTC" }


// indizierbar
class SensorTemp
{
public:
	
	bool Init( uint8_t Id, uint8_t SensIdx );
 	void Tim10mS( void );


  bool Failure;                             // TRUE == Fehler Sensor nicht angeschlossen, Laufzeit
  struct
    {
    uint16_t Temp;                           // akt. gemessene Temperatur in Grad Celsius, Telemetriewert
    }TVal;
    
private:

  AnalogFilter *AnaFiltNTC;                 // gefilterter Analog Port für NTC Temperatur

  uint8_t SensTyp;
  uint8_t IRAddr;                           // I2C Adresse für IR Sensor

  uint8_t TmoRefresh;                       // Zeit bis Messung erfolgt
    #define TEMP_TMO_REFRESH (250/TIM_BASE) // alle 250mS eine Messung, reicht für Motortemp

  uint16_t FiltTemp;                        // Filter Temp Messung
};

#endif


#endif
