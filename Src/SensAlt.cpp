/*************************************************************
  Höhen Sensor
  
  Sensor: MS5611 (GY-63)
  I2C Adrtesse 77h

  MS5611 ist wesentlich genauer

  MS5611:  24bit ADC, 4096 osr, best update rate: 50Hz, Auflösung 10cm
  BMP180:  19bit ADC, 8 osr, best update rate: 33Hz
  ------------------------------------------------------------
  
  Copyright (C) 2017 Thomas Lehmann
  
  Version history:
  1.00   19.06.2017  created
  1.01   01.07.2017  BMP180 hinzugefügt

***************************************************************/


#include <Wire.h>

#include "SensAlt.h"


#if _SENS_ALT_ == 1


#if HW_TERM == 1
#define DBG_5611_VALUES   0                       // 1 == gelesene und berechnete Werte ausgeben
#else
#define DBG_5611_VALUES   0
#endif

// ====
// Init
// ====
bool SensorAlt::Init( uint8_t Id, bool VarioActiv )
{
  if( TmoRefresh )                                // bereits gestartet ?
    return( true );

  #if _SENS_ALT_VARIO_  == 1
  Vario.Activ = VarioActiv;
  #endif

  // MS5611 Sensor vorhanden / ansprechbar ?
  Wire.begin();                                   // I2C als Master starten
  Wire.beginTransmission( MS5611_ADDRESS );       // Sensor andingsen
  if( Wire.endTransmission() )                    // 0 == korrekte Antwort
    return( false );                              // Sensor nicht angeschlossen

  WriteCmd( MS5611_CMD_RESET );
  delay( 100 );

  //readPROM();
  for( uint8_t offset = 0; offset < ARRAYCNT(MS5611.fc); offset++ )
    {
    MS5611.fc[offset] = ReadReg(MS5611_CMD_READ_PROM + (offset * 2), 2);
    #if DBG_5611_VALUES == 1
    DbgPrintf("FC[%d] %u\n", offset+1, MS5611.fc[offset]);
    #endif
    }

  MS5611.Delay = 1;                               // zykl. MS5611 Abfragen starten

  TmoRefresh = ALT_TMO_REFRESH + Id;              // 10mS Timer asynchron zu anderen Sensoren bedienen
  return( true );
}


