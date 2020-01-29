/*************************************************************
  JetiBox Bedienung
  
  include aus Main
  ------------------------------------------------------------
  
  Copyright (C) 2017 Thomas Lehmann
  
  Version history:
  1.00   05.12.2017  created

***************************************************************/



#define JBKEY_UP      0xd0
#define JBKEY_DOWN    0xb0
#define JBKEY_LEFT    0x70
#define JBKEY_RIGHT   0xe0
#define JBKEY_LR      0x60
#define JBKEY_UD      0x90
#define JBKEY_REFRESH 0xFF


// JB Sonderzeichen
// 0x12 Pfeil ho-ru
// 0x1d Pfeil re-li
// 0x1a Pfeil rechts
// 0x1b Pfeil links
// 0x18 Pfeil hoch
// 0x7e Pfeil doppelt rechts
// 0x7f Pfeil doppelt links




// ===========
// Auswahl +/-
// ===========
uint8_t _JBIdxPlusMinKey( uint8_t Key, uint8_t Index, uint8_t IdxMax, bool Save )
{
 uint8_t LIdx = Index;

  if( (Key == JBKEY_RIGHT && ++Index >= IdxMax) || 
      (Key == JBKEY_LEFT  && Index > 0) )
    Index--;
  if( Save && Index != LIdx )
    EEpromSave = true;
  return( Index );
}

// Val1 DIR Val2
// MUI1 < A0      durch <> wird Richtung gewechselt
// MUI1 > A0      und der jeweils andere Wert behandelt
int _JBDirIdx( bool *Dir, uint8_t Key, char *Buff, const char *Txt, uint8_t *Idx1, uint8_t Idx1Cnt, bool Idx1Save, uint8_t *Idx2, uint8_t Idx2Cnt, int Idx2Save )
{
  if( Key == JBKEY_LR )
    *Dir ^= true;
  if( *Dir == 0 )
    *Idx1 = _JBIdxPlusMinKey( Key, *Idx1, Idx1Cnt, Idx1Save );
  else if( Idx2Save >= 0 )                     // rechte Seite auch konfigurieren ?
    *Idx2 = _JBIdxPlusMinKey( Key, *Idx2, Idx2Cnt, Idx2Save );
  return( sprintf_P(Buff, PSTR("%s%d%c "), Txt, *Idx1+1, *Dir ? '>' : '<') );
}


// Reset Funktion für Fuel / Kapazität einstellen
uint8_t _JBGetResFct( uint8_t Key, char *Buff, uint8_t AutoCnt, uint8_t Fct )
{
  // Fct 0   = manuell ohne Taster
  // Fct 1-4 = manuell Taster an D4-D7
  // Fct 5-n = auto
  Fct = _JBIdxPlusMinKey( Key, Fct, CNT_PORT_DIGI_SW + 1 + AutoCnt, true );
  if( Fct <= CNT_PORT_DIGI_SW )
    {
    strcpy_P( Buff, PSTR("man SW: ") );
    if( Fct == 0 )
      strcpy_P( &Buff[8], PSTR("--") );
    else
      sprintf_P(&Buff[8], PSTR("D%d"), Fct+3);
    }
  else
    {
    strcpy_P( Buff, PgmTxtAuto );
    }

  return( Fct );
}



// ===========================
// #aktiver Sensoren ermitteln
// ===========================
// ret = # Zeichen im Buffer
int _JBPutSensActivCnt( char *Buff, uint8_t IdStart, uint8_t IdLast, bool CntOnly )
{
 uint8_t Idx, Cnt = 0;
 uint8_t Pos = CntOnly ? 6 : 0;
 const static char Strg[] PROGMEM = "activ #%u";

  for( Idx = IdStart; Idx <= IdLast; Idx++ )
    if( PBIT_TST(GData.EEprom.Data.SensActivMsk, Idx) )
      Cnt++;
  return( sprintf_P( Buff, &Strg[Pos], Cnt) );
}


// =======================
// Auswahl für Analog Port
// =======================
#define PA_OFFS  14    // A0 = 14
// ret = Status Sensor aktiv
bool _JBPortAnaChoice( char *Buff, int8_t SensId, int8_t *PA, uint8_t Key )
{                 // Port Idx > Port Nr
 int8_t  PIdx2PNb[] = { 0, A0, A1, A2, A3, A6, A7 };
 uint8_t PIdx;

  // Startindex suchen
  for( PIdx = 0; PIdx < ARRAYCNT(PIdx2PNb); PIdx++ )
    if( *PA == PIdx2PNb[PIdx] )
      break;
                                                  // nächsten / vorherigen Port einstellen
  PIdx = _JBIdxPlusMinKey( Key, PIdx, ARRAYCNT(PIdx2PNb), true );
  *PA = PIdx2PNb[PIdx];
  if( *PA == 0 )
    {
    strcpy_P( Buff, PSTR("--") );
    PBIT_CLR( GData.EEprom.Data.SensActivMsk, SensId );
    return( false );
    }
  else
    {
    sprintf_P(Buff, PSTR("A%d"), *PA-PA_OFFS);
    PBIT_SET( GData.EEprom.Data.SensActivMsk, SensId );
    return( true );
    }
}


// ========================
// Auswahl für Digital Port
// ========================
/*
uint8_t _JBPortDigiChoice( char *Buff, uint8_t PD, uint8_t Max, uint8_t Key )
{                    // Port Idx > Port Nr
 uint8_t PIdx2PNb[] = { 0, 4, 5, 6, 7, 10, 11, 12, 13 };
 uint8_t PIdx;

//  if( Max == 0 )
//    Max = ARRAYCNT(PIdx2PNb);

  // Startindex suchen
  for( PIdx = 0; PIdx < Max; PIdx++ )
    if( PD == PIdx2PNb[PIdx] )
      break;
                                                  // nächsten / vorherigen Port einstellen
  PIdx = _JBIdxPlusMinKey( Key, PIdx, ++Max, true );
  PD = PIdx2PNb[PIdx];
  if( PD == 0 )
    {
    const static char Strg[] PROGMEM = "--";
    strcpy_P( Buff, Strg );
    }
  else
    sprintf(Buff, "D%d", PD);
  return( PD );
}
*/

// ==================
// Wert konfigurieren
// ==================
void _JBCfgValue( uint8_t Key, uint16_t *Val, uint16_t Steps )
{
  if( Key == JBKEY_RIGHT )
    {
    EEpromSave = true;
    *Val += Steps;
    }
  else if( Key == JBKEY_LEFT && *Val >= Steps )
    {
    EEpromSave = true;
    *Val -= Steps;
    }
}

// ========================
// Widerstand konfigurieren
// ========================
static bool RStepFine;
void _JBCfgResistor( char *Buff, bool SetVcc, uint16_t *RGnd, uint16_t *RVcc, uint8_t Key )
{
 uint16_t *R = SetVcc ? RVcc : RGnd;

  if( Key == JBKEY_UD )                  // Hoch+Runter > Wechseln zwischen 100er und 5er Schritten
    RStepFine ^= true;

  _JBCfgValue( Key, R, RStepFine ? 5:100 );

  // GND100: 1000 Ohm
  // GND  5: 1000 Ohm
  strcpy_P(Buff, SetVcc ? PSTR("Vcc") : PSTR("GND"));
  sprintf_P(&Buff[3], PSTR("%s: %u Ohm"), RStepFine ? ".5":"", *R);
}



