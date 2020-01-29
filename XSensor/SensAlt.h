/*************************************************************
  Höhen Sensor
  
  Sensor: MS5611 / BMP180 konfigurierbar
  angeschlossen über I2C, A4=SDA A5=SCL
  ------------------------------------------------------------
  
  Copyright (C) 2017 Thomas Lehmann
  
  Version history:
  1.00   19.06.2017  created

***************************************************************/

#ifndef SENS_ALT_H
#define SENS_ALT_H


#include "Def.h"


#if _SENS_ALT_ == 1



#define MS5611_ADDRESS                (0x77)

#define MS5611_CMD_ADC_READ           (0x00)
#define MS5611_CMD_RESET              (0x1E)
#define MS5611_CMD_CONV_D1            (0x40)
#define MS5611_CMD_CONV_D2            (0x50)
#define MS5611_CMD_READ_PROM          (0xA2)

                                       // Conversion time
#define MS5611_ULTRA_HIGH_RES   0x08   // 7.40 - 9.04 mS
#define MS5611_HIGH_RES         0x06   // 3.72 - 4.54 mS
#define MS5611_STANDARD         0x04   // 1.88 - 2.28 mS
#define MS5611_LOW_POWER        0x02   // 0.95 - 1.17 mS
#define MS5611_ULTRA_LOW_POWER  0x00   // 0.48 - 0.60 mS

#define TMO_ULTRA_HIGH_RES      10     // 10mS Delay vor ADC Abfrage



class SensorAlt
{
public:
	//SensorAlt();
	
	bool Init( uint8_t Id, boolean Vario );
  void Loop();
  void Tim10mS();

  // Fehler / Analyse
  bool Failure;                             // TRUE == Fehler, Druck Sensor liefert keine Daten

  // Telemetriewerte
  struct
    {
    #if _SENS_ALT_TEMP_ == 1                // Temperatur Anzeige vom MS5611
    int16_t  TempDez1;                      // Temperatur 2412 = 24,1°
    #endif
    int16_t  AltitudeRelativ;               // relative Höhe ggü Einschaltzeitpunkt, abgerundet auf 1 Meter Genauigkeit, Telemetriewert
    uint32_t AltitudeTotDez2;               // Gesamt Höhe
    int16_t  VarioDez2;                     // Vario m/s
    }TVal;


private:

  int16_t LastAltTot;


  // Drucksensor MS5611, Höhe 123,45m
  float   getAltitude( int32_t pressure, int32_t sealevelPressure = 101325 );
  void    WriteCmd( uint8_t Cmd );
  int32_t ReadReg( uint8_t Cmd, int ByteCnt );
  struct
    {
    uint32_t Delay;
    uint8_t  State = 0;
    uint8_t  ReadCnt = 0;
    uint16_t fc[6];                         // Kompensierung
      #define _C1   0                       // fc[C1]
      #define _C2   1
      #define _C3   2
      #define _C4   3
      #define _C5   4
      #define _C6   5
    int32_t  RawPressure;                   // akt. berechneter Druck
    int32_t  RefPressure;                   // Referenz Druck beim Einschalten
    //float  RefAltitude;                   // Referenz Höhe beim Einschalten
    }MS5611;
  

  #if _SENS_ALT_VARIO_  == 1
  #define VARIO_SMOOTHING_FILTER  80        // default Wert für Vario Filter in %
  struct
    {
    bool Activ;
    long LastFilter;
    long LastAlt;
    long LastTime;
    }Vario;
  #endif

  uint8_t TmoRefresh;
    #define ALT_TMO_REFRESH  (200/TIM_BASE) // alle 200mS eine Messung vornehmen
};

#endif

#endif
