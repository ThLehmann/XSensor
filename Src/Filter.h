/*************************************************************
  Filter Funktionen
  
  ------------------------------------------------------------
  
  Copyright (C) 2019 Thomas Lehmann
  
  Version history:
  1.00   24.11.2019  created

***************************************************************/

#ifndef FILTER_H
#define FILTER_H

// kleiner Smoothing Filter um Spikes zu elemenieren
// Anteil neuer Wert 25%, alter Wert 75%
// Val = ((float)NewVal*0.25f) + ((float)Val*0.75f);

class AnalogFilter
{
public:

  bool Open( uint8_t AnaPort )
    {
    if( AnaPort == 0 )                // gültiger Analog Port ?
      return( false );
    PA = AnaPort;                     // der gewünschte Analog Pin
    AdcVal = analogRead(PA);          // Basis laden damit Filter sofort anständigen Wert liefert
DbgPrintf(">>PA: %u AdcVal:%u PAadr:%x\n", PA, AdcVal, &PA);
    return( true );
    }

  uint16_t Read( void )
    {
    if( this->PA )                    // gültiger Analog Port ?
      this->AdcVal = ((float)analogRead(this->PA) *0.25f) + ((float)this->AdcVal*0.75f);
    return( this->AdcVal );
    }

private:

  uint8_t  PA;                        // der gewünschte Analog Eingang
  uint16_t AdcVal;                    // letzt gelesener Adc Wert 0-1023
 
};


#endif