// ==================================================
// zyklischer Aufruf aus Loop()
// ==================================================
void SensorAlt::Loop()
{
 static uint32_t D1;                               // vom Sensor gelesener Druck

  // zyklisch Temp und Druck abfragen
  if( millis() < MS5611.Delay )
    return;

  #define CHK_CALC_MS5611 0
  #if CHK_CALC_MS5611  == 1
  // Höhensensor Rechentest
  if( 1 )
    {
    static uint8_t PressTst = 0xff;
    static uint32_t TstVal[10] =
      {
      101325,       // 0,00m  sealevel
      100716,       // 50,85m
      99763,        // 130,89m
      97569,        // 317,50m
      95462,        // 500,00m
      93669,        // 657,85m
      92601,        // 753,00m
      82981,        // 1653,20m
      76812,        // 2276,00m
      70452,        // 2962,00m
      };
    
    if( PressTst++ > 10 )
      MS5611.Delay = millis() + 2000;
    if( PressTst < 10 )
      {
      printFloat(" Alt:", getAltitude(TstVal[PressTst]));
      DbgPrintf(" Press:%lu\n", TstVal[PressTst]);
      MS5611.Delay = millis() + 500;
      }
    return;
    }
  #endif


  // höchste Auflösung die der Sensor zu bieten hat für Variofunktion
  switch( MS5611.State )
    {
    case 0:                                       // read raw Pressure and Delay(10)
      WriteCmd( MS5611_CMD_CONV_D1 + MS5611_ULTRA_HIGH_RES );
      break;
    case 1:                                       // Verzögerung beendet, Pressure auslesen
      D1 = ReadReg( MS5611_CMD_ADC_READ, 3 );
      // read raw Temperature and Delay(10)
      WriteCmd( MS5611_CMD_CONV_D2 + MS5611_ULTRA_HIGH_RES );
      break;
    case 2:                                       // Verzögerung beendet, Temp auslesen und Druck berechnen
      {
      uint32_t D2   = ReadReg( MS5611_CMD_ADC_READ, 3 );
      int32_t  dT   = D2 - ((uint32_t)MS5611.fc[_C5] << 8);           // Differenz zwischen akt. und Ref Temperatur
      #if _SENS_ALT_TEMP_ == 1                    // Temperatur Anzeige vom MS5611
      int32_t  TEMP = 2000 + (((int64_t)dT * MS5611.fc[_C6]) >> 23);  // Temperatur 2310 = 23,10°C, Auflösung 0,01°C, belegt 158Byte Code
      #endif
      // Temperatur kompensierter Druck
      int64_t  OFF  = (((int64_t)MS5611.fc[_C2]) << 16) + (((int64_t)MS5611.fc[_C4] * dT) >> 7);
      int64_t  SENS = (((int64_t)MS5611.fc[_C1]) << 15) + (((int64_t)MS5611.fc[_C3] * dT) >> 8);

      #if DBG_5611_VALUES == 1
      DbgPrintf("\nD1:%lu", D1);
      DbgPrintf(" D2:%lu", D2);
      DbgPrintf(" dT:%ld", dT);
      DbgPrintf(" TEMP:%ld", TEMP);
      print64  (" OFFS:", OFF);
      print64  (" SENS:", SENS);
      
      DbgPrintf("\nC5<<8:%ld", ((uint32_t)MS5611.fc[_C5] << 8));
      print64  (" C2<<16:", ((int64_t)MS5611.fc[_C2] << 16));
      print64  (" C4>>7:",  (((int64_t)MS5611.fc[_C4] * dT) >> 7));
      print64  (" C1<<15:", (((int64_t)MS5611.fc[_C1]) << 15));
      print64  (" C3>>8:",  (((int64_t)MS5611.fc[_C3] * dT) >> 8));
      DbgPrintf("\n");
      #endif

      #if _SENS_ALT_TEMP_ == 1                    // Temperatur Anzeige vom MS5611
      #define PRESS_COMP2ORDER    0
      #if     PRESS_COMP2ORDER == 1
      // Second Order Temperatur Kompensation, belegt 368Byte Code
      if( TEMP < 2000 )
        {
        int32_t Tmp;

        TEMP -= ((int64_t)dT*dT) >> 31;
        // 5 * (TEMP - 2000) * (TEMP - 2000)
        Tmp = TEMP - 2000;
        Tmp *= Tmp;
        Tmp *= 5;
        OFF  -= (Tmp / 2);
        SENS -= (Tmp / 4);

        /* für unsere Anwendung irrelevant
        int32_t T2;
        int64_t OFF2;
        int64_t SENS2;
        OFF2  = Tmp / 2;
        SENS2 = Tmp / 4;
        if( TEMP < -1500 )
          {
          // (TEMP + 1500) * (TEMP + 1500)
          Tmp = TEMP + 1500;
          Tmp *= Tmp;
          OFF2  +=  7 * Tmp;
          SENS2 += 11 * Tmp / 2;
          }
        TEMP -= T2;
        OFF  -= OFF2;
        SENS -= SENS2;
        */
        }
      #endif
      #endif

      // Temperaturkompensierter Druck, 10 - 1200mbar, Auflösung 0,012bar bei oversampling 4096
      // Druck in Pascal (100 Pa = 1 hPa = 1 mbar)
      // 1 digit ~ 8cm
      // NN   = 101325
      // Home ~ 101014 (25m) > offiziell 40m
      int32_t RawPressure = ((int64_t)((((int64_t)(D1 * SENS) >> 21) - OFF) ) >> 15);

      // kleiner Smoothing Filter um Spikes zu elemenieren
      // Anteil neuer Wert 25%, alter Wert 75%
      MS5611.RawPressure = ((float)RawPressure*0.25f) + ((float)MS5611.RawPressure*0.75f);

      if( MS5611.ReadCnt < (5000/100) )           // die ersten 5 Sek. (a' 3x10mS) zum Einpegeln
        {
        MS5611.ReadCnt++;
        MS5611.RawPressure = RawPressure;         // damit smootfilter nicht so lange einregeln muss
        MS5611.RefPressure = RawPressure;         // akt. Druck als Referenz für relative Höhe ggü Start
        //MS5611.RefAltitude = 0;
        //MS5611.RefAltitude = getAltitude(RawPressure);
        }
      else
        MS5611.ReadCnt = 0xff;
      MS5611.State = -1;

      #if DBG_5611_VALUES == 1
      DbgPrintf ("Press rd:%ld", RawPressure);
      DbgPrintf (" Press Raw:%ld Ref:%ld", MS5611.RawPressure, MS5611.RefPressure);
      DbgPrintf (" PressDiff:%ld", MS5611.RefPressure-MS5611.RawPressure);
      printFloat(" Alt:", getAltitude(MS5611.RawPressure, MS5611.RefPressure));
      DbgPrintf ("\n");
      #endif

      #if _SENS_ALT_TEMP_ == 1                    // Temperatur Anzeige vom MS5611
      TVal.TempDez1 = TEMP/10;                    // Temperatur
      #endif
      break;
      }
    }

  MS5611.State++;
  if( MS5611.State )
    MS5611.Delay = millis() + TMO_ULTRA_HIGH_RES;
  else
    MS5611.Delay = millis() + 80;                 // nur alle 100mS eine Abfrage, Höhe und Vario wird nur alle 200mS bedient
}


