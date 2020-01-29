/*************************************************************
  Energie Sensor (Spannung / Strom / Leistung)
  - Empfängerspannung über Ext Eingang

  Um Messfehler zu klein zu halten ist die genaue Referenzspannung sehr wichtig. AVcc ist mit der Versorgungsspannung belegt.
  R1 an Vcc, R2 an GND
                     R1+R2    4K4+2K2
  Teilerverhältnis = -----    ------- = 3
                      R2        2K2
                   
                   Referenzspannung   5060mV
  Bereichsbreite = ----------------   ------ = 4,946mV je Tick
                     Maximalwert      1023

  Messwert vom ADC umwandeln in Spannung:
  Spannung = ADCwert * Bereichsbreite * Teilerverhältnis



  Empfängerspannung am Ext Anschluss.
  Spannungsteiler, halbe Spg. am Analog Pin, max. 10V
  
          8V4 (max. 10V)
           |
          1K
           |
  PinA6 ---|  (max 5V)
           |
          1K
           |
          GND

  Testergebnis:
  RX = 7.07V, Mitte = 3.53V > ADC 716
  5000mV / 1023 = ADC 4,8875855 mV / Tick
  Anzeige 6.99 - 7.00V
  ------------------------------------------------------------
  
  Copyright (C) 2017 Thomas Lehmann
  
  Version history:
  1.00   23.12.2017  created

***************************************************************/

#define SENS_POWER_CPP

#include "SensPower.h"




#if _SENS_VOLT_ == 1

// Klasse indizierbar, Aufruf je Sensor
// ====================================
bool SensorPwrVolt::Init( uint8_t Id, uint8_t VIdx )
{
  AnaFiltVolt = new AnalogFilter();  // gefilterter Analog Port für Spannung
  if( !AnaFiltVolt->Open(PIN_VOLT(VIdx)) )
    return( false );

  // mV je Tick, AVcc / 1023 Schritte, nK:nK = Teiler 1:x
  VoltScale = GData.ADCmV * ((float)(GData.EEprom.Data.CfgVoltRVcc[VIdx] + GData.EEprom.Data.CfgVoltRGnd[VIdx]) / (float)(GData.EEprom.Data.CfgVoltRGnd[VIdx]+1));

/*
#if HW_TERM == 1
DbgPrintf("%d > R1:%d R2:%d Scale:", PIN_VOLT(VIdx), GData.EEprom.Data.CfgVoltRVcc[VIdx], GData.EEprom.Data.CfgVoltRGnd[VIdx]);
Serial.print(VoltScale, 4);
DbgPrintf(" ADCuV:");
Serial.println(GData.ADCmV, 4);
#endif
*/

  TmoRefresh = PWR_VOLT_TMO_REFRESH + Id;     // Zeit bis zur 1. Messung
  return( true );
}


// ==========
// 10mS Timer
// ==========
void SensorPwrVolt::Tim10mS()
{
  // Refresh Spannungsmessung ?
  if( --TmoRefresh == 0 )
    {
    TmoRefresh = PWR_VOLT_TMO_REFRESH;          // Zeit bis zur nächsten Messung

    // A/D Wandler lesen 0-1023 = 0-5V
    // Ergebnis in uV, Telewert 2 dez Stellen 1234mV = 12.34V / 10 >> 1.23V
    TVal.VoltageDez2 = (AnaFiltVolt->Read() * VoltScale) / 10.0;
//DbgPrintf(" Volt:%d ADC:%u\n", TVal.VoltageDez2, AnaFiltVolt->Read());
//printFloat("Scale: ", VoltScale);
    }
}

#endif





#if _SENS_MUI_  == 1
// Klasse indizierbar, Aufruf je Sensor
// ====================================
#define CFG_MUI_CAPA_RES  GData.EEprom.Data.CfgMuiCapaRes[MuiIdx]
bool SensorPwrMui::Init( uint8_t Id, uint8_t SensIdx )
{
  MuiIdx = SensIdx;

  // kein SensorTyp obwohl analog Port für Stromsensor konfiguriert (nur dann ist MUI aktiv)?
  if( GData.EEprom.Data.CfgMuiSensTyp[MuiIdx] == STYP_MUI_ACS_NONE )
    return( false );

  AnaFiltCurr = new AnalogFilter();               // gefilterter Analog Port für Strom
  AnaFiltCurr->Open( PIN_MUI_A(MuiIdx) );
  AnaFiltVolt = new AnalogFilter();               // gefilterter Analog Port für Spannung
  AnaFiltVolt->Open( PIN_MUI_V(MuiIdx) );
  
  // mV je Tick, AVcc / 1023 Schritte, nK:nK = Teiler 1:x
  VoltScale = GData.ADCmV * ((float)(GData.EEprom.Data.CfgMuiRVcc[MuiIdx] + GData.EEprom.Data.CfgMuiRGnd[MuiIdx]) / (float)(GData.EEprom.Data.CfgMuiRGnd[MuiIdx]+1));


  CapaResSw = MainEnableSw(CFG_MUI_CAPA_RES);     // ggf. Tasten Abfrage aktivieren, Rückgabe ist Switch Idx
  if( CFG_MUI_CAPA_RES >= RESFCT_AUTO )           // Kapazität auto Rücksetzen bei Überschreiten der letzten Akkuspg. um n%
    TmoCapaReset = PWR_MUI_TMO_CAPARES;           // Zeit bis Kapazitäts Reset, abwarten bis Spg. sicher anliegt
                                                  // im NV sichern solbald 10mAh entnommen wurden
  CapaStart = GData.EEprom.NV.MuiCapaConsumed[MuiIdx] + 10;

  TmoRefresh = PWR_MUI_TMO_REFRESH + Id;          // Zeit bis zur 1. Messung
  return( true );
}



