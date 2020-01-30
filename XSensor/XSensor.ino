// **************************************************************************************************************
// Multisensor für Jeti EX Telemetrie
// per JetiBox aktivierbare Sensoren
//
// Arudino Libs unter: (x86)/Arduino/hardware/arduino/avr/libraries/
// externe Librarys:
// TiniGps++          für GPS Sensor
// modifierte Libs:
// AltSoftSerial      dito, Software Serielle
// JetiEx Lib         Bernd Wokoeck https://sourceforge.net/projects/jetiexsensorcpplib/
//
//
// wenn kein Telemetrieframe gesendet wird, sendet der Empfänger alle 2Sek. seine Kennung
// feReceiver Type   Duplex R9 EX   ~ff
// feReceiver Type   Duplex R14 EX  ~ff
// feReceiver Type   Duplex Rsat2 EX~ff
//
// Hardware includes: "Arduino\hardware\tools\avr\avr\include\avr\"
// Register/ISR ... für 328er: iom328p.h
//
//
// Typ    Port  Funktion
// ---------------------
// Tele   UART  Empfängeranbindung
// RPM    IRQ   Drehzahlmessung
// FUEL   IRQ   Verbrauchsmessung per Flowmeter
// PWM    IRQ   PWM Messung
// ALT    I2C   Höhenmessung
// TEMP   I2C   Motortemperatur per IR Sensor
// ACC    I2C   Beschleunigungsmessung und Neigungswinkel
// GPS    SER   Positionsbestimmung, AltSoftserial IRQ+Timer gesteuert
// TEMP   ADC   Motortemperatur per NTC Widerstand
// PITOT  ADC   Geschwindigkeitsmessung per Drucksensor
// VOLT   ADC   Spannungsmessung
// CURR   ADC   Strommessung per ACSxx Board
//
// (c) 2017-2018 Th. Lehmann
// thomas@thls.de
//
// Historie
// --------
// 12.06.2017 V0.00 Erstellung
// 04.07.2017 V1.00 erste Version
// 30.10.2017 V1.00 RPM + Fuel Messung getestet und optimiert
// 02.11.2017 V1.00 Sensor Check, Init und RunTime Fehleranzeige (JB)
// 13.11.2017 V1.01 erste Version
// 17.11.2017 V1.02 NTC Tempsensor checked, jetzt eingestellt auf 1K Vorwiderstand und 10K NTC
// 21.11.2017 V1.03 GPS:Trip, gesamt zurückgelegte Strecke
// 23.11.2017 V1.04 IR Temperatursensor MXL90614
// 24.11.2017 V1.05 Tankvolume Autorücksetzen konfigurierbar
//                  Update to V1.05 JetiExSensor Lib from Sepp
//            V1.06 GPS: Trip und Gesamt Strecke (KM Stand)
// 30.11.2017 V1.06 Tankvolume Rest in Prozent, auf besonderen Wunsch von Alex F.
// 11.12.2017 V1.07 NTC und IR Temperatur Sensor auswählbar
// 16.12.2017 V1.08 Watchdog aktivierbar, SpeedMax im NV, GPS Trip korrigiert
// 21.12.2017 V1.09 unabhängige SensorId, Übergabe ist jetzt SensorIndex
// 26.12.2017 V1.10 JetiExLib Optimierungen, eingespart: FLASH - 1000 Byte / RAM stat - 50Byte, dyn - 48 Byte
//                  - Initverhalten optimiert, die ersten 30Sek wird zusätzlich Sensorname und Dictionary gesendet
//                  SensorNr bei Verwendung von mehreren XSens konfigurierbar
//                  Spg. Messung Ext Eingang, Referenzspg. für ADC konfigurierbar
// 27.12.2017 V1.20 JetiBox Menu neu gestaltet, flexibler Umgang mit bedingten Compilaten
// 28.12.2017 V1.21 Altimeter, Umgebungstemperatur entfernt, neuer Eintrag Gesamt Höhe
//                  JB neue Aufteilung, SensCfg/Service/Setup
//                  NTC Eingänge invertiert, NTC Eingang positiv, intern über 1K auf GND
// 30.12.2017 V1.22 Fuel, akt. entnommenes Volumen
// 02.01.2018 V1.23 Vario m/s
// 15.01.2018 V1.24 Sensor Idx, RX Spannung war vor GPS gerutscht
// 17.01.2018 V1.25 Vario Filter und Optimierungen
// 25.01.2018 V1.26 Geschwindigkeitsmessung per Pitot Rohr
// 05.02.2018 V1.30 Speicherüberarbeitung, GlobalData, Datenablage direkt im Speicher, Spg. Sensoren / Konfig
// 10.02.2018 V1.31 MUI Sensoren / Konfig, Strom/Spg/Watt/Kapazität, Fuel Prozentwert entfernt
// 17.02.2018 V1.32 MUI Kapazität verbleibend entfernt
//                  AltSoftSerial für GPS Nutzung optimiert
//                  nun bedingtes Compilat für alle Sensoren möglich
//                  Rücksetztaster an D4-D7 konfigurierbar
// 18.02.2018 V1.33 Vario, Smoothing Filter (danke an Rainer Stransky)
// 20.02.2018 V1.50 TinyGps++ Optimierung
// 12.03.2018 V1.51 JB Sensor Test Anzeige (Refresh) funktionierte nicht
// 02.07.2018 V1.52 GPS Höhe kann auch negative Werte annehmen (z.B. Hangflug)
// 24.07.2018 V1.53 ACSxx Sensorname ist zu lang, ACS in obere Zeile verschoben, Typ jetzt ohne vorangestelltes ACS
// 31.07.2018 V1.54 ACS712-05/20/30 ebenfalls mit Offset (Vref/2)
// 13.12.2018 V1.55 Sensor Indizierung fehlerhaft
// 17.12.2018 V1.56 Sensor Kennung in Zeile 2, vorgeschalteter Expander zeigt Zeile 2 in JB
// 22.12.2018 V1.57 Compiler Flag für Einzelzellen Spg. Anzeige
// 23.12.2018 V1.58 MUI Strom war auf 65A begrenzt
//                  Einzelzellen Spg. Messung jetzt konfigurierbar, Compilerflag entfernt
// 24.12.2018 V1.59 Scale Faktor für Spg. und MUI wurde nicht korrekt float berechnet, dadurch Auf/Abrundung und
//                  fehlerhafte Messung bei nicht ganzzahlig teilbaren Widerstandsverhältnis
// 25.12.2018 V1.60 Spg. Port Anzahl von 4 auf 6 erhöht, für Einzelzellenmessung bis max. 6S
// 29.12.2018 V1.61 Spg.+MUI Port, Widerstände mit Feineinstellung in 5er Schritten zur Kalibrierung, JB Taste hoch+runter
// 02.01.2019 V1.62 EEProm Speicher für Analoport Konfig Volt5+6 erweitert
// 04.01.2019 V1.63 Sensor Name ins Flash (6Byte mehr RAM)
//                  UART Korrektur, 2 Stop Bits und 9700Baud (besserer JB Tastenempfang)
// 09.01.2019 V1.64 Höhensensor MS5611 spinnt bei Temperaturen unter 20°, Berechnungsproblem
//                  - dT war als int64_t anstatt int32_t ausgelegt, dadurch hoher negativer 64BIT Wert bei Überlauf
//                  neuer Telewert: Temperatur vom Höhensensor (MS5611)
//                  Altitude gesamt wurde nicht in JB "Reset value" Menu angezeigt, konnte so auch nicht gelöscht werden
// 16.01.2019 V1.65 RAM Optimierung (zus. 140Byte), noch mehr Strings ins Flash verlagert
// 25.02.2019 V1.66 MUI Volt/Watt/Kapazität, Telemetriewerte wurden nicht übertragen, danke an Kees dem es aufgefallen ist
// 08.03.2019 V1.67 RPM, # Hallimpulse je Umdrehung von 1-2 auf 1-4 erweitert
// 11.03.2019 V1.68 MUI, Watt wurde nicht berechnet
//                  MUI, Telemetriewerte für Watt und Kapazität von 14b auf 22b vergrößert (vorher max. 8000W/8000mAh)
// 02.04.2019 V1.69 PWM Messung für zB Skynavigator (GPS Triangel), Ausgabe in 0-100%
//                  nicht zusammen mit Fuel Sensor nutzbar da gleicher INT Pin
// 12.04.2019 V1.70 PWM Messung, Ausgabe in uS
// 24.10.2019 V1.71 LED zeigt Init-Fehler bei Init. Bei Aktivierung von GPS_DIST_TO wird GPS_COURSE_TO mit aktiviert.
//                  Bei der Tele Init fehlte COURSE_TO
// 12.12.2019 V1.72 alle AnalogPorts mit Filterung (eigene Klasse) zur Glättung des ADC Rauschens (betroffen: Spg, Strom, NTC, Pitot)
//                  MUI Kalibrierung jetzt auch negativ möglich, ! Neukalibrierung notwendig
//                  auto Kapazitätsreset 2Sek. verzögert damit Spg. sicher anliegt
//                  Temperaturanzeige auf 0 bei Minusgrad-Ergebnis
// 16.12.2019 V1.73 MUI Volt hat Open() mit Current Klassenptr. gemacht, dadurch wurd V-Port auf A-Port gelegt :(
// 21.12.2019 V1.74 JetiBox Tastenentprellung optimiert
// 
// **************************************************************************************************************

