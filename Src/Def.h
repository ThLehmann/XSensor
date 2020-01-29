/*************************************************************
  allgemeine Defines und Macros
  Hardwareeigenschaften / Zuordnung
  
  ------------------------------------------------------------
  
  Copyright (C) 2017 Thomas Lehmann
  
  Version history:
  1.00   20.06.2017  created
***************************************************************/


#ifndef DEF_H
#define DEF_H

#if ARDUINO >= 100
 #include <Arduino.h>
#else
 #include <WProgram.h>
#endif

#ifdef MAIN_CPP
#define EXTRN
#else
#define EXTRN extern
#endif



// bedingtes Compilat, einzelne Funktionen nicht in den Code nehmen
// nur notwendig wenn Programmspeicher (Code/Flash) überläuft
// entspr. Sensor mit 1 aktivieren, mit 0 aus dem Code entfernen
// !! Sensoren müssen neu konfiguriert werden wenn hier etwas geändert wird !!

#define _SENS_RPM_          1   // 1350  1 = Drehzahl Sensor ist dabei
#define _SENS_FUEL_         0   // 1900  1 = Spritverbrauch Sensor ist dabei
#define _SENS_TEMP_         1   // 1400  1 = Temperatur Sensor ist dabei
#define _SENS_ALT_          1   // 2400  1 = Höhen Sensor ist dabei
  #define _SENS_ALT_TEMP_   0   //  300  1 = Temperatur Anzeige vom MS5611, nur möglich wenn _SENS_ALT_ = 1
  #define _SENS_ALT_VARIO_  1   //  700  1 = Variofunktion aktiv, nur möglich wenn _SENS_ALT_ = 1
#define _SENS_GPS_          1   // 6600  1 = GPS Sensor ist dabei
                                //           2500 davon für AltSoftSerial
#define _SENS_PITOT_        0   //  600  1 = Staudruck Sensor ist dabei
#define _SENS_VOLT_         1   //  740  1 = Spannungs Sensor ist dabei
#define _SENS_MUI_          1   // 3320  1 = Strom Sensor ist dabei
#define _SENS_PWM_          0   //  350  1 = PWM Messung ist dabei

// noch nicht in Funktion
#define _SENS_ACC_          0   //       1 = G-Force Sensor ist dabei


#if _SENS_PWM_ == 1 && _SENS_FUEL_ == 1
geht nicht, nutzen beiden den gleichen PIN
#endif


// Adapterboard
// V2.00  A0-A3 mit 1K Vorwiderstand gegen VCC
// V1.00  A0-A2 mit 1K Vorwiderstand gegen GND
// V3.21  A0-A3 mit 1K Vorwiderstand gegen GND
//#define BOARD_VERS     200  // HW Version des Adpterboards
#define BOARD_VERS     321  // HW Version des Adpterboards






#define HW_TERM         0   // JetiTele COM Port nicht öffnen, ist für Debugausgaben
#define SOFT_TERM       0   // AltSoftSerial als Consolenschnittstelle

#define DEBUG_PIN       0   // Debug Ports aktiv
#define DEBUG_GPS       0   // RX Daten ausgeben
#define DEBUG_SCEDULES  0   // Laufzeitmessung
//#define DEBUG_JB_RX   0   // JB Empfang mitschneiden
#define VARIO_TEST      0   // steigen/sinken simulieren


#if DEBUG_PIN == 1
#define PIN_DBG1         10           // für Testausgaben / Messungen ...
#define PIN_DBG2         11
#endif


// printf() für Testzwecke einbinden
// SerialPrintf() will automatically put the format string in AVR program space
// 2009 - bperrybap
#if HW_TERM == 0
#define DbgPrintf(_fmt_, ...)
#define printFloat(_Txt_, _F_)
#define print64(_Txt_, _Val_)
#else

#include <avr/pgmspace.h>
#ifdef MAIN_CPP
extern "C" {
  void serialputc(char c, FILE *fp)
    { 
    if(c == '\n')
      Serial.write('\r'); 
    Serial.write(c); 
    }
  }

void _SerPrintf(const char *fmt, ...)
{
 FILE stdiostr;
 va_list ap;

  fdev_setup_stream(&stdiostr, serialputc, NULL, _FDEV_SETUP_WRITE);
  va_start(ap, fmt);
  vfprintf_P(&stdiostr, fmt, ap);
  va_end(ap);
}


