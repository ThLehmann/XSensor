/*************************************************************
  Benzin Durchfluss Sensor

  Sensor Turbine: BioTech FCH-m-POM-LC
  - Durchflussmenge 50mL - 3000mL
  
  Sensor Benzin : BioTech FCH-m-PP-LC
  - Durchflussmenge 15mL - 800mL
  - 10500Pulse / Liter
    > 1mL     = 10,5 Pulse
    > 0,095mL = 1 Pulse = 

  BioTech: http://www.btflowmeter.com/flowmeter-produkte/mini-flowmeter-durchflussmesser-turbinen-durchflussmessgeraete-schaufelrad-durchfluss-messgeraete-q-0010-1000-lmin-uebersicht/fch-mini-pom-q-0005-8-lmin-auswahl/fch-m-pom-08-l-min-art-nr-97478617-lc.html
  ------------------------------------------------------------
  
  Rücksetztaster, bei permaneter Aktivierung (Jumper) wird Restmenge nach 
  Neustart zuückgesetzt und nur einmalig gespeichert.
    
  
  Copyright (C) 2017 Thomas Lehmann
  
  Version history:
  1.00   01.07.2017  created

***************************************************************/

#include "SensFuel.h"

#if _SENS_FUEL_ == 1



// min. entnommene Menge (50mL) um Summenwerte zu sichern
#define  FLOWCNT2SAVE  ((50L*1000L)/uL_OnePulse)


SensorFuel *_pInstFuel = 0;             // Instance Pointer um die SensorFuel class vom ISR zu finden


#define CfgVolumeTank   GData.EEprom.Data.CfgFuelVolume
#define CfgFlowCnt1L    GData.EEprom.Data.CfgFuelFlowCnt1L
#define NV_FlowCntTank  GData.EEprom.NV.FuelFlowCntTank
#define NV_FlowCntTot   GData.EEprom.NV.FuelFlowCntTot

bool SensorFuel::Init( void )
{
  if( _pInstFuel )                      // bereits gestartet ?
    return( true );
  _pInstFuel = this;

  pinMode( PIN_FUEL, INPUT_PULLUP );
  attachInterrupt( digitalPinToInterrupt(PIN_FUEL), FlowIRQ, FALLING );


  // Tankinhalt auto Rücksetzen oder manuell per Taster / Jetibox
  #define CFG_FUEL  GData.EEprom.Data.CfgFuelVolRes
  VolResSw = MainEnableSw(CFG_FUEL);    // ggf. Tasten Abfrage aktivieren, Rückgabe ist SW Idx
  if( CFG_FUEL >= RESFCT_AUTO )         // auto zurücksetzen nach jedem Einschalten ?
    ResetFlowCntTank();                 // entnommene Menge zurücksetzen
  #undef CFG_FUEL

  // Berechnungsparas vorbelegen
  //mL_PulseCnt = (float)CfgPulseCnt1L / 1000.0;  // #Pulse / mL
  uL_OnePulse = (1000.0*1000.0) / CfgFlowCnt1L;   // uL / Pulse

  // entnommene Menge ab der gespeichert wird
  FlowCntStart = NV_FlowCntTot + FLOWCNT2SAVE;

//  DbgPrintf("uL je Pulse:%d\n", uL_OnePulse);

  return( true );
}


// ============================
// capture Durchfluss interrupt
// ============================
void FlowIRQ( void )    // >> ISR( PCINT1_vect ) PIN change IRQ
{
 static uint8_t  CaptureIdx;
 static uint32_t LastMicros;
 register uint32_t uS = micros();

  // Pulse/Min und entspr. Abstände
  //   1mL = 10,5 Pulse = 5714,29mS
  //  50mL =  525 Pulse =  114,29mS
  // 800mL = 8400 Pulse =    7143uS
  // für Flowanzeige je n Messungen zur Durchschnittsberechnung sichern
  if( LastMicros )                                // aus irgendeinem Grund löst der IRQ nach Init 1x aus, 1. IRQ verwerfen
    _pInstFuel->MesureAvg[CaptureIdx++] = uS - LastMicros;
  CaptureIdx &= FLOW_AVG_MSK;
  LastMicros = uS;

  NV_FlowCntTank++;                               // Summe der Pulse, entnommenes Volumen
  NV_FlowCntTot++;                                // Summe der Pulse, gesamt entnommenes Volumen
  //OUT_TGL(PIN_LED)                              // LED wackelt bei jedem Impuls
}