/*
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
MIT License

Copyright (c) 2017-2019 Thomas Lehmann

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
*/

                                // 1234567890123456
const char JB_TEXT_Z1[] PROGMEM = "  XSensor by ThL";
const char JB_TEXT_Z2[] PROGMEM = "  XSens    V1.74";
//                                "  XSens1   V1.xx"
#define JB_POS_Z2_SENSNB  7                    // Sensor Nr eintragen wenn konfiguriert


#define MAIN_CPP


#include "EEPROM.h"
#include <avr/wdt.h>
// wdt_enable( WDTO_2S );                      // Watchdog aktivieren, Tmo 2Sek.
// wdt_reset();                                // Watchdog triggern


#include "Def.h"
#include <avr/pgmspace.h>

#include "JetiExProtocol.h"
JetiExProtocol JetiEx;

// Sensoren
#if _SENS_RPM_ == 1
#include "SensRpm.h"
SensorRpm *SensRpm;
#endif

#if _SENS_FUEL_ == 1
#include "SensFuel.h"
SensorFuel *SensFuel;
#endif

#if _SENS_ALT_ == 1
#include "SensAlt.h"
SensorAlt *SensAlt;
#endif

#if _SENS_PITOT_ == 1
#include "SensPitot.h"
SensorPitot *SensPitot;
#endif

#if _SENS_TEMP_ == 1
#include "SensTemp.h"
SensorTemp *SensTemp[CNT_SENS_TEMP];
#endif

#include "SensPower.h"
#if _SENS_VOLT_ == 1
SensorPwrVolt *SensPwrVolt[CNT_SENS_VOLT];    // Spannung
#endif
#if _SENS_MUI_  == 1
SensorPwrMui  *SensPwrMui[CNT_SENS_MUI];      // Strom
#endif

#if _SENS_GPS_ == 1
#include "SensGPS.h"
SensorGps *SensGps;
#endif

#if _SENS_PWM_ == 1
#include "SensPwm.h"
SensorPwm *SensPwm;
#endif

#if _SENS_ACC_ == 1
#include "SensACC.h"
SensorAcc *SensAcc;
#endif




// LED Anzeige
// ===========                      // ein / aus Zeit in mS, je BIT 100mS max 1600mS
#define LED_OFF     0x0000          //   0 /    0  LED aus
#define LED_ON      0xffff          //   1 /    0  LED statisch an
#define LED_SLOW    0x00ff          // 800 /  800  LED blinkt langsam
#define LED_FAST    0x0f0f          // 400 /  400  LED blinkt schnell
#define LED_VFAST   0x3333          // 200 /  200  LED blinkt sehr schnell
#define LED_VVFAST  0x5555          // 100 /  100  LED blinkt sehr sehr schnell
#define LED_FLASH1  0x0001          // 100 / 1500  LED Flash
#define LED_FLASH2  0x0101          // 100 /  700  LED Flash
#define LED_FLASH3  0x1111          // 100 /  300  LED Flash
#define LED_FLASH4  0xfffb          // 1400/  200  LED Flash

#define LED_SILENT     LED_FLASH1   // Ruhebetrieb
#define LED_NO_SENS    LED_FLASH4   // kein Sensor aktiv
#define LED_SENS_INI   LED_VVFAST   // Sensor init Fehler
#define LED_SENS_ERR   LED_SLOW     // Sensor failure

static uint16_t LedState;           // akt. LED Blinkrhytmus

static uint8_t CntSensActiv;        // # aktiver Sensoren

enum
{
  #if _SENS_RPM_ == 1
  ID_RPM,
  ID_RPM_REVOL,
  ID_RPM_RUNTIME,
  #endif

  #if _SENS_FUEL_ == 1
  ID_FUEL_REMAIN,
  ID_FUEL_CONSUMED,
  ID_FUEL_FLOW,
  ID_FUEL_TOT,
  #endif

  #if _SENS_TEMP_ == 1
  ID_TEMP1,
  ID_TEMP2,
  ID_TEMP3,
  ID_TEMP4,
  ID_TEMP5,
  ID_TEMP6,
  ID_TEMP7,
  #endif

  #if _SENS_ALT_ == 1
  ID_ALT_REL,
  ID_ALT_TOT,
  #if _SENS_ALT_TEMP_ == 1
  ID_ALT_TEMP,
  #endif
  #if _SENS_ALT_VARIO_ == 1
  ID_ALT_VARIO,
  #endif
  #endif