// =====================================================
// Calculate altitude from Pressure & Sea level pressure
// =====================================================
float SensorAlt::getAltitude( int32_t Pressure, int32_t sealevelPressure )
{
  return( 44330.0f * (1.0f - pow((float)Pressure / (float)sealevelPressure, 0.1902949f)) );
/*
 float rawAltitude;
  
  // if() ausgeliehen vom openXSensor
  // etwas ungenauer dafür deutlich schneller als pow() und benötigt auch noch weniger Code (zumindest bis nirgends ander pow() verwendet wird) 
  //         pow()   if()
  // --------------------
  // Dauer  ~325uS  ~25uS
  // Code   +92Byte

  // Höhengenauigkeit, Messergebnisse vom 09.01.2019
  // Press    Alt         pow()      if() 
  // ------------------------------------
  // 101325    0,00m       0,00m      0,00m
  // 100716   50,85m      50,83m     51,93m
  //  99763   130,89m    130,86m    133,19m
  //  97569   317,50m    317,51m    320,26m
  //  93669   657,85m    657,84m    660,43m
  //  95462   500,00m    499,97m    499,91m
  //  92601   753,00m    753,03m    756,04m
  //  82981  1653,20m   1653,19m   1658,52m
  //  76812  2276,00m   2275,97m   2279,09m
  //  70452  2962,00m   2961,98m   2962,83m

  // other alternative (faster) = 1013.25 = 0 m , 954.61 = 500m , etc...
  //      Pressure  Alt (m) Ratio
  //      101325       0    0.08526603
  //      95461      500    0.089525515
  //      89876     1000    0.094732853
  //      84598     1500    0.098039216
  //      79498     2000    0.103906899
  //      74686     2500    0.109313511
  //      70112     3000    0.115101289
  //      65768     3500    0.121270919
  //      61645     4000    0.127811861
  //      57733     4500    0.134843581
  //      54025     5000    1009106180
  //                1009106180
  if( Pressure > 95461)                   // < 500m / (101325 - 95461)  temp is fixed to 15 degree celcius)
    rawAltitude =        (float)(101325 - Pressure) * 0.08526603f;
  else if( Pressure > 89876 )             // < 1000m
    rawAltitude =  500 + (float)(95461 - Pressure) * 0.089525515f;
  else if ( Pressure > 84598)
    rawAltitude = 1000 + (float)(89876 - Pressure) * 0.094732853f;
  else if ( Pressure > 79498)
    rawAltitude = 1500 + (float)(84598 - Pressure) * 0.098039216f;
  else if ( Pressure > 74686)
    rawAltitude = 2000 + (float)(79498 - Pressure) * 0.103906899f;
  else if ( Pressure > 70112)
    rawAltitude = 2500 + (float)(74686 - Pressure) * 0.109313511f;
  else if ( Pressure > 65768)
    rawAltitude = 3000 + (float)(70112 - Pressure) * 0.115101289f;
  else if ( Pressure > 61645)
    rawAltitude = 3500 + (float)(65768 - Pressure) * 0.121270919f;
  else if ( Pressure > 57733)
    rawAltitude = 4000 + (float)(61645 - Pressure) * 0.127811861f;
  else
    rawAltitude = 4500 + (float)(57733 - Pressure) * 0.134843581f;
  rawAltitude -= MS6511.refAltitude;                      // Höhe über Startpos
  return( rawAltitude );
  */
}


