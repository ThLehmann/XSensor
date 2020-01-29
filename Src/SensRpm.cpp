/*************************************************************
  RPM Sensor
  
  - Auslösung über Hallgeber am INT0/1 Pin
  - 1 oder 2 Auslösungen je Umdrehung
  - Glättung, keine und in 10er Schritten 10-100
  ------------------------------------------------------------
  Hallsensor TLE4905L, Conrad Bestellnr: 153751-62
    
  Copyright (C) 2017 Thomas Lehmann
  
  Version history:
  1.00   19.06.2017  created

***************************************************************/

#include "SensRpm.h"


#if _SENS_RPM_ == 1


#define RPMCNT2SAVE   1000              // ab 1000 UPM läufts Motörchen, erst dann Summenwerte sichern

SensorRpm *_pInstRpm = 0;               // instance pointer um die SensorFuel class vom ISR zu finden


//SensorRpm::SensorRpm() : Rpm( 0 )


#define Accuracy    GData.EEprom.Data.CfgRpmAccuracy
#define NV_RevolTot GData.EEprom.NV.RpmRevolTot
#define NV_RunTimeS GData.EEprom.NV.RpmRunTimeS

bool SensorRpm::Init( void )
{
  if( _pInstRpm )                             // bereits gestartet ?
    return( true );
  _pInstRpm = this;

  RevolStart = NV_RevolTot + RPMCNT2SAVE;    // ab n UPM läufts Motörchen

  // Hallgeber   > INTx, fallende Flanke löst IRQ aus
  // Pin2 oder 3 > INT0/1
	pinMode( PIN_RPM_HALL, INPUT_PULLUP );
	attachInterrupt( digitalPinToInterrupt(PIN_RPM_HALL), HallIRQ, FALLING );

  return( true );
}


// ======================
// capture Hall interrupt
// ======================
void HallIRQ( void )    // >> ISR( PCINT0_vect ) PIN change IRQ
{
 static uint8_t CaptureCnt;                       // # Impulse für eine Umdrehung

  if( CaptureCnt-- == 0 )                         // Umdrehung komplett ?
    {
    static uint8_t  CaptureIdx;
    static uint32_t LastMicros;
    uint32_t uS = micros();

    //  500UPM = 120mS
    // 8000UPM = 7,5mS
    // für RPM Anzeige n Messungen zur Durchschnittsberechnung sichern
    _pInstRpm->MesureAvg[CaptureIdx++] = uS - LastMicros;
    CaptureIdx &= RPM_AVG_MSK;
    LastMicros = uS;
    NV_RevolTot++;                               // Summe der Umdrehungen
    CaptureCnt = GData.EEprom.Data.CfgRpmCntMag; // # Magnete, 0=einer, 1=zwei
    }
}

// ==========
// 10mS Timer
// ==========
void SensorRpm::Tim10mS()
{
 static uint32_t RevolCntLast;
 static uint8_t Sec;

  // Betriebszeit und Gesamtumdrehungen
  if( ++Sec >= 100 )
    {
    Sec = 0;
    if( NV_RevolTot > RevolStart )                // mind 1000 Umdrehungen abgespult ? Motörchen läuft
      NV_RunTimeS++;                              // Betriebszeit in Sek.

    // Tele Anzeige in Stunden mit 2 dezimal Stellen, 100Min > 140 > 1.40, 1000Min > 1640 > 16.40
    uint32_t RTimeCalc = NV_RunTimeS / 60;        // Anzeige erfolgt in Minuten
    TVal.RunTimeDez2 = ((RTimeCalc/60)*100) + (RTimeCalc%60);

    // # Umdrehungen gesamt Tele Anzeige in 10K mit 2 dezimal Stellen, 1Mil1ion 234tausend  > 1.23
    TVal.RevolTotDez2 = NV_RevolTot/10000;
    }


  // -------------------------------
  // RPM Messung bewerten, alle n mS
  // -------------------------------
  if( TmoRpmCalc )
    TmoRpmCalc--;
  else if( NV_RevolTot > RevolCntLast )           // Messung (IRQ) erfolgt ? oder Init ?
    {
    if( NV_RevolTot - RevolCntLast >= RPM_RUNNING )  // läuft Motor ? oder sind es nur Startversuche
      {
      uint32_t Avg = 0L;
      uint8_t Idx = RPM_AVG_CNT;
      while( Idx-- )
        Avg += MesureAvg[Idx];                    // Summe der Messungen in uS
      Avg >>= RPM_AVG_SH;                         // Sum/n, Mittelwert Impulsabstand in uS
      // # uS für eine Umdrehung auf 1 Min hochrechnen
      if( Avg )                                   // Division durch 0 vermeiden ;)
        {
        TVal.Rpm = (60L*1000L*1000L) / Avg;
        if( Accuracy )                            // Ergebnis glätten, Genauigkeit n UPM
          {
          TVal.Rpm += (Accuracy/2);               // auf/abrunden
          TVal.Rpm -= (TVal.Rpm % Accuracy);
          }
        }
      }

    RevolCntLast = NV_RevolTot;                   // auf nächste Messung warten
    TmoRpmCalc = TMO_RPM_CALC;                    // nur alle n mS bewerten und das nur bei Änderungen
    TmoRpmOff  = TMO_RPM_OFF;                     // Zeit bis RPM = 0 wenn keine Impulse mehr eintrudeln
    if( NV_RevolTot > RevolStart )                // mind 1000 Umdrehungen abgespult ? unerfolgreiche Startversuche zählen nicht
      GData.Signal |= SIG_SAVE2NV;                // Motörchen läuft, Summenwerte speichern
    }


  // Timeout Drehzahlmessung ? Motor steht
  // -------------------------------------
  if( TmoRpmOff && (--TmoRpmOff == 0) )
    {
    TVal.Rpm = 0;                                 // Drehzahl = 0 anzeigen
    RevolStart = NV_RevolTot + RPMCNT2SAVE;       // warten auf min n Umdrehungen, unerfolgreiche Startversuche verwerfen
    }
}