void printFloat( const char *Txt, float F )
{
  Serial.print(Txt);
  Serial.print(F, 4);
}

void print64( const char *Txt, uint64_t Val )
{
 union
  {
  uint64_t B64;
  struct
    {
    uint32_t Lo;
    uint32_t Hi;
    }B32;
  }Conv;

  Serial.print(Txt);
  Conv.B64 = Val;
  Serial.print(Conv.B32.Hi);
  Serial.print(Conv.B32.Lo);
}

#else
extern void _SerPrintf(const char *fmt, ...);
extern void printFloat( const char *Txt, float F );
extern void print64( const char *Txt, uint64_t Val );

#endif

#define DbgPrintf(fmt, ...)   _SerPrintf(PSTR(fmt), ##__VA_ARGS__)

#endif






// Hardware
// --------
#define PIN_RPM_HALL      2           // D2 INT0   RPM Hallgeber
// Fuel und PWM teilen sich den gleichen IRQ, nur alterativ verwendbar
#if _SENS_FUEL_ == 1
  #define PIN_FUEL        3           // D3 INT1   Fuel Impulsgeber
#elif _SENS_PWM_ == 1
  #define PIN_PWM         3           // D3 INT1   PWM Impulsgeber
#endif
//#define PIN_FUEL_RESET  4           // D4        Fuel Messung zurücksetzen, es wurde getankt (digital)
//#define PIN_GPS_RX      8           // D8 PCINT0 GPS serielle RX (GPS TX) wird von AltSoftSerial verwendet
//#define PIN_GPS_TX      9           // D9 PCINT1 GPS serielle TX (GPS RX)
#define PIN_TEMP_NTC1    A0           // A0 (14)   NTC Tempsensor 1 (analog)
#define PIN_TEMP_NTC2    A1           // A1 (15)   NTC Tempsensor 2 (analog)
#define PIN_TEMP_NTC3    A2           // A2 (16)   NTC Tempsensor 3 (analog)
#define PIN_TEMP_NTC4    A3           // A3 (17)   NTC Tempsensor 4 (analog)
//#define PIN_VOLT_RX    A3           // A3 (17)   Empfängerspg.
//#define PIN_CURRENT_A  A6           // A6 (20)   Stromsensor Ampere
//#define PIN_CURRENT_V  A7           // A7 (21)   Stromsensor Volt

// I2C Adr 0x5A = IR Temp MLX90614, 1.
// ....
// I2C Adr 0x60 = IR Temp MLX90614, 7.
// I2C Adr 0x76 = Höhe BMP280
// I2C Adr 0x77 = Höhe BMP180 (GY-68)
// I2C Adr 0x77 = Höhe MS5611 (GY-63)
// I2C Adr 0x53 = Grafity ADXL345 1. Adr. ( GY-291)
// I2C Adr 0x1D = Grafity ADXL345 2. Adr. ( GY-291)
#define PIN_I2C_SDA   18             // (A4) I2C Data
#define PIN_I2C_SCL   19             // (A5) I2C Clock

#define PIN_LED        LED_BUILTIN   // Pin13 schaltbare LED auf Platine
  #define PIN_LED_ON   HIGH
  #define PIN_LED_OFF  LOW




#if BOARD_VERS  > 100 && BOARD_VERS < 321
#define ADC_GND         0   // Board mit A0-A3 mit 1K Vorwiderstand gegen Vcc
#else
#define ADC_GND         1   // Board mit A0-A3 mit 1K Vorwiderstand gegen GND
#endif


#define CNT_SENS_TEMP     7           // # Temp Sensoren
#define CNT_SENS_TEMP_NTC 4           // davon max. 4 NTC
#define CNT_SENS_TEMP_IR  7           // und max. 7 IR

#define CNT_SENS_VOLT     6           // # Spannungs Sensoren, 1-6 (max 6S)
#define CNT_SENS_MUI      3           // # Strom Sensoren, MUI1-3


//#define CNT_PORT_ANA      6         // # konfigurierbare Analog Ports A0-A3, A6+A7
#define PORT_DIGI_SW_FIRST  4         // D4-D7
#define CNT_PORT_DIGI_SW    4         // # konfigurierbare digi Ports D4-D7 für Tastenfunktion





#define TIM_BASE    10               // Timer Zeitbasis 10mS

#define OUT_TGL(_PIN) { static int Sts; digitalWrite( _PIN, Sts ); Sts ^= 1; }

#define SoftReset()   { delay(100); asm volatile ("  jmp 0"); }




