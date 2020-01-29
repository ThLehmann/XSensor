/*************************************************************
  RPM Sensor
 
  ------------------------------------------------------------
  
  Copyright (C) 2017 Thomas Lehmann
  
  Version history:
  1.00   19.06.2017  created

***************************************************************/

#ifndef SENS_RPM_H
#define SENS_RPM_H


#include "Def.h"


#if _SENS_RPM_ == 1


void HallIRQ( void );                 // ISR Routine muss statisch sein


class SensorRpm
{
  friend void HallIRQ( void );


public:
  //SensorRpm();
	
	bool Init( void );
 	void Tim10mS( void );

  // Telemetriewerte
  struct
    {
    uint16_t Rpm;                           // akt. gemessene Drehzahl in UPM/Min, Telemetriewert
    uint32_t RevolTotDez2;                  // # Umdrehungen gesamt in dez. Format, Anzeige in 10K mit 2 dezimal Stellen, 1Mil1ion 234tausend  > 1.23
    uint32_t RunTimeDez2;                   // Betriebszeit in dez. Format, Anzeige in Stunden mit 2 dezimal Stellen, 100Min > 1.40
    }TVal;


private:

  uint32_t RevolStart;                      // # Umdrehungen bis NV Sicherung angeleiert wird

    #define RPM_AVG_CNT   8
    #define RPM_AVG_MSK   0x07
    #define RPM_AVG_SH    3                 // n/8, # shift right für Divison
  volatile uint32_t MesureAvg[RPM_AVG_CNT]; // die jeweils letzten n Messungen, # uS für eine Umdrehung (IRQ Messung)

  uint8_t TmoRpmCalc;
    #define TMO_RPM_CALC  (250/TIM_BASE)    // alle 250mS die akt. Drehzahl berechnen
    #define RPM_RUNNING   2                 // Motor läuft wenn in den 250mS mind. 2 Impulse (500UPM = 120mS) gemessen wurden
  uint8_t TmoRpmOff;                        // Zeit bis Messung beendet wird wenn keine Impulse mehr eintrudeln
    #define TMO_RPM_OFF  (500/TIM_BASE)     // 500mS ohne Messung > RPM = 0
};

#endif


#endif