#endif








/*
void HallIRQ( void )    // >> ISR( PCINT0_vect ) PIN change IRQ
{
 static uint16_t CaptureCnt;
 static long     SampleTime;
   #define RPM_SAMPLE_TIME_MS 1000L               // Messdauer 1Sek.

  // 1000 UPM ==  60mS / Umdrehung >  17 / Sek
  // 8000 UPM == 7.5mS / Umdrehung > 133 / Sek (!bei 2Mags > 266)
  if( CaptureCnt++ == 0 )
    SampleTime = millis() + RPM_SAMPLE_TIME_MS;   // Messung starten
  else if( millis() > SampleTime )                // Messung beendet
    {
    if( CaptureCnt > 2 )
      _pInstRpm->CaptureCnt = CaptureCnt-2;       // ohne Start und Endimpuls, != 0 > Messung bewerten
    CaptureCnt = 0;                               // nächste Messung starten
    }
  // else // Messung aktiv, # Impulse zählen
}

// ==========
// 10mS Timer
// ==========
void SensorRpm::Tim10mS()
{
  // RPM Messung bewerten, jede Sekunde
  // ----------------------------------
  if( CaptureCnt )
    {
    TmoRpmOff = TMO_RPM_OFF;                      // Zeit bis RPM = 0 wenn keine Impulse mehr eintrudeln

    // # Impulse während Messdauer auf 1 Min hochrechnen
  TValRpm = ((60L*1000L)/RPM_SAMPLE_TIME_MS) * (CaptureCnt >> CntMag);
    if( Accuracy )
      {
      TValRpm /= Accuracy;                        // Ergebnis glätten, Genauigkeit n UPM
      TValRpm *= Accuracy;
      }
    CaptureCnt = 0;                               // nächste Messung abwarten
    }

  // Timeout Drehzahlmessung ?
  if( TmoRpmOff && (--TmoRpmOff == 0) )
    TValRpm = 0;                                  // Drehzahl = 0 anzeigen
}
*/



/* Messung über Zeitabstand
 static long  HallTimeLast;
 static UCHAR MessCnt;

  // 1000 UPM ==  60mS / Umdrehung
  // 8000 UPM == 7.5mS / Umdrehung
  if( Rpm.HallTime == 0 )
    {
    MessCnt = 0;
    ++Rpm.HallTime;
    }
  else if( ++MessCnt <= 8 )
    Rpm.HallTime += (micros() - HallTimeLast);    // Impulsdauer dazu
  else
    Signal |= SIG_RPM;                            // Messung bewerten, bei Halltime == 0 nächste Messung starten

  HallTimeLast = micros();
*/
/*
 long HallTime;

  noInterrupts();
  HallTime = Rpm.HallTime;          // letzte/aktuelle Messung
  Rpm.HallTime = 0;                 // neue Messung starten
  interrupts();

  if( HallTime == 0 )               // doch noch nichts gemessen ?
    return;

  // Messung in mS, es sind 8 Messungen gemacht worden
  // 1 Minute / Intervall == Drehzahl/Min
  Rpm.TVal = (60L*1000L*1000L*8L) / (HallTime << EEprom.RpmCntMag);
//Rpm.TVal &= 0xffe0;               // Ergebnis glätten, Genauigkeit 32 UPM
  if( EEprom.RpmAccuracy )
    {
    Rpm.TVal /= EEprom.RpmAccuracy; // Ergebnis glätten, Genauigkeit n UPM
    Rpm.TVal *= EEprom.RpmAccuracy;
    }
  Rpm.Tmo = TMO_RPM;                // Zeit bis Messung beendet wird wenn keine Impulse mehr eintrudeln
*/