#define  ARRAYCNT(x)   ( sizeof(x)/sizeof(x[0]) )

#define BIT_TST(val, bitnb)   (boolean) ( (val>>(bitnb)) &1L)
#define BIT_SET(val, bitnb)   ( val |= (1L << (bitnb)) )
#define BIT_CLR(val, bitnb)   ( val &= (~(1L << (bitnb))) )
#define BIT_TGL(val, bitnb)   ( val ^= (1L << (bitnb)) )

#define PBIT_TST(ptr, bitnb)  (boolean) ( ( ptr[(bitnb)/8] >> ((bitnb)%8) ) & 1)
#define PBIT_SET(ptr, bitnb)  ( ptr[(bitnb)/8] |= (1 << ((bitnb)%8)) )
#define PBIT_CLR(ptr, bitnb)  ( ptr[(bitnb)/8] &= (~(1 << ((bitnb)%8))) )
#define PBIT_TGL(ptr, bitnb)  ( ptr[(bitnb)/8] ^= (1 << ((bitnb)%8)) )




// Reset Funktionen, Fuel / Kapazität ...
// Fct 0   = manuell ohne Taster
// Fct 1-4 = manuell Taster an D4-D7
// Fct 5-n = auto
#define RESFCT_AUTO     (CNT_PORT_DIGI_SW + 1)  // Fct == Auto
#define RESFCT_POSAUTO  5                       // BufferPos hinter "auto "
const static char PgmTxtAuto[] PROGMEM = "auto ";

uint8_t MainEnableSw( uint8_t ResFct );





struct S_GLOBALS
{
  uint8_t Signal;
    #define SIG_SAVE2NV     0x01              // Summenwerte im EEProm sichern
    #define SIG_JB_REFRESH  0x02              // JetBox Anzeige Refresh
    
  float   ADCmV;                              // mV je ADC Tick entspr. konfigurierter AVcc
    #define MAX_ANALOG_READ  1023.0           // 10BIT A/D Wandler, typ 4,88mV bei 5V Refspg.

  struct
    {                                         // BIT je digi Port für Taster (4Stk D4-D7)
    uint8_t Enable;                           // Tasten Abfrage aktiv
    uint8_t Activ;                            // Taster betätigt
    uint8_t Debounce[CNT_PORT_DIGI_SW];       // Entprell Timer
    }Switch;

  // Aufbau der Daten im EEProm
  // max 1024Byte beim ATmega328
  // ---------------------------
  struct //S_EEPROM
    {
    struct
      {
      uint16_t Magic;                         // EEProm gültig, nur um bei Erstinbetriebnahme zu löschen
        #define EEP_MAGIC_NV    0x1628        // 16.02.18
      uint16_t Align;

      // Summenwerte, löschen nur wenn vom User gewünscht
      uint32_t RpmRevolTot;                   // Gesamtdrehzahl
      uint32_t RpmRunTimeS;                   // Gesamt Betriebszeit in Sekunden
      
      uint32_t FuelFlowCntTank;               // # Pulse bei akt. Tank gemessen / entnommene Menge
      uint32_t FuelFlowCntTot;                // # Pulse gesamt entnommene Menge
  
      uint32_t GpsDistTot;                    // Gesamt zurückgelegte Strecke, KM Stand
      uint32_t GpsSpeedMax;                   // max. erreichte Geschwindigkeit
  
      uint32_t AltitudeTot;                   // Gesamt erklummende Höhe

      float    MuiCapaConsumed[CNT_SENS_MUI]; // akt. entnommene Akku Kapazität in mAh
      uint16_t MuiVolt[CNT_SENS_MUI];         // letzte Akkuspg. in mV für auto zurücksetzen der Kapazität
      uint16_t Align1;
  
      uint32_t Res[2];
      }NV;
  
  
    struct
      {
      uint16_t  Magic;                        // EEProm gültig
        #define EEP_MAGIC_DATA  (0x1828^ID_CNT) // 18.02.18, Compile Änderungen auch berücksichtigen

        #define SENS_BYTES  6
      uint8_t  SensActivMsk[SENS_BYTES];      // BIT je aktiven Sensor (0-n)

      unsigned int WDogActiv  : 1;            // Watchdog aktiv, Bootloader muss WDog unterstützen (ausschalten)
      unsigned int SensNb     : 3;            // Sensor Nr. 1-7, 0 = ohne
      unsigned int CfgVoltFct : 1;            // Art der Spg. Messung
        #define CFG_VOLT_FCT_SINGLE 0         // jede Spg. separat
        #define CFG_VOLT_FCT_DIFF   1         // Differenz für Einzelzellen Messung
      unsigned int Res : 11;