// -----------------------------------------------------------------------------------------------
// Menuaufbau
// ----------
//  Y
//  |
//   --X
//
//   X.Y
//   0.0 MP_MAIN
//       " XSensor by ThL"
//       "V1.00 12.06.2017"
//
//   0.1 Sens Cfg   F.1 Reset value       E.1 Setup/Service
//                  F.2 Res Fuel Volume   E.2 Sensor Nb
//                  F.3 Res Capacity      E.3 Watchdog
//                  F.4 Res Speed max     E.4 ADC Ref voltage
//                  F.5 Res Dist tot      E.5 MUI calib
//                  F.6 Res Runtime       E.6 Factory defaults  
//                  F.7 Res Revolution    E.7 Info
//                  F.8 Res Fuel tot      E.8 Debug
//                  F.9 Res Altitude tot
//
//   0.2 RPM        1.2 Fuel        2.2 TEMP        3.2 Alt         4.2 GPS               5.2 Pitot     6.2 Voltage   7.2 MUI             8.2 PWM       9.2 Acc
//   0.3 RelCnt     1.3 Volume      2.3 Temp Typ    3.3 AltTot      4.3 Pos activ         5.3 Port      6.3 Function  7.3 MUI1-3 Typ                    9.3 GForce X/Y/Z
//   0.4 Accuracy   1.4 Volume Res                  3.4 Temp        4.4 Alt activ                       6.4 Port      7.4 Port current                  9.4 calib GForce
//                  1.5 #Imp/L                      3.5 Vario       4.5 Speed activ                     6.5 R GND     7.5 capa reset                    9.5 Pitch
//                  1.6 calibrier                   3.6 Filter DZ   4.6 Dist-To activ                   6.6 R Vcc     7.6 Port voltage                  9.6 Roll
//                                                                  4.7 Dist Trip activ                               7.7 R GND
//                                                                  4.8 Sats activ                                    7.8 R Vcc

//
// -----------------------------------------------------------------------------------------------
// X-Pos für bedingtes Compilat
enum
  {
  #if _SENS_RPM_ == 1
  XP_RPM,
  #endif
  #if _SENS_FUEL_ == 1
  XP_FUEL,
  #endif
  #if _SENS_TEMP_ == 1
  XP_TEMP,
  #endif
  #if _SENS_ALT_ == 1
  XP_ALT,
  #endif
  #if _SENS_GPS_ == 1
  XP_GPS,
  #endif
  #if _SENS_PITOT_ == 1
  XP_PITOT,
  #endif
  #if _SENS_VOLT_ == 1
  XP_VOLT,
  #endif
  #if _SENS_MUI_  == 1
  XP_MUI,
  #endif
  #if _SENS_PWM_ == 1
  XP_PWM,
  #endif
  #if _SENS_ACC_ == 1
  XP_ACC,
  #endif

  XP_LAST
  };




//      4 BIT zulässige Key-Richtungen
//      4 BIT NC
//      4 BIT X-Pos (<->)
//      4 BIT Y-Pos (^^)
#define __MTYP(Key,X,Y)  (uint16_t)(((Key)<<12) | (X<<4) | Y)
#define __MPOS(MTYP)     (MTYP&0x00ff)
#define __MPOSX(MTYP)   ((MTYP&0x00f0)>>4)
#define __MPOSY(MTYP)    (MTYP&0x0f)

#define L 0x8    // left
#define R 0x4    // right
#define U 0x2    // up
#define D 0x1    // down
//                                          X  Y
#define MP_MAIN           __MTYP(D      ,   0, 0)   // Eintritt bzw. Konfig geändert, speichern/verwerfen
#define MP_SENSCFG        __MTYP(D|U|R  ,   0, 1)   // Sensor Konfig
#define MP_RESVAL         __MTYP(D|U|R|L, 0xf, 1)   // Reset Summvalues
#define MP_SETUP          __MTYP(D|U  |L, 0xe, 1)   // allg. Konfig
#define MP_HM_LAST        MP_SETUP


// =============
// Sensor Konfig
// =============

// Drehzahl
// --------
#if _SENS_RPM_ == 1
#define X0  XP_RPM
#define MP_RPM_A          __MTYP(D|U|R  ,   X0, 2)    // Sensor aktivieren
#define MP_RPM_RELCNT     __MTYP(D|U    ,   X0, 3)    // # Auslösungen je Umdrehung
#define MP_RPM_ACCURACY   __MTYP(  U    ,   X0, 4)    // Glättung, RPM Genauigkeit
#endif

// Fuel
// ----
#if _SENS_FUEL_ == 1
#define X1  XP_FUEL
#define MP_FUEL_A         __MTYP(D|U|R|L,   X1, 2)    // Sensor aktivieren
#define MP_FUEL_VOL       __MTYP(D|U    ,   X1, 3)    // Tankgröße
#define MP_FUEL_RES       __MTYP(D|U    ,   X1, 4)    // Volume Reset, man/auto
#define MP_FUEL_CNT       __MTYP(D|U    ,   X1, 5)    // # Impulse je Liter
#define MP_FUEL_CAL       __MTYP(  U    ,   X1, 6)    // # Impulse je Liter kalibrieren
#endif

// Temperatur
// ----------
#if _SENS_TEMP_ == 1
#define X2  XP_TEMP
#define MP_TEMP           __MTYP(D|U|R|L,   X2, 2)    // Sensor Anzeige # aktiver
#define MP_TEMP_TYP       __MTYP(  U    ,   X2, 3)    // Temp Typ (IR/NTC)
#endif

// Altimeter
// ---------
#if _SENS_ALT_ == 1
#define X3  XP_ALT
#define MP_ALT_A          __MTYP(D|U|R|L,   X3, 2)    // Sensor aktivieren
#define MP_ALT_TOT_A      __MTYP(D|U    ,   X3, 3)    // Gesamt Höhe
#if _SENS_ALT_TEMP_ == 1
#define MP_ALT_TEMP       __MTYP(D|U    ,   X3, 4)    // Temperatur
  #define ALT_XNXT 5
#else
  #define ALT_XNXT 4
#endif
#if _SENS_ALT_VARIO_ == 1
#define MP_ALT_VARIO_A    __MTYP(D|U    ,   X3, ALT_XNXT)    // Vario
#define MP_ALT_VARIO_SET  __MTYP(  U    ,   X3, (ALT_XNXT+1))// Vario Filter und Totzone
#endif
#endif

// GPS
// ---
#if _SENS_GPS_ == 1
#define X4  XP_GPS
#define MP_GPS            __MTYP(D|U|R|L,   X4, 2)    // Sensor Anzeige # aktiver
#define MP_GPS_POS_A      __MTYP(D|U    ,   X4, 3)    // Position, Latitude+Longitude aktivieren
#define MP_GPS_ALT_A      __MTYP(D|U    ,   X4, 4)    // Höhe aktivieren
#define MP_GPS_SPEED_A    __MTYP(D|U    ,   X4, 5)    // Speed+Speed max. aktivieren
#define MP_GPS_DIST_TO_A  __MTYP(D|U    ,   X4, 6)    // Entfernung + Richtung zum Modell aktivieren
#define MP_GPS_TRIP_A     __MTYP(D|U    ,   X4, 7)    // zurückgelegte Strecke und Gesamstrecke aktivieren
#define MP_GPS_SATS_A     __MTYP(  U    ,   X4, 8)    // # Sateliten
#endif


// Pitot Rohr, Geschwindigkeit
// ---------------------------
#if _SENS_PITOT_ == 1
#define X5  XP_PITOT
#define MP_PITOT          __MTYP(D|U|R|L,   X5, 2)    // Pitot Speed
#define MP_PITOT_PORT     __MTYP(  U    ,   X5, 3)    // Port
#endif

// Spannung
// --------
#if _SENS_VOLT_ == 1
#define X6  XP_VOLT
#define MP_VOLT           __MTYP(D|U|R|L,   X6, 2)    // Sensor Anzeige # aktiver
#define MP_VOLT_FCT       __MTYP(D|U    ,   X6, 3)    // Funktion, Einzelspg. Messung oder Differenz zwischen den Zellen
#define MP_VOLT_PORT      __MTYP(D|U    ,   X6, 4)    // Spg. Messung, Port
#define MP_VOLT_RGND      __MTYP(D|U    ,   X6, 5)    // Spg. Messung, Widerstand gegen GND
#define MP_VOLT_RVCC      __MTYP(  U    ,   X6, 6)    // Spg. Messung, Widerstand gegen Vcc
#endif