// =====================
// MS5611 I2C read/write
// =====================
void SensorAlt::WriteCmd( uint8_t Cmd )
{
  Wire.beginTransmission( MS5611_ADDRESS );
  Wire.write( Cmd );
  Failure = Wire.endTransmission();
}

int32_t SensorAlt::ReadReg( uint8_t Cmd, int ByteCnt )
{
  WriteCmd( Cmd );
  Wire.requestFrom( MS5611_ADDRESS, ByteCnt );

  int32_t Res = 0L;
  //while( ByteCnt-- )
  while( Wire.available() )
    {
    Res <<= 8;
    Res |= Wire.read();
    }
  return( Res );
}






// ==========
// 10mS Timer
// ==========
void SensorAlt::Tim10mS()
{
  // Sensor aktiv ?
  if( TmoRefresh == 0 )                           // Sensor nicht angeschlossen
    return;

  // Sensor noch nicht eingepegelt ?
  if( MS5611.ReadCnt != 0xff )
    return;

  if( --TmoRefresh )
    return;
  TmoRefresh = ALT_TMO_REFRESH;

  #if VARIO_TEST == 1   // steigen/sinken simulieren
  // je digit 8cm * 5 = 0.40m/s
  // steigen 1 m/s = 120 cm/s > 120/8 = 15 digits/s 
  // alle 200mS = 15/5 = 3
  static uint32_t TmpPressure = 101325;   // NN
  // 0,40m/s
  //TmpPressure -= 1;                  // je digit 8cm * 5 = 0.40m/s
  // 1,20m/s
  TmpPressure -= 3;                    // je digit 8cm * 5/s
  MS5611.RawPressure = TmpPressure;
  #endif



  // ====
  // Höhe
  // ====
                                                  // aktuelle relative Höhe ggü Start in Meter
  TVal.AltitudeRelativ = getAltitude( MS5611.RawPressure, MS5611.RefPressure );

  // Gesamthöhe, nur steigen, summieren
  // ----------------------------------
  #define ALT_HYSTERESE   10                      // Gesamthöhe alle 10m summieren
  if( TVal.AltitudeRelativ > LastAltTot + ALT_HYSTERESE ) // Höhe steigt um HYSTERESE Meter
    {
    GData.EEprom.NV.AltitudeTot += TVal.AltitudeRelativ - LastAltTot;
    GData.Signal |= SIG_SAVE2NV;                  // neue Gesamthöhe, speichern
    LastAltTot = TVal.AltitudeRelativ;
    }
  else if( TVal.AltitudeRelativ + ALT_HYSTERESE < LastAltTot )  // sinken
    LastAltTot = TVal.AltitudeRelativ;

  // Gesamt Höhe in km, 1234m = 1,23km
  TVal.AltitudeTotDez2 = GData.EEprom.NV.AltitudeTot / 1000;


  // =====
  // VARIO
  // =====
  #if _SENS_ALT_VARIO_  == 1

  if( !Vario.Activ )                   // Vario aktiv geschaltet ?
    return;                            // nö, dann sparen wir uns etwas Performance
                                       // akt. Höhe in cm
  long Altitude = getAltitude( MS5611.RawPressure, MS5611.RefPressure ) * 100;
  long uVario = 0L;
//DbgPrintf(" RawPress:%d Alt:%d lastAlt:%d", MS5611.RawPressure, Altitude, Vario.LastAlt);
  if( Vario.LastAlt )
    uVario = (Altitude - Vario.LastAlt) * (1000 / float(millis() - Vario.LastTime));
  Vario.LastAlt = Altitude;
  Vario.LastTime = millis();

//DbgPrintf(" uVario:%d", uVario);

  // Vario Filter
  // IIR Low Pass Filter
  // y[i] := α * x[i] + (1-α) * y[i-1]
  //      := y[i-1] + α * (x[i] - y[i-1])
  // mit α = 1- β
  // y[i] := (1-β) * x[i] + β * y[i-1]
  //      := x[i] - β * x[i] + β * y[i-1]
  //      := x[i] + β (y[i-1] - x[i])
  // see: https://en.wikipedia.org/wiki/Low-pass_filter#Simple_infinite_impulse_response_filter
  // Danke an Rainer Stransky
  
  float SmoothFilt = GData.EEprom.Data.CfgVarioSmoothing / 100.0;
  uVario = uVario + (SmoothFilt * (Vario.LastFilter - uVario));
  Vario.LastFilter = uVario;

  // Dead zone filter
  //  if( uVario > GData.EEprom.Data.CfgVarioDeadzone || uVario < (GData.EEprom.Data.CfgVarioDeadzone * -1) )
  //    uVario = 0;
  if( uVario > GData.EEprom.Data.CfgVarioDeadzone )
    uVario -= GData.EEprom.Data.CfgVarioDeadzone;
  else if( uVario < (GData.EEprom.Data.CfgVarioDeadzone * -1) )
    uVario += GData.EEprom.Data.CfgVarioDeadzone;
  else
    uVario = 0;

  TVal.VarioDez2 = uVario;

//DbgPrintf(" Filt:%d uVarioRes:%d\n", SmoothFilt, uVario);
  #endif
}

