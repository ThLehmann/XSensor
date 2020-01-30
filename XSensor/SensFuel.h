/*************************************************************
  Benzin Durchfluss Sensor
 
  ------------------------------------------------------------
  
  Copyright (C) 2017 Thomas Lehmann
  
  Version history:
  1.00   01.07.2017  created

***************************************************************/

#ifndef SENS_FUEL_H
#define SENS_FUEL_H


#include "Def.h"

#define FUEL_FLOWCNT_1L   10500               // # Pulse je Liter lt. Hersteller

#if _SENS_FUEL_ == 1




// Fuelgeber   > INTx, fallende Flanke löst IRQ aus
// Pin2 oder 3 > INT0/1
// ------------------------------------------------
void FlowIRQ( void );                         // ISR Routine muss statisch sein


class SensorFuel
{
  friend void FlowIRQ( void );

public:

  bool Init( void );
 	void Tim10mS( void );
  void ResetFlowCntTank( void )
    {
    GData.EEprom.NV.FuelFlowCntTank = 0;      // entnommene Menge zurücksetzen
    FlowCntLast = -1;                         // Refresh Volumenanzeige
    GData.Signal |= SIG_SAVE2NV;
    }
  
  // Telemetriewerte
  struct
    {
    float    Flow;                            // akt. Flow in mL
    uint32_t FlowTotDez2;                     // gesamt entnommene Menge in Liter mit 2 dezimal Stellen
    int16_t  VolumeRemain;                    // berechneter Wert, verbleibende Menge in mL, !! kann auch negativ werden !!
    uint16_t VolumeConsumed;                  // akt. entnommene Menge in mL
    }TVal;



private:

  uint8_t  VolResSw;                          // Idx auf Switch Array für Rücksetztaster
  float    uL_OnePulse;                       // uL / Pulse
  uint32_t FlowCntStart;                      // # Impulse bis NV Sicherung angeleiert wird
  uint32_t FlowCntLast = -1;                  // 1. Refresh auch wenn Sensor neu (FlowCntTot=0)


    #define FLOW_AVG_CNT   8
    #define FLOW_AVG_MSK   0x07
    #define FLOW_AVG_SH    3                  // n/8, # shift right für Divison
  volatile uint32_t MesureAvg[FLOW_AVG_CNT];  // die jeweils letzten n Messungen in uS


  uint8_t TmoVolumeRefresh;
    #define TMO_VOL_REFRESH  (1000/TIM_BASE)  // jede Sek. das Restvolumen berechnen
  uint8_t  TmoFlowCalc;
    #define TMO_FLOW_CALC     (500/TIM_BASE)  // alle 500mS die akt. Durchflussmenge berechnen
  uint8_t TmoFlowOff;
    #define TMO_FLOW_OFF  TMO_FLOW_CALC + (100/TIM_BASE)  // mS ohne Flow Impuls
};


#endif


#endif