// Stromsensor, Strom/Spannung/Watt 
// --------------------------------
#if _SENS_MUI_ == 1
#define X7  XP_MUI
#define MP_MUI            __MTYP(D|U|R|L,   X7, 2)    // Sensor Anzeige # aktiver
#define MP_MUI_TYP        __MTYP(D|U    ,   X7, 3)    // Stromsensor, Typ Auswahl
#define MP_MUI_PORT_A     __MTYP(D|U    ,   X7, 4)    // Stromsensor, Port für Strom
#define MP_MUI_CAPA_RES   __MTYP(D|U    ,   X7, 5)    // Kapazitäts Reset, man/auto
#define MP_MUI_PORT_V     __MTYP(D|U    ,   X7, 6)    // Stromsensor, Port für Spannung
#define MP_MUI_RGND       __MTYP(D|U    ,   X7, 7)    // Spg. Messung, Widerstand gegen GND
#define MP_MUI_RVCC       __MTYP(  U    ,   X7, 8)    // Spg. Messung, Widerstand gegen Vcc
#endif


// PWM Messung
// -----------
#if _SENS_PWM_ == 1
#define X8  XP_PWM
#define MP_PWM_A          __MTYP(  U|R|L,   X8, 2)     // Sensor aktivieren
#endif


// Beschleunigungsmesser
// ---------------------
#if _SENS_ACC_ == 1
#define X9  XP_ACC
#define MP_ACC            __MTYP(D|U|R|L,   X9, 2)    // Acceleration
#define MP_ACC_GFORCE     __MTYP(D|U    ,   X9, 3)    // G-Force X/Y/Z Achse
#define MP_ACC_CAL        __MTYP(  U    ,   X9, 4)    // G-Force X/Y/Z Achse Kalibrieren
#endif



// ===============
// Reset Sumvalues
// ===============
enum
  {
  YRES_START = 1,
  #if _SENS_FUEL_ == 1
  YRES_FVOL,
  #endif
  #if _SENS_MUI_ == 1
  YRES_CAPA,
  #endif
  #if _SENS_GPS_ == 1
  YRES_SPEED,
  YRES_DIST,
  #endif
  #if _SENS_RPM_ == 1
  YRES_RTIM,
  YRES_REVOL,
  #endif
  #if _SENS_FUEL_ == 1
  YRES_FUEL,
  #endif
  #if _SENS_ALT_VARIO_ == 1 && _SENS_ALT_ == 1
  YRES_ALT,
  #endif

  YRES_LAST
  };

#if _SENS_FUEL_ == 1
#define MP_RESVAL_FVOL    __MTYP(D|U    , 0xf, YRES_FVOL)   // verbrauchte Tankmenge zurücksetzen
#endif
#if _SENS_MUI_ == 1
#define MP_RESVAL_CAPA    __MTYP(D|U    , 0xf, YRES_CAPA)   // MUI1-3, verbrauchte Kapazität zurücksetzen
#endif
#if _SENS_GPS_ == 1
#define MP_RESVAL_SPEED   __MTYP(D|U    , 0xf, YRES_SPEED)  // max. Speed zurücksetzen
#define MP_RESVAL_DIST    __MTYP(D|U    , 0xf, YRES_DIST)   // Gesamt Distanz zurücksetzen
#endif
#if _SENS_RPM_ == 1
#define MP_RESVAL_RTIM    __MTYP(D|U    , 0xf, YRES_RTIM)   // Laufzeit zurücksetzen
#define MP_RESVAL_REVOL   __MTYP(D|U    , 0xf, YRES_REVOL)  // Gesamt Umdrehungen zurücksetzen
#endif
#if _SENS_FUEL_ == 1
#define MP_RESVAL_FUEL    __MTYP(D|U    , 0xf, YRES_FUEL)   // Gesamt Verbrauch zurücksetzen
#endif
#if _SENS_ALT_VARIO_ == 1 && _SENS_ALT_ == 1
#define MP_RESVAL_ALT     __MTYP(D|U    , 0xf, YRES_ALT)    // Gesamt Höhe zurücksetzen
#endif

#define MP_RESVAL_LAST    __MTYP(  U    , 0xf, YRES_LAST)


// =====
// Setup
// =====
#define MP_SETUP_SENS_NB  __MTYP(D|U    , 0xe, 0x2)   // Sensor Index
#define MP_SETUP_WDOG     __MTYP(D|U    , 0xe, 0x3)   // Watchdog aktivieren
#define MP_SETUP_AVCC     __MTYP(D|U    , 0xe, 0x4)   // ADC Referenzspg. in mV
#if _SENS_MUI_ == 1
#define MP_SETUP_MUI_CAL  __MTYP(D|U    , 0xe, 0x5)   // MUI Strom kalibrieren
#define YSUP  5
#else
#define YSUP  4
#endif
#define MP_SETUP_FAC      __MTYP(D|U    , 0xe, (YSUP+1))   // auf default Werte setzten
#define MP_SETUP_INFO     __MTYP(D|U    , 0xe, (YSUP+2))   // Info
#define MP_SETUP_DEBUG    __MTYP(  U    , 0xe, (YSUP+3))




union U_MVAL
  {
  uint16_t Val;
  struct
    {
    uint16_t NC         : 12;
    uint16_t KeyDown    : 1;
    uint16_t KeyUp      : 1;
    uint16_t KeyRight   : 1;
    uint16_t KeyLeft    : 1;
    }s1;
  struct
    {
    uint16_t PosY       : 4;
    uint16_t PosX       : 4;
    uint16_t NC         : 4;
    uint16_t KeyAllowed : 4;
    }s2;
  };

struct S_MENU_ENTRY
  {
  union U_MVAL u;
  char Zeile1[15];
  };