#endif




/*
// ****************
// openXSensor Code
// ****************
        if (D2Prev == 0) D2Prev = D2 ;
        D2Apply = (D2 + D2Prev ) >> 1 ;
        D2Prev = D2 ; 
        dT = D2Apply - ((long)_calibrationData[5] << 8);
//      TEMP = (2000 + (((int64_t)dT * (int64_t)_calibrationData[6]) >> 23)) / (float) 1.0 ;
        varioData.temperature = (2000 + (((int64_t)dT * (int64_t)_calibrationData[6]) >> 23)) ; 
//      varioData.temperature= TEMP;
//      OFF  = (((int64_t)_calibrationData[2]) << 16) + ( (_calibrationData[4] * dT) >> 7);
        OFF  = (((int64_t)_calibrationData[2]) << 16) + ( ( (_calibrationData[4] - alt_temp_compensation ) * dT) >> 7);
        SENS = (((int64_t)_calibrationData[1]) << 15) + ((_calibrationData[3] * dT) >> 8);
        varioData.rawPressure= (((((((int64_t) D1) * (int64_t) SENS) >> 21) - OFF) * 10000 ) >> 15) ; // 1013.25 mb gives 1013250000 is a factor to keep higher precision (=1/100 cm).
      
      // altitude = 44330 * (1.0 - pow(pressure /sealevelPressure, 0.1903));
      // other alternative (faster) = 1013.25 = 0 m , 954.61 = 500m , etc...
      //      Pressure  Alt (m) Ratio
      //      101325       0    0.08526603
      //      95461      500    0.089525515
      //      89876     1000    0.094732853
      //      84598     1500    0.098039216
      //      79498     2000    0.103906899
      //      74686     2500    0.109313511
      //      70112     3000    0.115101289
      //      65768     3500    0.121270919
      //      61645     4000    0.127811861
      //      57733     4500    0.134843581
      //      54025     5000    1009106180
      //                1009106180
      if ( realPressure > 954610000) {
        rawAltitude = ( 1013250000 - realPressure ) * 0.08526603 ; // = 500 / (101325 - 95461)  // returned value 1234567 means 123,4567 m (temp is fixed to 15 degree celcius)
      } else if ( realPressure > 898760000) {
        rawAltitude = 5000000 + ( 954610000 - realPressure ) * 0.089525515  ; 
      } else if ( realPressure > 845980000) {
        rawAltitude = 10000000 + ( 898760000 - realPressure ) * 0.094732853  ; 
      } else if ( realPressure > 794980000) {
        rawAltitude = 15000000 + ( 845980000 - realPressure ) *  0.098039216 ; 
      } else if ( realPressure > 746860000) {
        rawAltitude = 20000000 + ( 794980000 - realPressure ) *  0.103906899 ; 
      } else if ( realPressure > 701120000) {
        rawAltitude = 25000000 + ( 746860000 - realPressure ) *  0.109313511 ; 
      } else if ( realPressure > 657680000) {
        rawAltitude = 30000000 + ( 701120000 - realPressure ) *  0.115101289 ; 
      } else if ( realPressure > 616450000) {
        rawAltitude = 35000000 + ( 657680000 - realPressure ) *  0.121270919 ; 
      } else if ( realPressure > 577330000) {
        rawAltitude = 40000000 + ( 616450000 - realPressure ) *  0.127811861 ;
      } else
        rawAltitude = 45000000 + ( 577330000 - realPressure ) *  0.134843581 ;

TermPut(" rawAlt: ");
TermPut(rawAltitude);

// here the classical way to calculate Vspeed with high and low pass filter      
      if (altitude == 0) {
        altitudeLowPass = altitudeHighPass = altitude = rawAltitude ;
      }
      altitude += 0.04 * (rawAltitude - altitude) ;
//      varioData.altitudeAt20MsecAvailable = true ; // inform openxsens.ino that calculation of dTE can be performed
TermPut(" alt: ");
TermPut( altitude );

      altitudeLowPass += 0.085 * ( rawAltitude - altitudeLowPass) ;
      altitudeHighPass += 0.1  * ( rawAltitude - altitudeHighPass) ;
      climbRate2AltFloat = ((altitudeHighPass - altitudeLowPass )  * 5666.685 ) / 20000 ; 

      abs_deltaClimbRate =  abs(climbRate2AltFloat - climbRateFloat) ;

TermPut(" low: ");
TermPut(altitudeLowPass);
TermPut(" high: ");
TermPut(altitudeHighPass);
TermPut(" climb: ");
TermPut(climbRate2AltFloat);
TermPut(" delta: ");
TermPut(abs_deltaClimbRate);

#define SENSITIVITY_MIN 50
#define SENSITIVITY_MAX 300
#define SENSITIVITY_MIN_AT 100
#define SENSITIVITY_MAX_AT 1000

static int sensitivityMin = SENSITIVITY_MIN ; // set the min smoothing to the default value
static int32_t sensVal, climbRate;

//      if ( varioData.sensitivityPpm  > 0)
//        sensitivityMin =   varioData.sensitivityPpm ; 
      if ( (abs_deltaClimbRate <= SENSITIVITY_MIN_AT) || (sensitivityMin >= SENSITIVITY_MAX) ) {
         sensVal = sensitivityMin ;  
      } else if (abs_deltaClimbRate >= SENSITIVITY_MAX_AT)  {
         sensVal = SENSITIVITY_MAX ; 
      } else {
         sensVal = sensitivityMin + ( SENSITIVITY_MAX - sensitivityMin ) * (abs_deltaClimbRate - SENSITIVITY_MIN_AT) / (SENSITIVITY_MAX_AT - SENSITIVITY_MIN_AT) ;
      }
      climbRateFloat += sensVal * (climbRate2AltFloat - climbRateFloat)  * 0.001 ; // sensitivity is an integer and must be divided by 1000
      
#define VARIOHYSTERESIS 5
      if ( abs((int32_t)  climbRateFloat - climbRate) > VARIOHYSTERESIS ) {
          climbRate = (int32_t)  climbRateFloat  ;
      }    

TermPut(" climbRate: ");
TermPut(climbRate);

TermPut("\n");

  TVal.VarioDez2 = climbRate;

return;
*/











