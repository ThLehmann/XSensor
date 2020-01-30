/*************************************************************
  GPS Sensor
  
  Sensor: NEO 6M
  der NMEA Stream vom Sensor wird über TiniGps++ Library ausgewertet
  http://arduiniana.org/libraries/tinygpsplus/
  ------------------------------------------------------------
  
  Copyright (C) 2017 Thomas Lehmann
  
  Version history:
  1.00   21.06.2017  created

***************************************************************/


#include "SensGPS.h"

#if _SENS_GPS_ == 1


#define NV_DistTot    GData.EEprom.NV.GpsDistTot
#define NV_SpeedMax   GData.EEprom.NV.GpsSpeedMax


// ====
// Init
// ====
bool SensorGps::Init( uint8_t Id )
{
  // SensActiv[SensId] = TRUE;             // Sensoren aktivieren
  if( SoftSerial )                         // bereits gestartet ?
    return( true );
  SoftSerial = new AltSoftSerial;          // serielle auf PIN 8 (RX) + 9 (TX) !! IRQ getrieben !!
  SoftSerial->begin( 9600 );
  delay(10);

  Tmo = GPS_TMO_REFRESH + Id;              // 10mS Timer asynchron zu anderen Sensoren bedienen
  ChkFailureTmo = TMO_CHK_FAILURE;         // alle n Sek die Funktionalität prüfen
  return( true );

/*
unsigned char cfg_prt[28] = {
  0xB5,0x62,0x06,0x00,0x14,0x00,
  0x01,0x00,0x00,0x00,0xD0,0x08,0x00,0x00,
  0x00,0xE1,0x00,0x00,0x07,0x00,0x03,0x00,  // 57600
  0x00,0x00,0x00,0x00,
  0xDE,0xC9 };
uint8_t Cnt;
  for( Cnt = 0; Cnt < sizeof(cfg_prt); Cnt++ )
    SoftSerial->write(cfg_prt[Cnt]);  //set baud rate to 115200
  SoftSerial->flush();
  delay(10);
  SoftSerial->end();
  SoftSerial->begin(57600);
*/
}




// ==================================================
// zyklischer Aufruf aus Loop()
// seriellen Stream vom GPS Sensor lesen und bewerten
// ==================================================
void SensorGps::Loop()
{
 char Ch;

  if( (Ch = SoftSerial->read()) != -1 )         // Zeichen empfangen ?
    #if DEBUG_GPS == 0
    if( GpsPlus.encode(Ch) )                    // zur Decodierung geben, TRUE == Frame komplett und gültig (CRC)
      FrameCnt++;
    #else
    {
    DbgPrintf("%c", Ch);
    if( GpsPlus.encode(Ch) )                    // Frame komplett ?
      {
      FrameCnt++;
//    DbgPrintf("\nValid:%d isUpd:%d age:%d Sats:%d\n", GpsPlus.location.isValid(), GpsPlus.location.isUpdated(), GpsPlus.location.age(), GpsPlus.satellites.value());
      }
    }
    #endif
}



// ==========
// 10mS Timer
// ==========
void SensorGps::Tim10mS()
{
  // auf Fehler/Probleme prüfen
  if( ChkFailureTmo && (--ChkFailureTmo == 0) )
    {
    static uint16_t LastFrameCnt;

    ChkFailureTmo = TMO_CHK_FAILURE;
    if( FrameCnt < LastFrameCnt )         // empfangen wir gültige Daten ?
      Failure = true;
    else
      Failure = false;
    LastFrameCnt = FrameCnt + 20;         // min. 20 Frames sollten es schon sein
    }

  if( --Tmo == 0 )
    {
    Tmo = GPS_TMO_REFRESH;
    if( !Failure )
      RefreshSensors();                   // zyklsich gelesene Daten ablegen
    // Tele Anzeige max. Werte auch OHNE das der Sensor funktioniert
    TVal.DistTotDez2  = NV_DistTot / 10;  // Anzeige in km, 2345m > 23.45 / 10 = 2.34km
    TVal.SpeedMax = NV_SpeedMax;
    }
}