const struct S_MENU_ENTRY Menu[] PROGMEM =
  {
  //                     12345678901234
  { MP_MAIN,            "              " },
  { MP_SENSCFG,         ":Sensor config" },
  { MP_RESVAL,          ":Reset Values " },
  { MP_SETUP,           ":Setup/Info   " },

  #if _SENS_RPM_ == 1
  { MP_RPM_A,           ":Sensor RPM <>" },
  { MP_RPM_RELCNT,      ".Release cnt >" }, 
  { MP_RPM_ACCURACY,    ".Accuracy    >" }, 
  #endif

  #if _SENS_FUEL_ == 1
  { MP_FUEL_A,          ":Sensor Fuel<>" }, 
  { MP_FUEL_VOL,        ".Fuel vol. <->" },
  { MP_FUEL_RES,        ".Volume reset>" },
  { MP_FUEL_CNT,        ".pulse/L   <->" },
  { MP_FUEL_CAL,        ".cal pulse/L<>" },
  #endif

  #if _SENS_TEMP_ == 1
  { MP_TEMP,            ":Sensor Temp  " },
  { MP_TEMP_TYP,        ".Temp Typ  <->" },
  #endif

  #if _SENS_ALT_ == 1
  { MP_ALT_A,           ":Sensor Alt <>" },
  { MP_ALT_TOT_A,       ".Sens AltTot<>" },
  #if _SENS_ALT_TEMP_ == 1
  { MP_ALT_TEMP,        ".Sens Temp  <>" },
  #endif
  #if _SENS_ALT_VARIO_ == 1
  { MP_ALT_VARIO_A,     ".Sens Vario <>" },
  { MP_ALT_VARIO_SET,   ".Filt   Deadz." },
  #endif
  #endif

  #if _SENS_GPS_ == 1
  { MP_GPS,             ":Sensor GPS   " },
  { MP_GPS_POS_A,       ".Sens Pos   <>" },
  { MP_GPS_ALT_A,       ".Sens Alt   <>" },
  { MP_GPS_SPEED_A,     ".Sens Speed <>" },
  { MP_GPS_DIST_TO_A,   ".Sens DistTo<>" },
  { MP_GPS_TRIP_A,      ".Sens Trip  <>" },
  { MP_GPS_SATS_A,      ".Sens Sats  <>" },
  #endif

  #if _SENS_PITOT_ == 1
  { MP_PITOT,           ":Sens Pitot   " },
  { MP_PITOT_PORT,      ".Port      <->" },
  #endif

  #if _SENS_VOLT_ == 1
  { MP_VOLT,            ":Sens Voltage " },
  { MP_VOLT_FCT,        ".Function  <->" },
  { MP_VOLT_PORT,       ".Port      <->" },
  { MP_VOLT_RGND,       ".Resistor  <->" },
  { MP_VOLT_RVCC,       ".Resistor  <->" },
  #endif

  #if _SENS_MUI_ == 1
  { MP_MUI,             ":Sens MUI tot " },
  { MP_MUI_TYP,         ".Typ ACS   <->" },
  { MP_MUI_PORT_A,      ".Port curr <->" },
  { MP_MUI_CAPA_RES,    ".capa reset<->" },
  { MP_MUI_PORT_V,      ".Port volt <->" },
  { MP_MUI_RGND,        ".Resistor  <->" },
  { MP_MUI_RVCC,        ".Resistor  <->" },
  #endif

  #if _SENS_PWM_ == 1
  { MP_PWM_A,           ":Sensor PWM <>" },
  #endif

  #if _SENS_ACC_ == 1
  { MP_ACC,             ":Sensor Accel " },
  { MP_ACC_GFORCE,      ".Sens GForce<>" },
  { MP_ACC_CAL,         ".calib GForc<>" },
  #endif

    

  // Reset SumValues
  #if _SENS_FUEL_ == 1
  { MP_RESVAL_FVOL,     ".Fuel volume<>" },
  #endif
  #if _SENS_MUI_ == 1
  { MP_RESVAL_CAPA,     ".Capacity   <>" },
  #endif
  #if _SENS_GPS_ == 1
  { MP_RESVAL_SPEED,    ".Speed max  <>" },
  { MP_RESVAL_DIST,     ".Dist total <>" },
  #endif
  #if _SENS_RPM_ == 1
  { MP_RESVAL_RTIM,     ".RunTime    <>" },
  { MP_RESVAL_REVOL,    ".Revolution <>" },
  #endif
  #if _SENS_FUEL_ == 1
  { MP_RESVAL_FUEL,     ".Fuel total <>" },
  #endif
  #if _SENS_ALT_VARIO_ == 1 && _SENS_ALT_ == 1
  { MP_RESVAL_ALT,      ".Alt total  <>" },
  #endif
  { MP_RESVAL_LAST,     ".thats all cya" },

  
  // Setup
  { MP_SETUP_SENS_NB,   ".Sensor Nb   >" },
  { MP_SETUP_WDOG,      ".Watchdog   <>" },
  { MP_SETUP_AVCC,      ".ADC RefVcc<->" },
  #if _SENS_MUI_ == 1
  { MP_SETUP_MUI_CAL,   ".MUI calib <->" },
  #endif
  { MP_SETUP_FAC,       ".factory def<>" },
  { MP_SETUP_INFO,      ".(c)Th.Lehmann" },
  { MP_SETUP_DEBUG,     ".Debug        " },

  };





