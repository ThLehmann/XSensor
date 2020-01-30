/*************************************************************
  GPS Sensor
  
  Sensor: NEO 6M an PIN 8+9 über Softserial COM3
  ------------------------------------------------------------
  
  Copyright (C) 2017 Thomas Lehmann
  
  Version history:
  1.00   21.06.2017  created

***************************************************************/

#ifndef SENS_GPS_H
#define SENS_GPS_H


#include "Def.h"

#if _SENS_GPS_ == 1


#include "AltSoftSerial.h"
#include "TinyGPS.h"




class SensorGps
{
public:

  bool Init( uint8_t Id );
  void Loop();
  void Tim10mS();

  // Fehler / Analyse
  bool Failure;                             // TRUE == Fehler bei der Verarbeitung oder GPS Sensor liefert keine Daten

  // akt. GPS Daten
  struct
    {
    uint8_t  Sats;                          // # Satelliten
    float    Latitude;                      // Latitude
    float    Longitude;                     // Longitude
    int16_t  Altitude;                      // Höhe über Startpunkt in Meter
    uint16_t Speed;                         // Geschwindigkeit in km/h
    uint16_t SpeedMax;                      // max. Geschwindigkeit in km/h
    uint16_t Heading;                       // Flugrichtung in Grad
    uint16_t CourseTo;                      // Richtung zum Modell in Grad, vom Startpunkt gesehen
    uint16_t DistTo;                        // Entfernung zum Modell in Meter, vom Startpunkt gesehen
    uint32_t DistTrip;                      // zurückgelegte Strecke aktueller Flug
    uint32_t DistTotDez2;                   // gesamt zurückgelegte Strecke
    }TVal;


private:

  TinyGPSPlus   GpsPlus;
  AltSoftSerial *SoftSerial;

  uint16_t FrameCnt;                        // # gültige Frames empfangen

  uint8_t  Tmo;
    #define GPS_TMO_REFRESH     (200/TIM_BASE)    // 200mS, vom GPS gelesene Werte bewerten
    #define GPS_TMO_HOMESET     (5000/200)        // Homeposition erst 5 Sek. nach gültigen GPS Empfang setzen
  uint16_t ChkFailureTmo;
    #define TMO_CHK_FAILURE (5000/TIM_BASE) // alle 5Sek die Funktionalität prüfen

  void RefreshSensors();

  // Daten zur Startposition
  struct
    {
    bool  Known;
    float Lat; 
    float Lon;
    float Alt;
    }HomePos;

  // zurückgelegte Strecke
  struct
    {
    uint32_t Dist;                       // zurückgelegte Strecke
    float    LastLat; 
    float    LastLon;
    long     LastAlt;
    }Trip;

};


#endif

#endif