// ==================================
// Refresh aller aktivierten Sensoren
// ==================================
void SensorGps::RefreshSensors()
{
  // IsUpdated() wird gesetzt wenn entspr. Info vom GPS empfangen wurden
  // lesen des entspr. Wertes löscht IsUpdated Flag

  // Empfang OK ? wird Pos zyklisch gemeldet ? Infos frisch ?
  if( !GpsPlus.location.isUpdated() || GpsPlus.location.age() > 2000 )
    return;

  // Position
  TVal.Sats     = GpsPlus.satellites.value();
  TVal.Latitude = GpsPlus.location.lat();
  TVal.Longitude= GpsPlus.location.lng();

  if( !HomePos.Known )                  // Startposition unbekannt ?
    {
    static uint8_t Retrys;

    if( Retrys++ < GPS_TMO_HOMESET )    // lassen wir es mal ein klein wenig einschwingen ...
      return;
    HomePos.Known = true;
    HomePos.Lat = Trip.LastLat = GpsPlus.location.lat();
    HomePos.Lon = Trip.LastLon = GpsPlus.location.lng();
    HomePos.Alt = Trip.LastAlt = GpsPlus.altitude.meters();
//  Trip.Dist = 0;
    }


  // Höhe
  if( GpsPlus.altitude.isUpdated() )
    {
    /*
    static uint8_t Idx;
    static float   Avg[8];
    static float   Sum;
    // Durschnitt von n Messungen
    Sum -= Avg[Idx];
    Avg[Idx] = GpsPlus.altitude.meters();
    Sum += Avg[Idx];
    ++Idx &= 0x07;

    // bei Start Höhe über NN, nach Pos-Update relative Höhe ggü. Startpunkt
    if( (TValAltitude = (Sum/8) - HomePos.Alt) < 0 )
      TValAltitude = 0;                 // wir wollen ja nicht unterirdisch fliegen ;)
    */
    // bei Start Höhe über NN, nach Pos-Update relative Höhe ggü. Startpunkt
    TVal.Altitude = GpsPlus.altitude.meters() - HomePos.Alt;
    }

  
  // Geschwindigkeit
  if( GpsPlus.speed.isUpdated() )
    {
    TVal.Speed = GpsPlus.speed.kmph();
    if( TVal.Speed > NV_SpeedMax )
      {
      NV_SpeedMax = TVal.Speed;
      GData.Signal |= SIG_SAVE2NV;            // Summenwerte bei Stillstand speichern
      }
    }

  // akt. Richtung
  if( GpsPlus.course.isUpdated() )
    TVal.Heading = GpsPlus.course.deg();


  // Entfernung zum Modell vom Startpunkt gesehen
  TVal.DistTo = GpsPlus.distanceBetween( HomePos.Lat, HomePos.Lon, GpsPlus.location.lat(), GpsPlus.location.lng() );
  // Richtung zum Modell vom Startpunkt gesehen
  TVal.CourseTo = GpsPlus.courseTo( HomePos.Lat, HomePos.Lon, GpsPlus.location.lat(), GpsPlus.location.lng() );
  
  
  // 3D Distanz
  // ----------
  float AltDiff;
  if( Trip.LastAlt >= GpsPlus.altitude.meters() )
    AltDiff = Trip.LastAlt - GpsPlus.altitude.meters();
  else
    AltDiff = GpsPlus.altitude.meters() - Trip.LastAlt;

  // zurückgelegte Strecke
  float Dist   = GpsPlus.distanceBetween( Trip.LastLat, Trip.LastLon, GpsPlus.location.lat(), GpsPlus.location.lng() );
  // inkl. Höhendifferenz
  Dist = sqrt(pow(AltDiff,2) + pow(Dist,2));
  if( Dist > 10 && GpsPlus.speed.kmph() > 10 )   // bewegen wir uns ?
    {
    Trip.Dist  += Dist;
    NV_DistTot += Dist;                // Gesamt zurückgelegte Strecke
    GData.Signal |= SIG_SAVE2NV;      // Summenwerte bei Stillstand speichern
    Trip.LastLat = GpsPlus.location.lat();
    Trip.LastLon = GpsPlus.location.lng();
    Trip.LastAlt = GpsPlus.altitude.meters();
    }
  
  // Tele Anzeige
  TVal.DistTrip     = Trip.Dist;          // Anzeige in m
}




#if DEBUG_GPS == 2
TermPutln("");
TermPutTxt("GPS: ");

TermPutTxt(" Sats: ");
TermPut(TVal.Sats);
TermPutTxt(" Lat: ");
TermPut(TVal.Latitude);
TermPutTxt(" Lon: ");
TermPut(TVal.Longitude);
TermPutTxt(" age: ");
TermPut(GpsPlus.location.age());

TermPutTxt(" Alt abs: ");
TermPut(GpsPlus.altitude.meters());
TermPutTxt(" Alt rel: ");
TermPut(HomePos.Alt);
TermPutTxt("/");
TermPut(TVal.Altitude);

TermPutTxt(" Speed: ");
TermPut(TVal.Speed);
TermPutTxt("\n");


const float EIFFEL_TOWER_LAT = 48.85826;
const float EIFFEL_TOWER_LNG = 2.294516;
float distanceKm = GpsPlus.distanceBetween( GpsPlus.location.lat, GpsPlus.location.lng, EIFFEL_TOWER_LAT, EIFFEL_TOWER_LNG ) / 1000.0;
float CourseTo = GpsPlus.courseTo( GpsPlus.location.lat, GpsPlus.location.lng, EIFFEL_TOWER_LAT, EIFFEL_TOWER_LNG );
TermPutTxt("Dist (km) to Eifel Tower: ");
TermPutln( distanceKm );
TermPutTxt("Heading: ");
TermPutln(CourseTo);
TermPutTxt("\n");
#endif



#endif