// =======================
// JetiBox Tasten bewerten
// =======================
// wird min. alle 10mS aufgerufen
void HandleJB( bool Refresh )
{
 #if _SENS_FUEL_ == 1
 static bool FuelCalActiv; 
 #endif
 static uint8_t PosX, PosY, MenuIdx;        // Index in Menu Tabelle, aktueller Eintrag
 static uint8_t LastPosX, LastPosY;
 union U_MVAL MVal;                         // Pos und zugelassene Richtungen, Kopie aus Flash
 uint8_t Key, Idx;

 static uint8_t Index;
 static bool Dir;

 char    JB_Buff[17];                       // JB Ausgabe, Zeile 1/2
  #define  JBPOS_TXT  8                     // kleiner Text (5Ch 8-12)
  #define  JBPOS_CHK  15                    // Pos Sensortestausgabe in Zeile 2
  #define  JBPOS_INIT 14                    // Pos Init Fehler in Zeile 2
  #define  JBPOS_FAIL 15                    // Pos Ausfall in Zeile 2

  if( Refresh )
    Key = JBKEY_REFRESH;                     // Aufruf um Anzeige zu aktualisieren
  else
    #if HW_TERM == 1 || SOFT_TERM == 1
    Key = ConGetKey();                       // liefert 0 wenn nichts gedrückt
    #else
    // JB Empfang, Sender liefert zu schnell Tastenwiederholungen wenn schon etwas zu lange gedrückt
    // bei Jeti EX trubeln alle 150mS Daten (Tasten) ein
    if( !JetiEx.GetJetiboxKey(&Key) )        // liefert false wenn nichts empfangen
      return;
    static uint8_t KeyLast;
    // if( c == 0x70 || c == 0xb0 || c == 0xd0 || c == 0xe0 ) // Left = 0x70, down = 0xb0, up= 0xd0, right = 0xe0
    if( Key != 0xf0 && (Key & 0x0f) == 0 )   // check upper nibble
      {
      if( Key == KeyLast )                    // Entprellen, Tastenwiederholung verwerfen
        return;
      }
    KeyLast = Key;

      /*
      static uint32_t LastKeyMS;
      if( millis() < LastKeyMS )
        return;
      LastKeyMS = millis() + 300;
      */
    #endif
  if( Key == 0 )
    return;

  SensorChk.Id = -1;                         // Sensor Funktionstest ggf. ausschalten


  // akt. Eintrag für Zugriff aus Flash ins RAM kopieren
  memcpy_P( &MVal, &Menu[MenuIdx].u, sizeof(MVal) );

  switch( Key )
    {
    case JBKEY_LEFT:                // links
      if( MVal.s1.KeyLeft )
        {
        if( PosY == 1 )             // Hauptmenu, Richtung tauschen
          PosX++;
        else
          PosX--;
        }
      break;
    case JBKEY_RIGHT:               // rechts
      if( MVal.s1.KeyRight )
        {
        if( PosY == 1 )             // Hauptmenu, Richtung tauschen
          PosX--;
        else if( PosX < XP_LAST-1 ) // Ende noch nicht erreicht ?
          PosX++;
        }
      break;
    case JBKEY_UP:                  // hoch
      if( MVal.s1.KeyUp )
        {
        PosY--;
        // bei Übergang in MAIN oder Übergang aus irgendeinem SETUP Submenu
        if( __MPOSY(MP_MAIN) == PosY || (__MPOSY(MP_SENSCFG) == PosY && PosX < __MPOSX(MP_HM_LAST)) )
          PosX = 0;
        }
      break;
    case JBKEY_DOWN:                // runter
      if( MVal.s1.KeyDown )
        PosY++;
      break;

    case JBKEY_LR:                  // links+rechts
    case JBKEY_UD:                  // Hoch+Runter
    case JBKEY_REFRESH:
      break;
    default:
      return;
    }
  PosX &= 0xf;  // bei Wechsel von Setup auf Service

  Idx = ARRAYCNT(Menu);
  uint8_t MPos = (PosX<<4)|PosY;
  while( MPos != (MVal.Val & 0xff) )                // Positionsänderung, neuen Menueintrag suchen
    {
    #if _SENS_FUEL_ == 1
    FuelCalActiv = false;                           // FuelFlow Kalibrierung ohne speichern beenden, Cal-Menue wurde verlassen
    #endif

    MenuIdx++;
    if( MenuIdx >= ARRAYCNT(Menu) )
      MenuIdx = 0;
    memcpy_P( &MVal, &Menu[MenuIdx].u, sizeof(MVal) );  // aus Flash ins RAM kopieren
    if( Idx-- == 0 )
      {
      DbgPrintf("MPosXY:%x nicht gefunden\n", MPos);
      PosX = LastPosX;
      PosY = LastPosY;
      return;
      }
    }
   LastPosX = PosX;
   LastPosY = PosY;

  #if HW_TERM == 1 || SOFT_TERM == 1
  JB_Buff[sizeof(JB_Buff)-1] = 0;                   // Strg Ende nur für DbgPrintf() notwendig
  #endif
  // Menutext Zeile 1
  if( MVal.Val == MP_MAIN )                         // Eintritt bzw. Konfig geändert, speichern/verwerfen
    {
    if( EEpromSave )                                // bei Austritt fragen ob Änderungen gespeichert werden sollen
      strcpy_P( JB_Buff, PSTR("config changed") );
    else
      strcpy_P( JB_Buff, JB_TEXT_Z1 );
    }
  else
    {
    sprintf_P(JB_Buff, PSTR("%02X"), MPos);
    memcpy_P( &JB_Buff[2], &Menu[MenuIdx].Zeile1, sizeof(Menu[MenuIdx].Zeile1) );
    }
  JetiEx.SetJetiboxText( JetiExProtocol::LINE1, JB_Buff );
  memset( JB_Buff, ' ', sizeof(JB_Buff)-1 );


  // ================
  // Auswahl bedienen
  // ================
  int8_t  IdActiv = -1;
  bool    ErrFail = false;
  const char *Txt = "";
  uint8_t PosJB = 0;
  uint32_t Val = 0;

// 0x12 war Pfeil hoch/runter, Zeichen unter 0x20 gehen mit 23er nicht mehr :(
// StrgExitSave[] = "\x12 exit / save <>";
  switch( MVal.Val )
    {
    case MP_MAIN:                                   // Eintritt bzw. Konfig geändert, speichern/verwerfen
      strcpy_P( JB_Buff, JB_TEXT_Z2 );
      if( GData.EEprom.Data.SensNb )                // Sensor Nr eintragen wenn konfiguriert
        JB_Buff[JB_POS_Z2_SENSNB] = '0' + GData.EEprom.Data.SensNb;
      if( EEpromSave )                              // bei Austritt fragen ob Änderungen gespeichert werden sollen
        {
        strcpy_P( JB_Buff, PSTR("| exit / save <>") );
        if( Key == JBKEY_LR )                       // Rechts+Links > Änderungen speichern
          EEpromRW( true, false );
        else if( Key != JBKEY_UD )                  // Hoch+Runter > Änderungen verwerfen / Reset
          break;
        SoftReset();                                // Neustart mit geänderter oder alter Konfig
        }
      break;


    
    // =====
    // Setup
    // =====
    case MP_SETUP:
      Index = Dir = 0;
      break;
    case MP_SETUP_DEBUG:
      sprintf_P(JB_Buff, PSTR("free RAM:%u"), freeRam());
      break;

    case MP_SETUP_INFO:
                           // 1234567890123456
      strcpy_P(JB_Buff, PSTR(" thomas@thls.de"));
      break;
    
    case MP_SETUP_SENS_NB:        // Sensor Nummer festlegen, 0 = ohne, 1-9
      if( Key == JBKEY_RIGHT )    // Änderung ?
        {
        GData.EEprom.Data.SensNb++;
        EEpromSave = true;
        }
      sprintf_P(JB_Buff, PSTR("#%d"), GData.EEprom.Data.SensNb);
      break;
    
    case MP_SETUP_WDOG:
      if( Key == JBKEY_LR )      // Änderung ?
        {
        GData.EEprom.Data.WDogActiv ^= true;
        EEpromSave = true;
        }
      sprintf_P( JB_Buff, PSTR("%sactiv"), GData.EEprom.Data.WDogActiv ? "" : "de");
      break;

    case MP_SETUP_AVCC:           // ADC Referenzspg. in mV
      _JBCfgValue( Key, &GData.EEprom.Data.AVcc, 5 );
      sprintf_P(JB_Buff, PSTR("%umV"), GData.EEprom.Data.AVcc);
      break;

    #if _SENS_MUI_ == 1
    case MP_SETUP_MUI_CAL:        // MUI Strom kalibrieren
      #define CFG_MUI   GData.EEprom.Data.CfgMuiCalib[Index]
      {
      // Dir == 0 > MUI 1-n einstellen
      // Dir == 1 > Kalibrierwert einstellen
      // Key == LR> zwischen 0 und 1 für Dir wechseln
      int Pos = _JBDirIdx(&Dir, Key, JB_Buff, "MUI", &Index, CNT_SENS_MUI, false, &CFG_MUI, 250, true );
      if( SensPwrMui[Index] )
        sprintf_P( &JB_Buff[Pos], PSTR("%ldmA"), SensPwrMui[Index]->GetCurrent(true));
      break;
      }
      #undef CFG_MUI
    #endif


    case MP_SETUP_FAC:
      if( Key == JBKEY_LR )
        {
        GData.EEprom.Data.Magic = 0xffff;
        EEpromRW( true, false );
        SoftReset();
        }
      break;

   
    // ===============
    // Reset Sumvalues
    // ===============
    case MP_RESVAL:
      Index = 0;
      break;

    uint32_t *NV_Val;
    #if _SENS_FUEL_ == 1
    case MP_RESVAL_FVOL:                 // verbrauchte Tankmenge zurücksetzen
      if( SensFuel )
        {
        if( Key == JBKEY_LR )
          SensFuel->ResetFlowCntTank();
        NV_Val = &GData.EEprom.NV.FuelFlowCntTank;
        Val = SensFuel->TVal.VolumeRemain;
        Txt = "ml";
        goto RESET_NV_VAL;
        }
      break;
      #endif
    #if _SENS_MUI_ == 1
    case MP_RESVAL_CAPA:               // MUI1-3 verbrauchte Kapazität zurücksetzen
      // MUI 1-n einstellen
      Index = _JBIdxPlusMinKey( Key, Index, CNT_SENS_MUI, false );
      sprintf_P( JB_Buff, PSTR("MUI%d "), Index+1);
      PosJB = 5;
      NV_Val = (uint32_t*)&GData.EEprom.NV.MuiCapaConsumed[Index];
      Val = GData.EEprom.NV.MuiCapaConsumed[Index];
      Txt = "mAh";
      goto RESET_NV_VAL;
    #endif
    #if _SENS_GPS_ == 1
    case MP_RESVAL_SPEED:              // max. Speed zurücksetzen
      NV_Val = &GData.EEprom.NV.GpsSpeedMax;
      goto RESET_NV_VAL;
    case MP_RESVAL_DIST:               // Gesamt Distanz zurücksetzen
      NV_Val = &GData.EEprom.NV.GpsDistTot;
      goto RESET_NV_VAL;
    #endif
    #if _SENS_RPM_ == 1
    case MP_RESVAL_RTIM:               // Laufzeit zurücksetzen
      NV_Val = &GData.EEprom.NV.RpmRunTimeS;
      goto RESET_NV_VAL;
    case MP_RESVAL_REVOL:              // Gesamt Umdrehungen zurücksetzen
      NV_Val = &GData.EEprom.NV.RpmRevolTot;
      goto RESET_NV_VAL;
    #endif
    #if _SENS_FUEL_ == 1
    case MP_RESVAL_FUEL:               // Gesamt Verbrauch zurücksetzen
      NV_Val = &GData.EEprom.NV.FuelFlowCntTot;
      goto RESET_NV_VAL;
    #endif
    #if _SENS_ALT_VARIO_ == 1 && _SENS_ALT_ == 1
    case MP_RESVAL_ALT:
      NV_Val = &GData.EEprom.NV.AltitudeTot;
      goto RESET_NV_VAL;

    case MP_RESVAL_LAST:
      break;
    #endif

RESET_NV_VAL:
      if( Key == JBKEY_LR )
        {
        *NV_Val = 0L;
        EEpromRW( true, false );
        GData.Signal |= SIG_JB_REFRESH;
        break;
        }
      if( Val == 0 )
        Val = *NV_Val;
      sprintf_P( &JB_Buff[PosJB], PSTR("%lu%s"), Val, Txt);
      break;



    // =============
    // Sensor Konfig
    // =============
    case MP_SENSCFG:
      break;

    // Drehzahl
    // --------
    #if _SENS_RPM_ == 1
    case MP_RPM_A:                                  // Sensor aktivieren
      IdActiv = ID_RPM;
      if( Key == JBKEY_LR )                         // Änderung ?
        {
        PBIT_TGL( GData.EEprom.Data.SensActivMsk, ID_RPM_REVOL );
        PBIT_TGL( GData.EEprom.Data.SensActivMsk, ID_RPM_RUNTIME );
        }
      SensorChk.Id = ID_RPM;                        // Sensorcheck aktiv, Änderungen anzeigen
      break;
    case MP_RPM_RELCNT:                             // # Auslösungen je Umdrehung
      if( Key == JBKEY_RIGHT )
        {
        GData.EEprom.Data.CfgRpmCntMag++;
        GData.EEprom.Data.CfgRpmCntMag &= 3;
        EEpromSave = true;
        }
                             // 1234567890123456
      sprintf_P( JB_Buff, PSTR("%u / revolution"), GData.EEprom.Data.CfgRpmCntMag+1);
      break;

    case MP_RPM_ACCURACY:                           // RPM Anzeige, Genauigkeit n UPM
      _JBCfgValue( Key, (uint16_t *)&GData.EEprom.Data.CfgRpmAccuracy, 10 );
      if( GData.EEprom.Data.CfgRpmAccuracy > RPM_ACCURACY_MAX )
        GData.EEprom.Data.CfgRpmAccuracy = RPM_ACCURACY_MAX;
                      // 1234567890123456
      sprintf_P( JB_Buff, PSTR("%u RPM/min"), GData.EEprom.Data.CfgRpmAccuracy ? GData.EEprom.Data.CfgRpmAccuracy : 1);
      break;
    #endif



    // Fuel
    // ----
    #if _SENS_FUEL_ == 1
    case MP_FUEL_A:                                 // Sensor aktivieren
      IdActiv = ID_FUEL_REMAIN;
      if( Key == JBKEY_LR )                         // Änderung ?
        {
        PBIT_TGL( GData.EEprom.Data.SensActivMsk, ID_FUEL_CONSUMED  );
        PBIT_TGL( GData.EEprom.Data.SensActivMsk, ID_FUEL_FLOW  );
        PBIT_TGL( GData.EEprom.Data.SensActivMsk, ID_FUEL_TOT );
        }
      SensorChk.Id = ID_FUEL_FLOW;                  // Sensorcheck aktiv, Änderungen anzeigen
      break;
    case MP_FUEL_VOL:                               // Tankgröße
      _JBCfgValue( Key, &GData.EEprom.Data.CfgFuelVolume, 50 );
      sprintf_P( JB_Buff, PSTR("%umL"), GData.EEprom.Data.CfgFuelVolume);
      break;
    case MP_FUEL_RES:                               // Volume Reset, man/auto
      #define CFG_FUEL GData.EEprom.Data.CfgFuelVolRes
      CFG_FUEL = _JBGetResFct( Key, JB_Buff, 1, CFG_FUEL );
      break;
    #undef CFG_FUEL


    case MP_FUEL_CNT:                               // # Impulse je Liter
      _JBCfgValue( Key, &GData.EEprom.Data.CfgFuelFlowCnt1L, 10 );

      if( Key == JBKEY_LR )                         // Reset auf default # Pulse
        {
        GData.EEprom.Data.CfgFuelFlowCnt1L = FUEL_FLOWCNT_1L;
        EEpromSave = true;
        }
                           // 1234567890123456
      sprintf_P( JB_Buff, PSTR("%5u/L    def<>"), GData.EEprom.Data.CfgFuelFlowCnt1L);
      break;

    case MP_FUEL_CAL:                               // # Impulse je Liter kalibrieren
      if( Key == JBKEY_LR )                         // Start/Stop
        FuelCalActiv ^= true;
      // zyklischer Aufruf von SensorChk bei Änderungen
      if( FuelCalActiv )
        {
        SensorChk.Id = ID_FUEL_FLOW;  // Sensorcheck aktiv, Änderungen summieren
        if( SensorChk.MesureVal )//1234567890123456
          sprintf_P( JB_Buff, PSTR("pulse cnt %5u"), SensorChk.MesureVal);
        else                     // 1234567890123456
          sprintf_P( JB_Buff, PSTR("start flow 1L"));
        }
      else if( SensorChk.MesureVal > 1000L )   // Ende der Messung, speichern
        {
        GData.EEprom.Data.CfgFuelFlowCnt1L = SensorChk.MesureVal;
        EEpromSave = true;     // 1234567890123456
        sprintf_P( JB_Buff, PSTR("pulse cnt saved"));
        }
      break;
    #endif

    // Temperatur
    // ----------
    #if _SENS_TEMP_ == 1
    case MP_TEMP:                                 // Sensor Anzeige # aktiver
      _JBPutSensActivCnt( &JB_Buff[3], ID_TEMP1, ID_TEMP1+CNT_SENS_TEMP-1, false );
      Index = Dir = 0;
      break;
    case MP_TEMP_TYP:                             // Temp Typ (IR/NTC)
      #define CFG_TEMP GData.EEprom.Data.CfgSensTypTemp[Index]
      {
      const char *TypTxt[] = STYP_TEMP_TXT;
      uint8_t CntSTyp = CNT_STYP_TEMP;
      if( Index >= CNT_SENS_TEMP_NTC )
        CntSTyp--;                                // nur 4 NTC aber 7 IR
      int Pos = _JBDirIdx(&Dir, Key, JB_Buff, "TEMP", &Index, CNT_SENS_TEMP, false, &CFG_TEMP, CntSTyp, true );
      sprintf_P(&JB_Buff[Pos], PSTR("%s"), TypTxt[CFG_TEMP]);

      IdActiv = (ID_TEMP1 + Index);
      if( CFG_TEMP == STYP_TEMP_NONE )
        PBIT_CLR( GData.EEprom.Data.SensActivMsk, IdActiv );
      else
        PBIT_SET( GData.EEprom.Data.SensActivMsk, IdActiv );
      IdActiv |= 0x80;                             // ohne "activ/deactiv" Anzeige

      if( SensTemp[Index] )
        ErrFail = SensTemp[Index]->Failure;
      break;
      }
    #undef CFG_TEMP
    #endif



    // Altimeter
    // ---------
    #if _SENS_ALT_ == 1
    case MP_ALT_A:                                  // Sensor aktivieren
      IdActiv = ID_ALT_REL;
      if( SensAlt )
        ErrFail = SensAlt->Failure;
      break;
    case MP_ALT_TOT_A:                              // Gesamt Höhe
      IdActiv = ID_ALT_TOT;
      break;
    #if _SENS_ALT_TEMP_ == 1
    case MP_ALT_TEMP:                               // Temperatur
      IdActiv = ID_ALT_TEMP;
      break;
    #endif
    #if _SENS_ALT_VARIO_ == 1
    case MP_ALT_VARIO_A:                            // Vario
      IdActiv = ID_ALT_VARIO;
      Dir = 0;
      break;
    case MP_ALT_VARIO_SET:                          // Vario Filter und Totzone
      #define CFG_FILT   GData.EEprom.Data.CfgVarioSmoothing
      #define CFG_DZ     GData.EEprom.Data.CfgVarioDeadzone
      {
      // Dir == 0 > Smoothing Filter 0-100% einstellen
      // Dir == 1 > Totzone 0- einstellen
      // Key == LR> zwischen 0 und 1 für Dir wechseln
      CFG_FILT--;                                   // Anzeige erfolgt mit +1
      int Pos = 3;
      Pos += _JBDirIdx(&Dir, Key, &JB_Buff[Pos], "% ", &CFG_FILT, 99, true, &CFG_DZ, 100, true );
      sprintf_P( &JB_Buff[Pos], PSTR("0.%02u m/s"), CFG_DZ);
      CFG_FILT++;
      break;
      }
      #undef CFG_FILT
      #undef CFG_DZ
      break;
    #endif
    #endif
    

    
    // Pitot Rohr, Speedmessung
    // ------------------------
    #if _SENS_PITOT_ == 1
    case MP_PITOT:                                  // Speed Messung
      IdActiv = ID_PITOT_SPEED;
      Key = 0;
      break;
    case MP_PITOT_PORT:                             // Port Auswahl
      #define PA  GData.EEprom.Data.CfgAnaFct[ANAFCT_PITOT]
      _JBPortAnaChoice( JB_Buff, ID_PITOT_SPEED, &PA, Key );
      EEpromSave = true;
      break;
      #undef PA
    #endif


    // Spannung
    // --------
    #if _SENS_VOLT_ == 1
    case MP_VOLT:                                 // Sensor Anzeige # aktiver
      _JBPutSensActivCnt( &JB_Buff[3], ID_PWR_VOLT1, ID_PWR_VOLT1 + CNT_SENS_VOLT-1, false );
      Index = Dir = 0;
      break;

    case MP_VOLT_FCT:                             // Funktion, Einzelspg. Messung oder Differenz zwischen den Zellen
      if( Key == JBKEY_LR )                       // Änderung ?
        {
        GData.EEprom.Data.CfgVoltFct ^= true;
        EEpromSave = true;
        }
      sprintf_P(JB_Buff, PSTR("%s"), GData.EEprom.Data.CfgVoltFct == CFG_VOLT_FCT_SINGLE ? "singles" : "diff");
      break;

    case MP_VOLT_PORT:                            // Spg. Messung, Port
      #define PA  GData.EEprom.Data.CfgAnaFct[ANAFCT_VOLT1 + Index]
      {
      int Pos = _JBDirIdx(&Dir, Key, JB_Buff, "VOLT", &Index, CNT_SENS_VOLT, false, 0, 0, -1 );
      if( Dir == 0 )                              // Port Auswahl selektiert
        Key = 0;                                  // nur Refresh Analog Port Anzeige
      _JBPortAnaChoice( &JB_Buff[Pos], ID_PWR_VOLT1 + Index, &PA, Key );
      RStepFine = false;                          // Widerstandänderung, große Schrittweite
      break;
      }
      #undef PA

    case MP_VOLT_RGND:                            // Spg. Messung, Widerstand gegen GND
    case MP_VOLT_RVCC:                            // Spg. Messung, Widerstand gegen Vcc
      #define RGND  GData.EEprom.Data.CfgVoltRGnd[Index]
      #define RVCC  GData.EEprom.Data.CfgVoltRVcc[Index]
      _JBCfgResistor( &JB_Buff[0], (MVal.Val - MP_VOLT_RGND)&1, &RGND, &RVCC, Key );
      break;
      #undef RGND
      #undef RVCC
    #endif



    // Stromsensor ACSxx
    // -----------------
    #if _SENS_MUI_ == 1
    case MP_MUI:                                  // Sensor Anzeige # aktiver
      {
      int Pos = 3;
      Pos += _JBPutSensActivCnt( &JB_Buff[Pos], ID_PWR_MUI1_A, ID_PWR_MUI1_A+CNT_SENS_MUI-1, false );
      JB_Buff[Pos] = '/';
      _JBPutSensActivCnt( &JB_Buff[Pos+1], _ID_MUI_FIRST, _ID_MUI_LAST, true );
      Index = 0;
      break;
      }

    case MP_MUI_TYP:                              // Stromsensor Typ
      #define CFG_MUI GData.EEprom.Data.CfgMuiSensTyp[Index]
      {
      int Pos = _JBDirIdx(&Dir, Key, JB_Buff, "MUI", &Index, CNT_SENS_MUI, false, 0, 0, -1 );
      if( Dir == 0 )                              // MUI Index Auswahl selektiert
        Key = 0;                                  // nur Refresh Typ Anzeige
      CFG_MUI = _JBIdxPlusMinKey( Key, CFG_MUI, CNT_STYP_MUI, true );
      sprintf_P( &JB_Buff[Pos], Acs[CFG_MUI].Text);
      if( CFG_MUI == 0 )                          // Sensor Text ist zu lang, 'I' stört dann
        IdActiv = (ID_PWR_MUI1_A + Index) | 0x80; // Init Fehler anzeigen aber ohne "activ/deactiv" Anzeige
      break;
      }
    case MP_MUI_PORT_A:                           // vewendeter AnalogPort für Ampere
      #define PA  GData.EEprom.Data.CfgAnaFct[ANAFCT_MUI1_A + Index]
      {
      if( CFG_MUI == STYP_MUI_ACS_NONE )          // kein Sensor Typ gewählt ?
        {
        PosY--;
        GData.Signal |= SIG_JB_REFRESH;
        break;
        }
      // wenn Ampere aktiv können wir auch Kapazität berechnen
      if( _JBPortAnaChoice(JB_Buff, (ID_PWR_MUI1_A + Index), &PA, Key) )
        PBIT_SET( GData.EEprom.Data.SensActivMsk, ID_PWR_MUI1_C + Index );
      else
        PBIT_CLR( GData.EEprom.Data.SensActivMsk, ID_PWR_MUI1_C + Index );
      break;
      }
      #undef PA
      #undef CFG_MUI

    case MP_MUI_CAPA_RES:                         // Stromsensor, Kapazitäts Reset
      #define CFG_MUI GData.EEprom.Data.CfgMuiCapaRes[Index]
      CFG_MUI = _JBGetResFct( Key, JB_Buff, 5, CFG_MUI ); // auto + 1-4%
      if( CFG_MUI > RESFCT_AUTO )                 // Auto = immer, oder Auto mit 1-4% höherer Spg.
        sprintf_P(&JB_Buff[RESFCT_POSAUTO], PSTR("%d%%"), CFG_MUI - RESFCT_AUTO);
      break;
      #undef CFG_MUI

    case MP_MUI_PORT_V:                           // vewendeter AnalogPort für Spannung
      RStepFine = false;                          // Widerstandänderung, große Schrittweite
      #define PA  GData.EEprom.Data.CfgAnaFct[ANAFCT_MUI1_V + Index]
      // Ampere und Spannung > dann auch Watt
      if( _JBPortAnaChoice(JB_Buff, (ID_PWR_MUI1_V + Index), &PA, Key) )
        PBIT_SET( GData.EEprom.Data.SensActivMsk, ID_PWR_MUI1_W + Index );
      else
        PBIT_CLR( GData.EEprom.Data.SensActivMsk, ID_PWR_MUI1_W + Index );
      break;
      #undef PA
    case MP_MUI_RGND:                            // Spg. Messung, Widerstand gegen GND
    case MP_MUI_RVCC:                            // Spg. Messung, Widerstand gegen Vcc
      #define RGND  GData.EEprom.Data.CfgMuiRGnd[Index]
      #define RVCC  GData.EEprom.Data.CfgMuiRVcc[Index]
      _JBCfgResistor( &JB_Buff[0], (MVal.Val - MP_MUI_RGND)&1, &RGND, &RVCC, Key );
      break;
      #undef RGND
      #undef RVCC
    #endif



    
    #if _SENS_GPS_ == 1
    // GPS
    // ---
    case MP_GPS:                                    // Sensor Anzeige # aktiver
      _JBPutSensActivCnt( &JB_Buff[3], _ID_GPS_FIRST, _ID_GPS_LAST, false );
      if( SensGps )
        ErrFail = SensGps->Failure;
      break;

    case MP_GPS_POS_A:           // Position Latitude+Longitude aktivieren
      IdActiv = ID_GPS_LAT;
      if( Key == JBKEY_LR )      // Änderung ?
        {
        PBIT_TGL( GData.EEprom.Data.SensActivMsk, ID_GPS_LON );
        PBIT_TGL( GData.EEprom.Data.SensActivMsk, ID_GPS_HEAD );
        }
      break;
    case MP_GPS_ALT_A:           // Höhe aktivieren
      IdActiv = ID_GPS_ALT;
      break;
    case MP_GPS_SPEED_A:         // Speed+max. aktivieren
      IdActiv = ID_GPS_SPEED;
      if( Key == JBKEY_LR )      // Änderung ?
        PBIT_TGL( GData.EEprom.Data.SensActivMsk, ID_GPS_SPEED_MAX );
      break;
    case MP_GPS_DIST_TO_A:       // Entfernung+Richtung aktivieren
      IdActiv = ID_GPS_DIST_TO;
      if( Key == JBKEY_LR )      // Änderung ?
        PBIT_TGL( GData.EEprom.Data.SensActivMsk, ID_GPS_COURSE_TO );
      break;
    case MP_GPS_TRIP_A:           // zurückgelegte und Gesamt Strecke aktivieren
      IdActiv = ID_GPS_DIST_TRIP;
      if( Key == JBKEY_LR )      // Änderung ?
        PBIT_TGL( GData.EEprom.Data.SensActivMsk, ID_GPS_DIST_TOT );
      break;
    case MP_GPS_SATS_A:          // # Sateliten
      IdActiv = ID_GPS_SATS;
      break;
    #endif



    // PWM Messung
    // -----------
    #if _SENS_PWM_ == 1
    case MP_PWM_A:                                  // Sensor aktivieren
      IdActiv = ID_PWM;
      break;
    #endif

    
    
    #if _SENS_ACC_ == 1
    // Beschleunigungsmesser
    // ---------------------
    case MP_ACC:                  // Acceleration
      //_JBPutSensActivCnt( JB_Buff, _ID_ACC_FIRST, _ID_ACC_LAST );
      //SensAcc->Failure
      break;
    case MP_ACC_GFORCE:           // G-Force X/Y/Z Achse
      IdActiv = ID_ACC_GFORCE_X;
      if( Key == JBKEY_LR )       // Änderung ? G-Force immer für alle Achsen
        {
        PBIT_TGL( GData.EEprom.Data.SensActivMsk, ID_ACC_GFORCE_Y );
        PBIT_TGL( GData.EEprom.Data.SensActivMsk, ID_ACC_GFORCE_Z );
        }
      break;
    case MP_ACC_CAL:             // G-Force X/Y/Z Achse Kalibrieren
      // G-Force nicht aktiv, Calib überspringen 
      if( !PBIT_TST(GData.EEprom.Data.SensActivMsk, ID_ACC_GFORCE_X) )
        {                      // 1234567890123456
        sprintf_P( JB_Buff, PSTR("not available") );
        }
      break;
    #endif


    default:
      sprintf_P(JB_Buff, PSTR("?? MVal:%x ??"), MVal.Val);
      break;
    }


  
  // Sensor Akt/Deaktivierung
  // ========================
  if( IdActiv != -1 )
    {
    uint8_t IdA80 = IdActiv & 0x80;   // 0x80 = keine "activ" Ausgabe
    IdActiv &= 0x7f;
    if( IdA80 )
      Key = 0;
    
    if( Key == JBKEY_LR )         // Änderung ?
      {
      PBIT_TGL( GData.EEprom.Data.SensActivMsk, IdActiv );
      EEpromSave = true;
      }
    if( IdA80 == 0 )
      sprintf_P( JB_Buff, PSTR("%sactiv%s"), PBIT_TST(GData.EEprom.Data.SensActivMsk, IdActiv) ? "" : "de", Txt);
    if( SensorChk.Id != -1 )                      // Sensor Funktionstest, Anzeige 0-9 bei jeder Änderung 
      JB_Buff[JBPOS_CHK] = (SensorChk.MesureVal%10) | '0';
    // Sensor bereits aktiviert konnte aber nicht initialisiert werden ?
    if( PBIT_TST(Tele.ErrInit, IdActiv) )
      JB_Buff[JBPOS_INIT] = 'I';                  // Sensor init Fehler
    }

  if( ErrFail )                                   // Ausfall zur Laufzeit
    JB_Buff[JBPOS_FAIL] = 'F';

  // alle \0 durch Space ersetzen
  for( Idx = 0; Idx < sizeof(JB_Buff)-1; Idx++ )
    if( JB_Buff[Idx] == 0 )
      JB_Buff[Idx] = ' ';
  JetiEx.SetJetiboxText( JetiExProtocol::LINE2, JB_Buff );


  #if HW_TERM == 1
  // Cursor 2 Zeilen hoch
  Serial.write(27);
  Serial.write('M');
  Serial.write(27);
  Serial.write('M');
  #endif

//  if( SensTstCh )                                 // Sensor Funktionstest
//    JB_Buff[JBPOS_TEST] = SensTstCh;
}