  #if _SENS_GPS_ == 1
  _ID_GPS_FIRST,
  ID_GPS_SATS = _ID_GPS_FIRST,
  ID_GPS_LAT,
  ID_GPS_LON,
  ID_GPS_ALT,
  ID_GPS_SPEED,
  ID_GPS_SPEED_MAX,
  ID_GPS_HEAD,
  ID_GPS_COURSE_TO,
  ID_GPS_DIST_TO,
  ID_GPS_DIST_TRIP,
  ID_GPS_DIST_TOT,
  _ID_GPS_LAST = ID_GPS_DIST_TOT,
  #endif

  #if _SENS_PITOT_ == 1
  ID_PITOT_SPEED,
  #endif

  #if _SENS_VOLT_ == 1
  ID_PWR_VOLT1,                              // Spg. 1 - 4
  ID_PWR_VOLT2,
  ID_PWR_VOLT3,
  ID_PWR_VOLT4,
  ID_PWR_VOLT5,
  ID_PWR_VOLT6,
  #endif

  #if _SENS_MUI_ == 1
  _ID_MUI_FIRST,
  ID_PWR_MUI1_A = _ID_MUI_FIRST,             // Stromsensor 1-3, Ampere
  ID_PWR_MUI2_A,
  ID_PWR_MUI3_A,
  ID_PWR_MUI1_V,                             // Spannung
  ID_PWR_MUI2_V,
  ID_PWR_MUI3_V,
  ID_PWR_MUI1_W,                             // Watt
  ID_PWR_MUI2_W,
  ID_PWR_MUI3_W,
  ID_PWR_MUI1_C,                             // entnommene Kapazität
  ID_PWR_MUI2_C,
  ID_PWR_MUI3_C,
  _ID_MUI_LAST = ID_PWR_MUI3_C,
  #endif

  #if _SENS_PWM_ == 1
  ID_PWM,
  #endif

  #if _SENS_ACC_ == 1
  ID_ACC_GFORCE_X,
  ID_ACC_GFORCE_Y,
  ID_ACC_GFORCE_Z,
  #endif

  ID_CNT,
  ID_CNT_BYTES = (ID_CNT/8) + (ID_CNT%8 ? 1:0)
};



// max. 32 Sensoren
// Id von 1..255
// Name plus Unit < 20 characters
//  - Name < 14 characters inkl. Sensornamen (NAME_SENSOR)
//  - unit <  4 characters
// precision = 0 --> 0, precision = 1 --> 0.0, precision = 2 --> 0.00
// Wertebereich:
// TYPE_6b   int6_t  Data type 6b  (       -31        +31)
// TYPE_14b  int14_t Data type 14b (     -8191      +8191)
// TYPE_22b  int22_t Data type 22b (  -2097151   +2097151)
// TYPE_30b  int30_t Data type 30b (-536870911 +536870911) 