      uint16_t AVcc;                          // Referenzspg. für ADC in mV (typ. Board Versorgungsspg.)
    
      #if _SENS_RPM_ == 1
      uint8_t  CfgRpmCntMag;                  // RPM, # Magnete, 0=einer / 1=zwei
      uint8_t  CfgRpmAccuracy;                // Ergebnis glätten, Genauigkeit n UPM, 0-100
        #define RPM_ACCURACY_DEF   10
        #define RPM_ACCURACY_MAX  100
      #endif

      #if _SENS_FUEL_ == 1
      uint16_t CfgFuelVolume;                 // Tankvolumen in mL
        #define FUEL_VOLUME_DEF   750
      uint16_t CfgFuelFlowCnt1L;              // # Pulse / Liter
      uint8_t  CfgFuelVolRes;                 // auto Reset oder manuell per Taster/JetiBox
      #endif

      #if _SENS_TEMP_ == 1
      uint8_t  CfgSensTypTemp[CNT_SENS_TEMP]; // Sensortyp Temperatur, NTC oder IR
      #endif
      
      #if _SENS_ALT_ == 1 && _SENS_ALT_VARIO_ == 1
      uint8_t  CfgVarioSmoothing;             // Vario Filter 0-100%
      uint8_t  CfgVarioDeadzone;              // Totzone, Vario ausblenden
      #endif

      #if _SENS_VOLT_ == 1
      // Spannungsmessung V1-4
      uint16_t CfgVoltRVcc[CNT_SENS_VOLT];    // Widerstandwert R1 gegen Vcc
      uint16_t CfgVoltRGnd[CNT_SENS_VOLT];    // Widerstandwert R2 gegen GND
      #endif

      #if _SENS_MUI_ == 1
      // Stromsensor MUI1-3
      uint8_t  CfgMuiSensTyp[CNT_SENS_MUI];   // Sensor Typ (STYP_MUI_XX)
      uint8_t  CfgMuiCalib[CNT_SENS_MUI];     // Kalibrierung auf 0
      uint8_t  CfgMuiCapaRes[CNT_SENS_MUI];   // wie Akkukapazität zurücksetzen
                                              // - auto immer oder bei Überschreiten der letzten Akkuspg. um n%
                                              // - manuell per Taster oder JetiBox
      uint16_t CfgMuiRVcc[CNT_SENS_MUI];      // Widerstandwert R1 gegen Vcc
      uint16_t CfgMuiRGnd[CNT_SENS_MUI];      // Widerstandwert R2 gegen GND
      #endif

        #define ANAFCT_PITOT      0
        #define ANAFCT_MUI1_A     1
        #define ANAFCT_MUI2_A     2
        #define ANAFCT_MUI3_A     3
        #define ANAFCT_MUI1_V     4
        #define ANAFCT_MUI2_V     5
        #define ANAFCT_MUI3_V     6
        #define ANAFCT_VOLT1      7
        #define ANAFCT_VOLT2      8
        #define ANAFCT_VOLT3      9
        #define ANAFCT_VOLT4     10
        #define ANAFCT_VOLT5     11
        #define ANAFCT_VOLT6     12
        #define ANAFCT_CNT       13
      int8_t CfgAnaFct[ANAFCT_CNT];           // ausgewählter Analog Port je Funktion
    
      #if _SENS_ACC_ == 1
//      int8_t GForceCfgCal[ACC_CAL_CNT];       // Kalibrierungswerte Accelerator / G-Force (X/Y/Z/Pitch/Roll)
      #endif
      }Data;
    }EEprom;
};


EXTRN struct S_GLOBALS GData;

#define PIN_PITOT         GData.EEprom.Data.CfgAnaFct[ANAFCT_PITOT]
#define PIN_VOLT(_VIdx)   GData.EEprom.Data.CfgAnaFct[ANAFCT_VOLT1+_VIdx]
#define PIN_MUI_A(_MIdx)  GData.EEprom.Data.CfgAnaFct[ANAFCT_MUI1_A+_MIdx]
#define PIN_MUI_V(_MIdx)  GData.EEprom.Data.CfgAnaFct[ANAFCT_MUI1_V+_MIdx]





#endif