// =============================================
// Tastencode von Console in Jetibox Key wandeln
// =============================================
#if HW_TERM == 1 || SOFT_TERM == 1

  #if HW_TERM == 1
#define TermGet Serial.read
  #else
//#define TermGet TermSoftSerial->read
#define TermGet JetiEx.GetJetiboxKey
  #endif

uint8_t ConGetKey()
{
  uint8_t Key;

  Key = TermGet();

  if ( Key == 0xff )
    return( 0 );
  //DbgPrintf("Key:%x\n", Key);
  if ( Key == 0x1B )              //ESC
    {
    Key = 0xff;
    delay(10);
    if ( TermGet() == 0x5B )  // Cursor
      {
      delay(10);
      switch ( TermGet() )    // Richtung
        {
        case 0x44: Key = JBKEY_LEFT;   break;
        case 0x43: Key = JBKEY_RIGHT;  break;
        case 0x42: Key = JBKEY_DOWN;   break;
        case 0x41: Key = JBKEY_UP;     break;
        }
      }
    }

  switch ( Key )
    {
    case JBKEY_LEFT:                // links
    case JBKEY_RIGHT:               // rechts
    case JBKEY_DOWN:                // runter
    case JBKEY_UP:                  // hoch
      break;
    case 0x0d:
      Key = JBKEY_LR;
      break;
    case 'U':
      Key = JBKEY_UD;
      break;
    case 'R':
      while (1);
    default:
      Key = 0;
      break;
    }
//  DbgPrintf("KeyConv:%x", Key);

  return( Key );
}
#endif



#if SOFT_TERM == 1

void TermPutTest( void *Buff )
{
 uint8_t *P = Buff;

  while( *P )
    TermSoftSerial->write( *P++ );
}

#endif

/*
  uint8_t *P = (uint8_t*)&MEntry;
  DbgPrintf("MEntry: ");
  for( Idx = 0; Idx < sizeof(MEntry); Idx++ )
    DbgPrintf("%02x ", *P++);
  DbgPrintf("\n");
*/