// ==========
// 10mS Timer
// ==========
void SensorFuel::Tim10mS()
{
//NV_FlowCntTank++;                   // Summe der Pulse, entnommenes Volumen
//NV_FlowCntTot++;                    // Summe der Pulse, gesamt entnommenes Volumen

  // -----------------------
  // Rücksetztaster bedienen
  // -----------------------
  // Key low > Rücksetzen der entnommenen Menge
  if( BIT_TST(GData.Switch.Activ, VolResSw) )   // Rücksetztaster betätigt ?
    ResetFlowCntTank();                         // entnommene Menge zurücksetzen


  // ------------------------------------
  // Durchflussmenge berechnen, alle n mS
  // ------------------------------------
  if( TmoFlowCalc )
    TmoFlowCalc--;
  else if( NV_FlowCntTot != FlowCntLast )       // liegt neue Messung vor ? oder Init ? oder ResetVolume
    {
    uint32_t Avg = 0L;
    uint8_t Idx;

    Idx = FLOW_AVG_CNT;
    while( Idx-- )
      Avg += MesureAvg[Idx];                    // Summe der Messungen in uS
    Avg >>= FLOW_AVG_SH;                        // Sum/n, Mittelwert Impulsabstand in uS

    // Impuls Anzahl auf 1 Min hochrechnen
    float FCntMin;
    if( Avg )                                   // Division durch 0 vermeiden ;)
      {
      FCntMin = (60L*1000L*1000L) / Avg;
      // # uL/Pulse auf ml/Min umrechnen
      TVal.Flow = (FCntMin * uL_OnePulse) / 1000.0;  // uL > mL
      }

    // ausreichend entnommen zum Sichern ?
    if( NV_FlowCntTot > FlowCntStart )
      GData.Signal |= SIG_SAVE2NV;


    // akt. entnommene Menge
    TVal.VolumeConsumed = ((float)NV_FlowCntTank * uL_OnePulse) / 1000.0;
    // Restmenge und Gesamdurchfluß berechnen
    TVal.VolumeRemain = CfgVolumeTank - TVal.VolumeConsumed;
    // # Liter gesamt Tele Anzeige mit 2 dezimal Stellen, 1,79L > 1.79
    TVal.FlowTotDez2 = NV_FlowCntTot / (CfgFlowCnt1L/100);


    FlowCntLast = NV_FlowCntTot;
    TmoFlowCalc = TMO_FLOW_CALC;            // nur alle n mS bewerten und das nur bei Änderungen
    TmoFlowOff  = TMO_FLOW_OFF;             // Zeit ohne Impulse bis Durchfluss Stopp erkannt
    }


  // ----------------------------
  // Motor steht, kein Durchfluss
  // ----------------------------
  if( TmoFlowOff && (--TmoFlowOff == 0) )   // entnommene Menge sichern nur wenn wir ausreiched Flow hatten
    {
    TVal.Flow = 0;                          // Flow Anzeige = 0mL/Min
    // entnommene Menge (50mL) ab der gespeichert wird
    FlowCntStart = NV_FlowCntTot + FLOWCNT2SAVE;  // warten auf min mL Verbrauch, unerfolgreiche Startversuche verwerfen
    //memset( MesureAvg, 0, sizeof(MesureAvg) );  // Durchnitt neu berechnen
    }
}

#endif
