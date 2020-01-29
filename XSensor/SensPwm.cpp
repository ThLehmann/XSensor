/*************************************************************
  PWM Messung
  
  - Auslösung über digitalen Port per PINCHG IRQ
  ------------------------------------------------------------
    
  Copyright (C) 2019 Thomas Lehmann
  
  Version history:
  1.69   02.04.2019  created

***************************************************************/

#include "SensPwm.h"


#if _SENS_PWM_ == 1



SensorPwm *_pInstPwm = 0;               // instance pointer um die SensorPwm class vom ISR zu finden



bool SensorPwm::Init( void )
{
  if( _pInstPwm )                             // bereits gestartet ?
    return( true );
  _pInstPwm = this;

  // Digital Pin Dn > PinChg löst IRQ aus
	pinMode( PIN_PWM, INPUT_PULLUP );
	attachInterrupt( digitalPinToInterrupt(PIN_PWM), PwmIRQ, CHANGE );

  return( true );
}


// =====================
// capture PWM interrupt
// =====================
void PwmIRQ( void )     // >> ISR( PCINT0_vect ) PIN change IRQ
{
 static uint32_t LastMicros;
 uint32_t uS = micros();                          // Genauigkeit 4uS

  // PWM startet mit steigender und endet mit fallender Flanke
  if( digitalRead(PIN_PWM) )                      // PWM Start
    LastMicros = uS;
  else
    _pInstPwm->MesureTime = uS - LastMicros;      // PWM Pulsbreite
}

// ==========
// 10mS Timer
// ==========
void SensorPwm::Tim10mS()
{
  // -------------------------------
  // PWM Messung bewerten, alle n mS
  // -------------------------------
  if( TmoPwmCalc )
    {
    TmoPwmCalc--;
    return;
    }
  TmoPwmCalc = TMO_PWM_CALC;                    // nur alle n mS bewerten
  TVal.Pwm = (MesureTime/10)*10;                // Messung glätten
  // kleiner Smoothing Filter um Spikes zu elemenieren
  // Anteil neuer Wert 25%, alter Wert 75%
  //TVal.Pwm = ((float)MesureTime*0.25f) + ((float)TVal.Pwm*0.75f);
/*
  TVal.Pwm = (MesureTime/10);                   // 100 - 200 und Messung Glätten
  if( TVal.Pwm < 100 )                          // Messungenauigkeit oder PWM < 1000uS
    TVal.Pwm = 100;
  else if( TVal.Pwm > 200 )                     // Messungenauigkeit oder PWM > 2000uS
    TVal.Pwm = 200;
  TVal.Pwm -= 100;                              // 0-100%
*/
}

#endif





//
