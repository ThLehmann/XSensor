/*************************************************************
  Energie Sensor (Spannung / Strom / Leistung)

  - Empfängerspannung über Ext Eingang
  - freie Spannungsmessung 1-4

  - Strom über ACSxx Sensoren
  - zugeordneter Spg. Port für Leistungsberechnung

  ------------------------------------------------------------
  
  Copyright (C) 2017 Thomas Lehmann
  
  Version history:
  1.00   23.12.2017  created

***************************************************************/

#ifndef SENS_POWER_H
#define SENS_POWER_H

#include "Def.h"
#include "Filter.h"


#if _SENS_VOLT_ == 1

// ======================================================
// Spannungsmessung, indizierbar, 0=RX, 1-4 = Voltage 1-4
// Widerstände / Teilerverhältnis frei wählbar
// ======================================================
// max. 5V, Widerstandsverhältnis bestimmt resultierende Spg.
// max 10V (2S  8,4V) Vorwiderstand 1K : 1K
// max 15V (3S 12,6V) Vorwiderstand 1K : 2K
// max 20V (4S 16,8V) Vorwiderstand 1K : 3K
// max 25V (5S 21,0V) Vorwiderstand 1K : 4K
// max 30V (6S 25,2V) Vorwiderstand 1K : 5K
// ....
class SensorPwrVolt
{
public:

	bool Init( uint8_t Id, uint8_t VIdx );
 	void Tim10mS( void );


  //bool Failure;                             // TRUE == Fehler Sensor nicht angeschlossen, Laufzeit
  struct                                      // Telemetriewerte
    {
    uint16_t VoltageDez2;                     // akt. gemessene Spannung
    }TVal;

private:

  AnalogFilter *AnaFiltVolt;                  // gefilterter Analog Port für Spannung
  float   VoltScale;                          // uV je ADC Tick

  uint8_t TmoRefresh;                         // Zeit bis Messung erfolgt
    #define PWR_VOLT_TMO_REFRESH (500/TIM_BASE)  // alle 500mS eine Messung
#if HW_TERM == 1
    #undef  PWR_VOLT_TMO_REFRESH
    #define PWR_VOLT_TMO_REFRESH (2500/TIM_BASE)  // alle 500mS eine Messung
#endif
};

#endif




#if _SENS_MUI_  == 1

// =================
// Stromsensor ACSxx
// =================

// % um welche die Akkuspg. bei Neustart höher als letzte sein muss um die Kapazität zurückzusetzen
#define MUI_CAPA_AUTO_P   2                 / default 2%

enum
  {
  // bidirektionale Typen mit 2,5V Offs (Mittenspg.)
  STYP_MUI_ACS_NONE,
  STYP_MUI_ACS712_05,
  STYP_MUI_ACS712_20,
  STYP_MUI_ACS712_30,

  STYP_MUI_ACS758_50B,
  STYP_MUI_ACS758_100B,
  STYP_MUI_ACS758_150B,
  STYP_MUI_ACS758_200B,
  STYP_MUI_WITH_OFFS = STYP_MUI_ACS758_200B, 

  // unidirektionale Typen mit 0,6V Offs
  STYP_MUI_ACS758_50U,
  STYP_MUI_ACS758_100U,
  STYP_MUI_ACS758_150U,
  STYP_MUI_ACS758_200U,
  
  CNT_STYP_MUI
  };

#define ACS_TXT_LEN      8+1
struct S_ACS
  {
  char    Text[ACS_TXT_LEN];
  uint8_t mVperAmp;
  };

const struct S_ACS Acs[] PROGMEM =
{
  // 1234567
  { "------  ",   0 },
  { "712-05  ", 185 },   // ACS712-05
  { "712-20  ", 100 },   // ACS712-20
  { "712-30  ",  66 },   // ACS712-30
  { "758-50B ",  40 },   // ACS758-50B
  { "758-100B",  20 },   // ACS758-100B
  { "758-150B",  13 },   // ACS758-150B (13,3mV)
  { "758-200B",  10 },   // ACS758-200B
  { "758-50U ",  60 },   // ACS758-50U
  { "758-100U",  40 },   // ACS758-100U
  { "758-150U",  27 },   // ACS758-150U (26,7mV)
  { "758-200U",  20 }    // ACS758-200U
};

#define ACS_CALIB          125                    // 125mV Kalibrierwert Mitte, Bereich -125 bis + 125mV
#define ACS_CALIB_MUL      0                      // Multiplikator je Kalibrier Step, Bereich wird größer 
#define ACS_B_OFFS(Vref)  ((Vref/2.0) -(ACS_CALIB<<ACS_CALIB_MUL)) // offset in mV für Bidirektionale Typen,   typ 2.5V (ACS712-xx + ACS758-200B)
#define ACS_U_OFFS(Vref)  ((Vref/8.33)-(ACS_CALIB<<ACS_CALIB_MUL)) // offset in mV für uni-direktionale Typen, typ 0.6V


class SensorPwrMui
{
public:

  bool Init( uint8_t Id, uint8_t MuiIdx );
  void Tim10mS( void );
  int32_t  GetCurrent( bool Signed );


  //bool Failure;                             // TRUE == Fehler Sensor nicht angeschlossen, Laufzeit
  // Strom / Spg / Leistung / Verbrauch
  struct                                      // Telemetriewerte
    {
    uint16_t  CurrentDez2;                    // akt. gemessener Strom
    uint16_t  VoltageDez2;                    // akt. gemessene Spannung
    uint16_t  Watt;                           // akt. gemessener Leistung
    uint16_t  CapaConsumed;                   // verbauchte Kapazität
    }TVal;

private:

  void CapaReset( void )
    {
    GData.EEprom.NV.MuiCapaConsumed[MuiIdx] = 0;
    CapaStart = 0;
    GData.Signal |= SIG_SAVE2NV;
    }

  AnalogFilter *AnaFiltCurr;                  // gefilterter Analog Port für Strom
  AnalogFilter *AnaFiltVolt;                  // gefilterter Analog Port für Spannung
  float VoltScale;                            // Multiplikator entspr. Widerstandsteiler für Spg.Messung

  uint16_t GetVoltage( void )
    {
    return( AnaFiltVolt->Read() * VoltScale );
    }


  uint8_t  MuiIdx;
  uint8_t  CapaResSw;                         // Idx auf Switch Array für Rücksetztaster
  float    CapaConsumed;
  uint16_t CapaStart;
  uint32_t MesureTime;

  uint8_t TmoRefresh;                         // Zeit bis Messung erfolgt
    #define PWR_MUI_TMO_REFRESH (200/TIM_BASE)// alle 200mS eine Messung
  uint8_t TmoCapaReset;                       // Zeit bis Kapazitäts Reset, abwarten bis Spg. sicher anliegt
    #define PWR_MUI_TMO_CAPARES (2000/TIM_BASE)
};

#endif


#undef EXTRN
#endif