const char NAME_SENSOR[] PROGMEM = "XSens";
JETISENSOR_CONST Sensors[] PROGMEM =
{
  // id    name               unit         data type             precision      Idx
  //#20> name+unit
  //XSens1:12345678901234     1234
  #if _SENS_RPM_ == 1
  {  1,   "RPM",             "/min",       JetiSensor::TYPE_22b, 0 }, //        ID_RPM
  {  2,   "RPM.Revol tot",   "Mil",        JetiSensor::TYPE_22b, 2 }, // dez.2  ID_RPM_REVOL
  {  3,   "RPM.Runtime",     "h",          JetiSensor::TYPE_22b, 2 }, // dez.2  ID_RPM_RUNTIME
  #endif

  #if _SENS_FUEL_ == 1
  { 10,   "FUEL.Remain",     "mL",         JetiSensor::TYPE_14b, 0 }, //        ID_FUEL_REMAIN
  { 12,   "FUEL.Consumed",   "mL",         JetiSensor::TYPE_14b, 0 }, //        ID_FUEL_CONSUMED
  { 13,   "FUEL.Flow",       "mL",         JetiSensor::TYPE_14b, 0 }, //        ID_FUEL_FLOW
  { 14,   "FUEL.Flow tot",   "L",          JetiSensor::TYPE_22b, 2 }, // dez.2  ID_FUEL_TOT
  #endif

  #if _SENS_TEMP_ == 1
  { 20,   "Temp.1",          "\xB0\x43",   JetiSensor::TYPE_14b, 0 }, // °C     ID_TEMP1
  { 21,   "Temp.2",          "\xB0\x43",   JetiSensor::TYPE_14b, 0 }, // °C     ID_TEMP2
  { 22,   "Temp.3",          "\xB0\x43",   JetiSensor::TYPE_14b, 0 }, // °C     ID_TEMP3
  { 23,   "Temp.4",          "\xB0\x43",   JetiSensor::TYPE_14b, 0 }, // °C     ID_TEMP4
  { 24,   "Temp.5",          "\xB0\x43",   JetiSensor::TYPE_14b, 0 }, // °C     ID_TEMP5
  { 25,   "Temp.6",          "\xB0\x43",   JetiSensor::TYPE_14b, 0 }, // °C     ID_TEMP6
  { 26,   "Temp.7",          "\xB0\x43",   JetiSensor::TYPE_14b, 0 }, // °C     ID_TEMP7
  #if CNT_SENS_TEMP != 7
  #error hier ist was faul
  #endif
  #endif

  //#20> name+unit
  //XSens1:12345678901234     1234
  #if _SENS_ALT_ == 1
  { 30,   "ALT.Altit. rel",  "m",          JetiSensor::TYPE_14b, 0 }, //        ID_ALT_REL
  { 31,   "ALT.total",       "km",         JetiSensor::TYPE_22b, 2 }, // dez.2  ID_ALT_TOT
  #if _SENS_ALT_TEMP_ == 1
  { 32,   "ALT.Temp",        "\xB0\x43",   JetiSensor::TYPE_14b, 1 }, // dez.1  ID_ALT_TEMP °C
  #endif
  #if _SENS_ALT_VARIO_ == 1
  { 35,   "ALT.Vario",       "m/s",        JetiSensor::TYPE_22b, 2 }, // dez.2  ID_ALT_VARIO
  #endif
  #endif

  #if _SENS_GPS_ == 1
  { 40,   "GPS.Satellites",  "#",          JetiSensor::TYPE_14b, 0 }, //        ID_GPS_SATS
  { 41,   "GPS.Latitude",    " ",          JetiSensor::TYPE_GPS, 0 }, //        ID_GPS_LAT
  { 42,   "GPS.Longitude",   " ",          JetiSensor::TYPE_GPS, 0 }, //        ID_GPS_LON
  { 43,   "GPS.Altitude",    "m",          JetiSensor::TYPE_14b, 0 }, //        ID_GPS_ALT
  { 44,   "GPS.Speed",       "km/h",       JetiSensor::TYPE_14b, 0 }, //        ID_GPS_SPEED
  { 45,   "GPS.Speed max.",  "km/h",       JetiSensor::TYPE_14b, 0 }, //        ID_GPS_SPEED_MAX
  { 46,   "GPS.Heading",     "\xB0",       JetiSensor::TYPE_14b, 0 }, // °      ID_GPS_HEAD
  { 47,   "GPS.Course to",   "\xB0",       JetiSensor::TYPE_14b, 0 }, // °      ID_GPS_COURSE_TO
  { 48,   "GPS.Dist to",     "m",          JetiSensor::TYPE_14b, 0 }, //        ID_GPS_DIST_TO
  { 49,   "GPS.Dist trip",   "m",          JetiSensor::TYPE_22b, 0 }, //        ID_GPS_DIST_TRIP
  { 50,   "GPS.Dist total",  "km",         JetiSensor::TYPE_22b, 2 }, // dez.2  ID_GPS_DIST_TOT
  #endif

  //#20> name+unit
  //XSens1:12345678901234     1234
  #if _SENS_PITOT_ == 1
  { 55,   "Pitot.Speed",     "km/h",       JetiSensor::TYPE_14b, 0 }, //        ID_PITOT_SPEED
  #endif

  #if _SENS_VOLT_ == 1
  { 60,   "Voltage.1",       "V",          JetiSensor::TYPE_14b, 2 }, // dez.2  ID_PWR_VOLT1
  { 61,   "Voltage.2",       "V",          JetiSensor::TYPE_14b, 2 }, // dez.2  ID_PWR_VOLT2
  { 62,   "Voltage.3",       "V",          JetiSensor::TYPE_14b, 2 }, // dez.2  ID_PWR_VOLT3
  { 63,   "Voltage.4",       "V",          JetiSensor::TYPE_14b, 2 }, // dez.2  ID_PWR_VOLT4
  { 64,   "Voltage.5",       "V",          JetiSensor::TYPE_14b, 2 }, // dez.2  ID_PWR_VOLT5
  { 65,   "Voltage.6",       "V",          JetiSensor::TYPE_14b, 2 }, // dez.2  ID_PWR_VOLT6
  #endif

  //#20> name+unit
  //XSens1:12345678901234     1234
  #if _SENS_MUI_ == 1
  { 70,   "MUI1.Current",    "A",         JetiSensor::TYPE_22b, 2 }, // dez.2  ID_PWR_MUI1_A
  { 74,   "MUI2.Current",    "A",         JetiSensor::TYPE_22b, 2 }, // dez.2  ID_PWR_MUI2_A
  { 78,   "MUI3.Current",    "A",         JetiSensor::TYPE_22b, 2 }, // dez.2  ID_PWR_MUI3_A
  { 71,   "MUI1.Voltage",    "V",         JetiSensor::TYPE_14b, 2 }, // dez.2  ID_PWR_MUI1_V
  { 75,   "MUI2.Voltage",    "V",         JetiSensor::TYPE_14b, 2 }, // dez.2  ID_PWR_MUI2_V
  { 79,   "MUI3.Voltage",    "V",         JetiSensor::TYPE_14b, 2 }, // dez.2  ID_PWR_MUI3_V
  { 72,   "MUI1.Power",      "W",         JetiSensor::TYPE_22b, 0 }, //        ID_PWR_MUI1_W
  { 76,   "MUI2.Power",      "W",         JetiSensor::TYPE_22b, 0 }, //        ID_PWR_MUI2_W
  { 80,   "MUI3.Power",      "W",         JetiSensor::TYPE_22b, 0 }, //        ID_PWR_MUI3_W
  { 73,   "MUI1.Capacity",   "mAh",       JetiSensor::TYPE_22b, 0 }, //        ID_PWR_MUI1_C
  { 77,   "MUI2.Capacity",   "mAh",       JetiSensor::TYPE_22b, 0 }, //        ID_PWR_MUI2_C
  { 81,   "MUI3.Capacity",   "mAh",       JetiSensor::TYPE_22b, 0 }, //        ID_PWR_MUI3_C
  #endif

  #if _SENS_PWM_ == 1
  { 85,   "PWM",             "uS",        JetiSensor::TYPE_14b, 0 }, //        ID_PWM
  #endif

  #if _SENS_ACC_ == 1
  //#20> name+unit
  //XSens1:12345678901234     1234
  { 90,   "ACC.GForce X",    "G",          JetiSensor::TYPE_14b, 1 }, // dez.1  ID_ACC_GFORCE_X
  { 91,   "ACC.GForce Y",    "G",          JetiSensor::TYPE_14b, 1 }, // dez.1  ID_ACC_GFORCE_Y
  { 92,   "ACC.GForce Z",    "G",          JetiSensor::TYPE_14b, 1 }, // dez.1  ID_ACC_GFORCE_Z
  { 95,   "ACC.GFor maxX",   "G",          JetiSensor::TYPE_14b, 1 }, // dez.1  ID_ACC_GFORCE_MAX_X
  { 96,   "ACC.GFor maxY",   "G",          JetiSensor::TYPE_14b, 1 }, // dez.1  ID_ACC_GFORCE_MAX_Y
  { 97,   "ACC.GFor maxZ",   "G",          JetiSensor::TYPE_14b, 1 }, // dez.1  ID_ACC_GFORCE_MAX_Z
  #endif

  { 0 }   // end of array
};


// JetiBox + Telemetrieübertragung
// -------------------------------
static struct
  {
  uint8_t  AvailMsk[SENS_BYTES];      // verfügbare + aktive Sensoren, BIT je Idx = ID_XX
  uint8_t  ErrInit[SENS_BYTES];       // Fehler bei der Sensor Initialisierung
  bool     Error;
  int16_t  TmoRefresh;                // JB Tasten und Telewerte nur alle 100mS checken
    #define TMO_TELE_SND    (100/TIM_BASE)
  int8_t   JBTmoRefresh;              // JB Refresh nicht permanent ausführen
    #define TMO_JB_REFRESH  (100/TIM_BASE)
  }Tele;


// Sensor Test aktiv, auch Flow Kalibrierung
// -----------------------------------------
static struct
  {
  int8_t   Id;                        // Sensor ID
  uint8_t  IdLast;
  uint32_t StartVal;                  // 0 = neue Messung
  uint16_t MesureVal;                 // Differenz zum Start, Sensor meldet Änderung
  }SensorChk;

static bool EEpromSave;               // EEProm Daten geändert > speichern




// Funktionsprototypen
// -------------------
static void HandleTim( void );
static void EEpromRW( bool Write, bool WrNV );
static void EEpromChk( void );
static void HandleJB( bool Refresh );
static uint8_t ConGetKey( void );



#if SOFT_TERM == 1
#include <AltSoftSerial.h>
AltSoftSerial *TermSoftSerial;
char TermBuff[30];
void TermPutTest( void *Buff );
//void TermPutln( void *Buff );
#endif



