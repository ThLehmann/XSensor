/*************************************************************
  PWM Messung
 
  ------------------------------------------------------------
  
  Copyright (C) 2019 Thomas Lehmann
  
  Version history:
  1.69   02.04.2019  created

***************************************************************/

#ifndef SENS_PWM_H
#define SENS_PWM_H


#include "Def.h"


#if _SENS_PWM_ == 1


void PwmIRQ( void );                 // ISR Routine muss statisch sein


class SensorPwm
{
  friend void PwmIRQ( void );


public:
  //SensorRpm();
	
	bool Init( void );
 	void Tim10mS( void );

  // Telemetriewerte
  struct
    {
    int16_t Pwm;                            // akt. gemessene PWM Pulsbreite in 0-100% (später evt. auch -100% - +100% / 750uS - 2250uS)
    }TVal;


private:

  volatile uint32_t MesureTime;             // die letzte Messung, # uS Pulsabstand (IRQ Messung)

  uint8_t TmoPwmCalc;
    #define TMO_PWM_CALC  (250/TIM_BASE)    // alle 250mS die akt. PWM berechnen

};

#endif


#endif