/*
void SensorAlt::Loop()
{
 static int64_t D1;                               // vom Sensor gelesener Druck

  // zyklisch Temp und Druck abfragen
  if( millis() < MS5611.Delay )
    return;

  // höchste Auflösung die der Sensor zu bieten hat für Variofunktion
  switch( MS5611.State )
    {
    case 0:                                       // read raw Pressure and Delay(10)
      WriteCmd( MS5611_CMD_CONV_D1 + MS5611_ULTRA_HIGH_RES );
      break;
    case 1:                                       // Verzögerung beendet, Pressure auslesen
      D1 = ReadReg( MS5611_CMD_ADC_READ, 3 );
                                                  // read raw Temperature and Delay(10)
      WriteCmd( MS5611_CMD_CONV_D2 + MS5611_ULTRA_HIGH_RES );
      break;
    case 2:                                       // Verzögerung beendet, Temp auslesen und Druck berechnen
      {
      int64_t D2   = ReadReg( MS5611_CMD_ADC_READ, 3 );

      int64_t dT   = D2 - ((uint64_t)MS5611.fc[4] << 8);
      int64_t OFF  = (((uint64_t)MS5611.fc[1]) << 16) + ( ((uint64_t)MS5611.fc[3] * dT) >> 7);
      int64_t SENS = (((uint64_t)MS5611.fc[0]) << 15) + ( ((uint64_t)MS5611.fc[2] * dT) >> 8);
      // ohne Temperatur Kompensation
      // Druck in Pascal (100 Pa = 1 hPa = 1 mbar)
      // 1 digit ~ 8cm
      // NN   = 101325
      // Home ~ 101014 (25m) > offiziell 40m
      uint32_t RawPressure = ((((((D1) * SENS) >> 21) - OFF) ) >> 15);
      // kleiner Smoothing Filter um Spikes zu analogRead() elemenieren
      // Anteil neuer Wert 25%, alter Wert 75%
      MS5611.RawPressure = ((float)RawPressure*0.25) + ((float)MS5611.RawPressure*0.75);

DbgPrintf("Press:%d D1:%d D2:%d\n", MS5611.RawPressure, D2, D1);
      if( MS5611.ReadCnt < (5000/30) )            // die ersten 5 Sek. (a' 3x10mS) zum Einpegeln
        {
        MS5611.ReadCnt++;
        MS5611.RefPressure = MS5611.RawPressure;  // akt. Druck als Referenz für relative Höhe ggü Start
        }
      else
        MS5611.ReadCnt = 0xff;
      MS5611.State = -1;
      break;
      }
    }

  MS5611.State++;
  MS5611.Delay = millis() + TMO_ULTRA_HIGH_RES;
}
*/