int freeRam()
{
 extern int __heap_start, *__brkval;
 int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

#include "JetiBox.h"



// Watchdog Time-out
// =================
/*
ISR( WDT_vect )
{
  WDTCSR = 0;                         // Watchdog aus, akt. installierter Bootloader macht das leider nicht, Update auf OptiBoot notwendig
  OUT_TGL( PIN_DBG );
  SoftReset();                        // Neustart
}
*/


// ====
// INIT
// ====
void setup( void )
{
 
#if SOFT_TERM == 1
  TermSoftSerial = new AltSoftSerial;          // serielle auf PIN 8 (RX) + 9 (TX)
  TermSoftSerial->begin( 9600 );
  delay(1000);
#endif

#if HW_TERM == 1
  Serial.begin(115200);

  DbgPrintf("\nXSensor Dbg\n");
  DbgPrintf("free RAM init:%u\n", freeRam());

  DbgPrintf("ID_CNT:%d ID_CNT_BYTES:%d SENS_BYTES:%d\n", ID_CNT, ID_CNT_BYTES, SENS_BYTES);
  if( SENS_BYTES < ID_CNT_BYTES )
    {
    DbgPrintf(">>>ERROR SensMsk");
    while(1);
    }
#endif

#if DEBUG_PIN == 1
  pinMode( PIN_DBG1, OUTPUT );
  digitalWrite( PIN_DBG1, HIGH );
  pinMode( PIN_DBG2, OUTPUT );
  digitalWrite( PIN_DBG2, HIGH );
#endif


  EEpromRW( false, false );                       // EEProm lesen
  pinMode( PIN_LED, OUTPUT );
  //digitalWrite( PIN_LED, PIN_LED_ON );

//analogReference( INTERNAL );

  // aktive Sensoren ermitteln und initialisieren
  // --------------------------------------------
  for( uint8_t Idx = 0; Idx < ID_CNT; Idx++ )
    {
    bool Avail = false;
    uint8_t i;

    if( !PBIT_TST(GData.EEprom.Data.SensActivMsk, Idx) )    // Sensor aktiv geschaltet ?
      continue;

    bool VarioActiv = false;
    switch ( Idx )
      {
      #if _SENS_RPM_ == 1
      case ID_RPM:
      case ID_RPM_REVOL:
      case ID_RPM_RUNTIME:
        if( SensRpm == 0L )
          SensRpm = new SensorRpm();
        Avail = SensRpm->Init();
        break;
      #endif

      #if _SENS_FUEL_ == 1
      case ID_FUEL_REMAIN:
      case ID_FUEL_CONSUMED:
      case ID_FUEL_FLOW:
      case ID_FUEL_TOT:
        if( SensFuel == 0L )
          SensFuel = new SensorFuel();
        Avail = SensFuel->Init();
        break;
      #endif

      #if _SENS_ALT_ == 1
      case ID_ALT_REL:
      case ID_ALT_TOT:
      #if _SENS_ALT_TEMP_ == 1
      case ID_ALT_TEMP:
      #endif
      #if _SENS_ALT_VARIO_ == 1
      case ID_ALT_VARIO:
        VarioActiv = PBIT_TST(GData.EEprom.Data.SensActivMsk, ID_ALT_VARIO);
      #endif
        if( SensAlt == 0L )
          SensAlt = new SensorAlt();
        Avail = SensAlt->Init( Idx, VarioActiv );
        break;
      #endif

      #if _SENS_PITOT_ == 1
      case ID_PITOT_SPEED:
        SensPitot = new SensorPitot();
        Avail = SensPitot->Init( Idx );
        break;
      #endif

      #if _SENS_TEMP_ == 1
      case ID_TEMP1 ... ID_TEMP7:
        i = Idx - ID_TEMP1;
        SensTemp[i] = new SensorTemp();
        Avail = SensTemp[i]->Init( Idx, i );
        break;
      #endif

      #if _SENS_VOLT_ == 1
      case ID_PWR_VOLT1 ... ID_PWR_VOLT6:
        i = Idx - ID_PWR_VOLT1;
        SensPwrVolt[i] = new SensorPwrVolt();
        Avail = SensPwrVolt[i]->Init( Idx, i );
        break;
      #endif

      #if _SENS_MUI_  == 1
      case ID_PWR_MUI1_A ... ID_PWR_MUI3_A:         // MUIn aktivieren
        i = Idx - ID_PWR_MUI1_A;
        SensPwrMui[i] = new SensorPwrMui();
        Avail = SensPwrMui[i]->Init( Idx, i );
        break;
      case ID_PWR_MUI1_V:
        i = Idx - ID_PWR_MUI1_V;
        goto MUI_CHK;
      case ID_PWR_MUI1_W:
        i = Idx - ID_PWR_MUI1_W;
        goto MUI_CHK;
      case ID_PWR_MUI1_C:
        i = Idx - ID_PWR_MUI1_C;
MUI_CHK:
        if( SensPwrMui[i] )                         // ID_PWR_MUIn_A aktiviert MUI vorher
          Avail = true;
        break;
      #endif


      #if _SENS_GPS_ == 1
      // GPS Sensoren
      case ID_GPS_SATS:
      case ID_GPS_LAT:
      case ID_GPS_LON:
      case ID_GPS_ALT:
      case ID_GPS_SPEED:
      case ID_GPS_SPEED_MAX:
      case ID_GPS_HEAD:
      case ID_GPS_COURSE_TO:
      case ID_GPS_DIST_TO:
      case ID_GPS_DIST_TRIP:
      case ID_GPS_DIST_TOT:
        if( SensGps == 0L )
          SensGps = new SensorGps();
        Avail = SensGps->Init( Idx );
        break;
      #endif

      #if _SENS_PWM_ == 1
      case ID_PWM:
        SensPwm = new SensorPwm();
        Avail = SensPwm->Init();
        break;
      #endif

      #if _SENS_ACC_ == 1
      // Acceleration G-Force
      case ID_ACC_GFORCE_X:
      case ID_ACC_GFORCE_Y:
      case ID_ACC_GFORCE_Z:
        if( SensAcc == 0L )
          SensAcc = new SensorAcc();
        //Avail = SensAcc->Init( Idx, GData.EEprom.Data.GForceCfgCal );
        break;
      #endif
      }

    CntSensActiv++;                                   // # aktivierter Sensoren
    if( Avail )
      PBIT_SET( Tele.AvailMsk, Idx );
    else
      {
      Tele.Error = true;
      PBIT_SET( Tele.ErrInit, Idx );
      }
    }



  char JB_Buff[17];
  // Jeti EX Protokoll aktivieren, Sensor und Sensorwerte anlegen
  // DevId = 0xa4093276 + DevIdx
  strcpy_P(JB_Buff, NAME_SENSOR);
  JetiEx.Start( JB_Buff, GData.EEprom.Data.SensNb, Sensors, ARRAYCNT(Sensors), GData.EEprom.Data.SensActivMsk, JetiExProtocol::SERIAL2 );
  //Tele.TmoRefresh = 1;                              // JB Tasten und Telewerte direkt mal checken

  // unterstützt Bootloader Watchdog, User meint das ;)
  if( GData.EEprom.Data.WDogActiv )
    wdt_enable( WDTO_2S );                            // Watchdog aktivieren, Tmo 2Sek.
    //WDTCSR |= (1<<WDIE);                              // Timeout IRQ aktivieren
/*
#define WDTCSR _SFR_MEM8(0x60)
#define WDP0 0
#define WDP1 1
#define WDP2 2
#define WDE 3
#define WDCE 4
#define WDP3 5
#define WDIE 6
#define WDIF 7
*/

  DbgPrintf("free RAM runtime:%u\n", freeRam());
}


void loop( void )
{
 static uint32_t Tim;

  wdt_reset();                                // Watchdog triggern

  #if DEBUG_SCEDULES == 1
  static long SceduleCnt;
  SceduleCnt++;
  #endif

  #if _SENS_ALT_ == 1
  if( SensAlt )
    SensAlt->Loop();
  #endif

  #if _SENS_GPS_ == 1
  if( SensGps )
    SensGps->Loop();
  #endif

  // 10mS Timer generieren
  // ---------------------
  if ( Tim <= micros() )
    {
    Tim = micros() + (TIM_BASE*1000L);
    HandleTim();                              // 10mS Timer bedienen

    #if DEBUG_SCEDULES == 1
    static uint16_t t100;
    if ( ++t100 > 100 )
      {
      t100 = 0;
      DbgPrintf("SceduleCnt:%u\n", SceduleCnt);
      SceduleCnt = 0;
      }
    #endif
    }


  // anstehende Events bewerten
  // --------------------------
  if( GData.Signal & SIG_JB_REFRESH )
    {
    GData.Signal &= ~SIG_JB_REFRESH;
    if( Tele.JBTmoRefresh == 0 )              // nicht permanent Refresh aufrufen
      Tele.JBTmoRefresh = TMO_JB_REFRESH;     // nur alle 100mS
    }
}



// ===================
// 10mS Timer bedienen
// ===================
void HandleTim( void )
{
 static uint8_t TmoLED, LedShift;
 static uint16_t Save2NVTmo;
  #define SAVE_2NV_TMO  (5000/TIM_BASE)
 bool SensFailure = false;
 uint8_t Idx;

  if( Tele.JBTmoRefresh && (--Tele.JBTmoRefresh == 0) )
    HandleJB( true );

  // Sensor Timer bedienen
  // ---------------------
  #if _SENS_RPM_ == 1
  if( SensRpm )
    SensRpm->Tim10mS();                       // Drehzahl
  #endif
  #if _SENS_FUEL_ == 1
  if( SensFuel )
    SensFuel->Tim10mS();                      // Durchfluss
  #endif
  #if _SENS_ALT_ == 1
  if( SensAlt )
    {
    SensAlt->Tim10mS();                       // Höhe
    SensFailure |= SensAlt->Failure;          // irgendwas faul ?
    }
  #endif
  #if _SENS_PITOT_ == 1
  if( SensPitot )
    {
    SensPitot->Tim10mS();                     // Speed
    SensFailure |= SensPitot->Failure;        // irgendwas faul ?
    }
  #endif
  #if _SENS_VOLT_ == 1
  for( Idx = 0; Idx < CNT_SENS_VOLT; Idx++ )
    if( SensPwrVolt[Idx] )
      SensPwrVolt[Idx]->Tim10mS();            // Spannung
  #endif
  #if _SENS_MUI_  == 1
  for( Idx = 0; Idx < CNT_SENS_MUI; Idx++ )
    if( SensPwrMui[Idx] )
      SensPwrMui[Idx]->Tim10mS();             // Energie (Strom/Spg/Watt/Verbrauch)
  #endif
  #if _SENS_TEMP_ == 1
  // Temperatur
  for( Idx = 0; Idx < CNT_SENS_TEMP; Idx++ )
    if( SensTemp[Idx] )
      {
      SensTemp[Idx]->Tim10mS();
      SensFailure |= SensTemp[Idx]->Failure;  // irgendwas faul mit dem Sensor ?
      }
  #endif
  #if _SENS_GPS_ == 1
  if( SensGps )
    {
    SensGps->Tim10mS();                       // GPS
    SensFailure |= SensGps->Failure;          // irgendwas faul im GPS ?
    }
  #endif
  #if _SENS_PWM_ == 1
  if( SensPwm )
    SensPwm->Tim10mS();                       // PWM Messung
  #endif
  #if _SENS_ACC_ == 1
  if( SensAcc )
    SensAcc->Tim10mS();                       // G-Force
  #endif

  //JetiEx.DoJetiSend();                      // Routine sendet nur alle 150mS
  JetiEx.Tim10mS();                           // JetiEx senden, nur alle 150mS einen Frame
  HandleJB( false );                          // JetiBox bedienen


  if( CntSensActiv == 0 )                     // kein einziger Sensor aktiv ? was soll denn der Quatsch ;)
    LedState = LED_NO_SENS;                   // evt. EEprom gelöscht
  else if( Tele.Error )                       // Sensor init fehlgeschlagen
    LedState = LED_SENS_INI;
  else if( SensFailure )                      // hat ein Sensor ein Problem ?
    LedState = LED_SENS_ERR;                  // Sensor failure
  else
    LedState = LED_SILENT;                    // alles im grünen Bereich


  // LED Blinkrhytmus
  // ----------------
  if( ++TmoLED >= 10 )                        // je Tick 100mS
    {
    TmoLED = 0;
    digitalWrite( PIN_LED, (LedState >> LedShift) & 1 );
    LedShift++;
    LedShift &= 0x0f;                         // nur 16 BIT Blinken
    }


  // Taster auf digi Ports bedienen
  // ------------------------------
  uint8_t PinSw;
  for( PinSw = PORT_DIGI_SW_FIRST; PinSw < (PORT_DIGI_SW_FIRST + CNT_PORT_DIGI_SW); PinSw++ )
    {
    if( BIT_TST(GData.Switch.Enable, PinSw) )         // Taster aktiviert ?
      {
      Idx = PinSw - PORT_DIGI_SW_FIRST;
      if( digitalRead(PinSw) )                        // Taster LOW == betätigt
        {
        BIT_CLR(GData.Switch.Activ, PinSw );          // Taste losgelassen
        GData.Switch.Debounce[Idx] = (200/TIM_BASE);  // 200mS entprellen wenns auf LOW geht
        }                                             // entprellen, Taster muss 200mS gedrückt sein
      else if( GData.Switch.Debounce[Idx] && (--GData.Switch.Debounce[Idx] == 0) )
        BIT_SET(GData.Switch.Activ, PinSw );          // Tastendruck erkannt, war länger als 200mS betätigt
      }
    }


  // Summenwerte im EEProm sichern wenn sich nichts mehr ändert
  // ==========================================================
  if( GData.Signal & SIG_SAVE2NV )            // irgendein Sensor signalisiert das Änderungen vorliegen
    {
    GData.Signal &= ~SIG_SAVE2NV;
    // wenn sich nichts mehr ändert wird sehr wahrscheinlich geich ausgeschaltet, vorher noch ins EEProm schreiben
    Save2NVTmo = SAVE_2NV_TMO;
    //DbgPrintf("SIG_SAVE2NV\n");
    }
  if( Save2NVTmo && (--Save2NVTmo) == 0 )
    {
    DbgPrintf("SAVE2EEPROM\n");
    // Messung vom 02.01.2019 > 232uS
    EEpromRW( true, true );                   // nur NV Bereich schreiben
    }


  // Sensor Test aktiv, auch Flow Kalibrierung
  // -----------------------------------------
  if( SensorChk.Id != SensorChk.IdLast )
    {
    SensorChk.IdLast = SensorChk.Id;
    SensorChk.StartVal = 0L;
    }
  if( SensorChk.Id != -1 )
    {
    uint32_t Val = 0L;

    if( PBIT_TST(Tele.AvailMsk, SensorChk.Id) )           // Sensor aktiv ?
      {
      #if _SENS_RPM_ == 1
      if( SensorChk.Id == ID_RPM )                        // Hallgeber Test ?
        Val = GData.EEprom.NV.RpmRevolTot;
      #endif
      #if _SENS_FUEL_ == 1
      if( SensorChk.Id == ID_FUEL_FLOW )                  // Durchflußgeber Test ?
        Val = GData.EEprom.NV.FuelFlowCntTot;
      #endif
  //  if( SensorChk.Id == ID_GPS )                        // seriellen Empfang testen ?
  //    Val = SensGps->Read_RxChCnt();
      }

    if( SensorChk.StartVal == 0L )                        // 1. Aufruf ?
      {
      SensorChk.StartVal = Val;
      SensorChk.MesureVal = 0L;
      GData.Signal |= SIG_JB_REFRESH;                     // Refresh der Anzeige auf 0
      }
    if( SensorChk.StartVal != Val )                       // Änderung, tut sich was ?
      {
      SensorChk.MesureVal = Val - SensorChk.StartVal;
      GData.Signal |= SIG_JB_REFRESH;                     // Refresh der Anzeige im akt. Menu
      }
    }

    
  // ---------------------------------
  // zykl. Refresh Telemetrie Sensoren
  // ---------------------------------
  //if( Tele.TmoRefreshTmo && (--Tele.TmoRefresh == 0) )
  if( --Tele.TmoRefresh <= 0 )
    {
    Tele.TmoRefresh = TMO_TELE_SND;           // delay bis zum nächsten Check


    // Drehzahl
    #if _SENS_RPM_ == 1
    if( PBIT_TST(Tele.AvailMsk, ID_RPM) )
      JetiEx.SetSensorValue( ID_RPM, SensRpm->TVal.Rpm );
    if( PBIT_TST(Tele.AvailMsk, ID_RPM_REVOL) )
      JetiEx.SetSensorValue( ID_RPM_REVOL, SensRpm->TVal.RevolTotDez2 );
    if( PBIT_TST(Tele.AvailMsk, ID_RPM_RUNTIME) )          // Anzeige in Stunden mit 2 dezimal Stellen, 100Min > 1.40
      JetiEx.SetSensorValue( ID_RPM_RUNTIME, SensRpm->TVal.RunTimeDez2 );
    #endif

    // Flowmeter
    // ---------
    #if _SENS_FUEL_ == 1
    if( PBIT_TST(Tele.AvailMsk, ID_FUEL_REMAIN) )
      JetiEx.SetSensorValue( ID_FUEL_REMAIN, SensFuel->TVal.VolumeRemain );
    if( PBIT_TST(Tele.AvailMsk, ID_FUEL_CONSUMED) )
      JetiEx.SetSensorValue( ID_FUEL_CONSUMED, SensFuel->TVal.VolumeConsumed );
    if( PBIT_TST(Tele.AvailMsk, ID_FUEL_FLOW) )
      JetiEx.SetSensorValue( ID_FUEL_FLOW, SensFuel->TVal.Flow );
    if( PBIT_TST(Tele.AvailMsk, ID_FUEL_TOT) )
      JetiEx.SetSensorValue( ID_FUEL_TOT, SensFuel->TVal.FlowTotDez2 );
    #endif

    // Höhe
    // ----
    #if _SENS_ALT_ == 1
    if( PBIT_TST(Tele.AvailMsk, ID_ALT_REL) )
      JetiEx.SetSensorValue( ID_ALT_REL, SensAlt->TVal.AltitudeRelativ );
    if( PBIT_TST(Tele.AvailMsk, ID_ALT_TOT) )
      JetiEx.SetSensorValue( ID_ALT_TOT, SensAlt->TVal.AltitudeTotDez2 );
    #if _SENS_ALT_TEMP_ == 1
    if( PBIT_TST(Tele.AvailMsk, ID_ALT_TEMP) )
      JetiEx.SetSensorValue( ID_ALT_TEMP, SensAlt->TVal.TempDez1 );
    #endif
    // Vario
    #if _SENS_ALT_VARIO_ == 1
    if( PBIT_TST(Tele.AvailMsk, ID_ALT_VARIO) )
      JetiEx.SetSensorValue( ID_ALT_VARIO, SensAlt->TVal.VarioDez2 );
    #endif
    #endif

    // Pitot Speed
    // -----------
    #if _SENS_PITOT_ == 1
    if( PBIT_TST(Tele.AvailMsk, ID_PITOT_SPEED) )
      JetiEx.SetSensorValue( ID_PITOT_SPEED, SensPitot->TVal.Speed );
    #endif

    // Temperatur
    // ----------
    #if _SENS_TEMP_ == 1
    for( Idx = 0; Idx < CNT_SENS_TEMP; Idx++ )
      if( PBIT_TST(Tele.AvailMsk, ID_TEMP1+Idx) )
        JetiEx.SetSensorValue( ID_TEMP1+Idx, SensTemp[Idx]->TVal.Temp );
    #endif

    // Spannung 1-4
    // ------------
    #if _SENS_VOLT_ == 1
    uint16_t VoltLast = 0;
    for( Idx = 0; Idx < CNT_SENS_VOLT; Idx++ )
      if( PBIT_TST(Tele.AvailMsk, ID_PWR_VOLT1+Idx) )
        {
        JetiEx.SetSensorValue( ID_PWR_VOLT1+Idx, SensPwrVolt[Idx]->TVal.VoltageDez2 - VoltLast);
        if( GData.EEprom.Data.CfgVoltFct == CFG_VOLT_FCT_DIFF )   // Differenz für Einzelzellen Messung ?
          VoltLast = SensPwrVolt[Idx]->TVal.VoltageDez2;
        }
    #endif

    // Energie (Strom/Spg/Watt/Verbrauch)
    // ----------------------------------
    #if _SENS_MUI_  == 1
    for( Idx = 0; Idx < CNT_SENS_MUI; Idx++ )
      {
      if( PBIT_TST(Tele.AvailMsk, ID_PWR_MUI1_A+Idx) )
        JetiEx.SetSensorValue( ID_PWR_MUI1_A+Idx, SensPwrMui[Idx]->TVal.CurrentDez2 );
      if( PBIT_TST(Tele.AvailMsk, ID_PWR_MUI1_V+Idx) )
        JetiEx.SetSensorValue( ID_PWR_MUI1_V+Idx, SensPwrMui[Idx]->TVal.VoltageDez2 );
      if( PBIT_TST(Tele.AvailMsk, ID_PWR_MUI1_W+Idx) )
        JetiEx.SetSensorValue( ID_PWR_MUI1_W+Idx, SensPwrMui[Idx]->TVal.Watt );
      if( PBIT_TST(Tele.AvailMsk, ID_PWR_MUI1_C+Idx) )
        JetiEx.SetSensorValue( ID_PWR_MUI1_C+Idx, SensPwrMui[Idx]->TVal.CapaConsumed );
      }
    #endif

    #if _SENS_GPS_ == 1
    // GPS
    // ---
    if( PBIT_TST(Tele.AvailMsk, ID_GPS_SATS) )
      JetiEx.SetSensorValue( ID_GPS_SATS, SensGps->TVal.Sats );
    if( PBIT_TST(Tele.AvailMsk, ID_GPS_LAT) )
      JetiEx.SetSensorValueGPS( ID_GPS_LAT, false, SensGps->TVal.Latitude );
    if( PBIT_TST(Tele.AvailMsk, ID_GPS_LON) )
      JetiEx.SetSensorValueGPS( ID_GPS_LON, true, SensGps->TVal.Longitude );
    if( PBIT_TST(Tele.AvailMsk, ID_GPS_ALT) )
      JetiEx.SetSensorValue( ID_GPS_ALT, SensGps->TVal.Altitude );
    if( PBIT_TST(Tele.AvailMsk, ID_GPS_SPEED) )
      JetiEx.SetSensorValue( ID_GPS_SPEED, SensGps->TVal.Speed );
    if( PBIT_TST(Tele.AvailMsk, ID_GPS_SPEED_MAX) )
      JetiEx.SetSensorValue( ID_GPS_SPEED_MAX, SensGps->TVal.SpeedMax );
    if( PBIT_TST(Tele.AvailMsk, ID_GPS_HEAD) )
      JetiEx.SetSensorValue( ID_GPS_HEAD, SensGps->TVal.Heading );
    if( PBIT_TST(Tele.AvailMsk, ID_GPS_COURSE_TO) )
      JetiEx.SetSensorValue( ID_GPS_COURSE_TO, SensGps->TVal.CourseTo );
    if( PBIT_TST(Tele.AvailMsk, ID_GPS_DIST_TO) )
      JetiEx.SetSensorValue( ID_GPS_DIST_TO, SensGps->TVal.DistTo );
    if( PBIT_TST(Tele.AvailMsk, ID_GPS_DIST_TRIP) )
      JetiEx.SetSensorValue( ID_GPS_DIST_TRIP, SensGps->TVal.DistTrip );
    if( PBIT_TST(Tele.AvailMsk, ID_GPS_DIST_TOT) )
      JetiEx.SetSensorValue( ID_GPS_DIST_TOT, SensGps->TVal.DistTotDez2 );
    #endif

    // PWM Messung
    #if _SENS_PWM_ == 1
    if( PBIT_TST(Tele.AvailMsk, ID_PWM) )
      JetiEx.SetSensorValue( ID_PWM, SensPwm->TVal.Pwm );
    #endif

    #if _SENS_ACC_ == 1
    // Beschleunigung / 3D Raum
    // ------------------------
    if( PBIT_TST(Tele.AvailMsk, ID_ACC_GFORCE_X) )
      JetiEx.SetSensorValue( ID_ACC_GFORCE_X, SensAcc->TVal.GForceXDez1 );
    if( PBIT_TST(Tele.AvailMsk, ID_ACC_GFORCE_Y) )
      JetiEx.SetSensorValue( ID_ACC_GFORCE_Y, SensAcc->TVal.GForceYDez1 );
    if( PBIT_TST(Tele.AvailMsk, ID_ACC_GFORCE_Z) )
      JetiEx.SetSensorValue( ID_ACC_GFORCE_Z, SensAcc->TVal.GForceZDez1 );
    #endif
    }
}


// ==============================================
// Tasten Abfrage aktivieren, Rückgabe ist SW Idx
// ==============================================
uint8_t MainEnableSw( uint8_t ResFct )
{
  // Digi Port D4-D7 ?
  if( ResFct && ResFct <= CNT_PORT_DIGI_SW )
    {
    ResFct += PORT_DIGI_SW_FIRST-1;     // Idx 1 = D4 ...
    pinMode( ResFct, INPUT_PULLUP );
    BIT_SET(GData.Switch.Enable, ResFct);
    }
  else
    ResFct = 0;
  return( ResFct );                     // 0 == kein Taster, sonst D4-D7
}


// ===================
// EEProm read / write
// ===================
void EEpromRW( bool Wr, bool WrNV )
{
  uint8_t  ByteCnt = WrNV ? sizeof(GData.EEprom.NV) : sizeof(GData.EEprom);
  uint8_t *Pos = (uint8_t *)&GData.EEprom;

  if( Wr )
    EEpromChk();
  for( uint8_t Idx = 0; Idx < ByteCnt; Idx++ )
    {
    if( Wr )
      {
      //noInterrupts();           // write muss kontinuierlich erfolgen, steuert angeblich die Lib selber
      EEPROM.update( Idx, *Pos ); // nur bei Abweichung schreiben
      //interrupts();
      }
    else
      *Pos = EEPROM.read( Idx );
    Pos++;
    }
  if( !Wr )
    EEpromChk();
}

// EEProm gültig ?
// ===============
void EEpromChk( void )
{
  if( GData.EEprom.NV.Magic != EEP_MAGIC_NV )         // Fabrikneu ?
    {
    memset( &GData.EEprom.NV, 0, sizeof(GData.EEprom.NV) ); // Summenwerte auch initialisieren
    GData.EEprom.NV.Magic = EEP_MAGIC_NV;
    }

  if( GData.EEprom.Data.Magic != EEP_MAGIC_DATA )
    {
    memset( &GData.EEprom.Data, 0, sizeof(GData.EEprom.Data) );
    GData.EEprom.Data.Magic = EEP_MAGIC_DATA;
    // default Werte setzen
    GData.EEprom.Data.AVcc             = 5000;               // ADC Referenzsspg.

    #if _SENS_RPM_ == 1
    GData.EEprom.Data.CfgRpmAccuracy   = RPM_ACCURACY_DEF;   // RPM Anzeige, Genauigkeit n UPM
    #endif
    #if _SENS_FUEL_ == 1
    GData.EEprom.Data.CfgFuelVolume    = FUEL_VOLUME_DEF;    // Tankvolumen
    GData.EEprom.Data.CfgFuelFlowCnt1L = FUEL_FLOWCNT_1L;    // # Pulse / Liter
    #endif
    #if _SENS_ALT_VARIO_ == 1 && _SENS_ALT_ == 1
    GData.EEprom.Data.CfgVarioSmoothing = VARIO_SMOOTHING_FILTER; // default Wert für Vario Filter
    #endif
    #if _SENS_MUI_ == 1
    for( int Idx = 0; Idx < CNT_SENS_MUI; Idx++ )
      GData.EEprom.Data.CfgMuiCalib[Idx] = ACS_CALIB;        // default Offs für MUI, Bereich zur Kalibrierung
    #endif
    }

  // ADC uV je Tick, AVcc / 1023 Schritte, entspr. konfigurierter AVcc
  GData.ADCmV = (float)GData.EEprom.Data.AVcc / MAX_ANALOG_READ;
}



//