// ==========
// 10mS Timer
// ==========
void SensorPwrMui::Tim10mS()
{
  if( TmoRefresh == 0 )                           // nicht aktiv
    return;

  // Kapazitäts Reset per Taster ?
  if( BIT_TST(GData.Switch.Activ, CapaResSw) )    // Rücksetztaster betätigt ?
    CapaReset();

  // auto Kapazitäts Reset, abwarten bis Spg. sicher anliegt
  if( TmoCapaReset && (--TmoCapaReset) == 0 )
    {
    uint16_t mV = GetVoltage();                   // Spg. holen
    uint8_t Proz = CFG_MUI_CAPA_RES - RESFCT_AUTO + 100;   // 0% = auto > immer
    if( mV >= (GData.EEprom.NV.MuiVolt[MuiIdx] * (Proz/100.0)) )
      CapaReset();
//DbgPrintf(" AkkuSpg:%d Saved:%d Diff:%d Capa:%d\n", mV, GData.EEprom.NV.MuiVolt[MuiIdx], GData.EEprom.NV.MuiVolt[MuiIdx] * (Proz/100.0), GData.EEprom.NV.MuiCapaConsumed[MuiIdx]);
    }

  // Refresh Messwerte ?
  if( --TmoRefresh == 0 )
    {
    TmoRefresh = PWR_MUI_TMO_REFRESH;             // Zeit bis zur nächsten Messung
    uint32_t mA = GetCurrent(false);              // Strom berechnen
    uint16_t mV = GetVoltage();                   // Spg. holen

    TVal.CurrentDez2 = mA / 10.0;                 // Übergabe in Ampere
    TVal.VoltageDez2 = mV / 10.0;                 // Telewert 2 dez Stellen 1234mV = 12.34V / 10 >> 1.23V
    TVal.Watt        = mV * mA / 1000L / 1000L;   // akt. Leistung in W

    GData.EEprom.NV.MuiVolt[MuiIdx] = mV;         // letzte Spg. Lage für Auto Rücksetzen merken, gespeichert wird bei Stromfluss
//DbgPrintf("A:%u V:%u W:%u\n", TVal.Current, TVal.VoltageDez2, TVal.Watt);
    }
}
#undef CFG_MUI_CAPA_RES





// Strom ... berechnen
// ===================
// auch negativen Stromfluss melden
// Rückgabe ist akt. Stromfluss in mA
int32_t SensorPwrMui::GetCurrent( bool Signed )
{
  uint8_t STyp = GData.EEprom.Data.CfgMuiSensTyp[MuiIdx]; // eingestellter SensorTyp
  float mVOffs = GData.EEprom.Data.CfgMuiCalib[MuiIdx] << ACS_CALIB_MUL;   // Kalibrierung auf 0
  if( STyp <= STYP_MUI_WITH_OFFS )
    mVOffs += ACS_B_OFFS(GData.EEprom.Data.AVcc); // bi-direktionale Typen, Offs halbe Refspannung
  else
    mVOffs += ACS_U_OFFS(GData.EEprom.Data.AVcc); // uni-direktionale Typen

  // Strom AnalogPort gefiltert lesen
  float mV = AnaFiltCurr->Read() * GData.ADCmV;   // tasächliche mV für den gemessenen Analogwert
  float mA = ((mV - mVOffs) * 1000.0) / pgm_read_byte(&Acs[STyp].mVperAmp);

/*
if( Signed )
{
printFloat("\nmVOffs ", mVOffs);
printFloat("\nmV", mV );
DbgPrintf("\n\n");
}
*/

  if( mA < 0 && !Signed )                       // es ist ja kein Generator ;)
    mA = 0;
  MesureTime = millis() - MesureTime;           // Entnahme anteilig auf 1h gerechnet
  GData.EEprom.NV.MuiCapaConsumed[MuiIdx] += mA * MesureTime / (60*60*1000.0);
  MesureTime = millis();
  // ausreichend entnommen zum Sichern ?
  if( GData.EEprom.NV.MuiCapaConsumed[MuiIdx] > CapaStart )
    {                                           // triggern solange Strom fliesst
    CapaStart = GData.EEprom.NV.MuiCapaConsumed[MuiIdx];
    CapaStart++;
    GData.Signal |= SIG_SAVE2NV;
    }
  // verbrauchte
  TVal.CapaConsumed = GData.EEprom.NV.MuiCapaConsumed[MuiIdx];

//DbgPrintf(" mV:%d mA:%d / %d Capa:%d Start:%d\n", uV/1000, mA, TVal.CurrentDez2, GData.EEprom.NV.MuiCapaConsumed[MuiIdx], CapaStart);
//DbgPrintf("consumed:%d remain:%d A:%d V:%d W:%d\n", TVal.CapaConsumed, TVal.CapaRemain, TVal.CurrentDez2, TVal.VoltageDez2, TVal.Watt);
  return( (int32_t)mA );
}
/*
DbgPrintf ("uVFilt:%u", uVFilt);
printFloat(" ADCuV", GData.ADCuV );
printFloat(" uV", uV );
DbgPrintf("\n");
//Serial.println(uV/1000.0f);
return(0);
*/


#endif
