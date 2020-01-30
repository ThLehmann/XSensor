/*************************************************************
  Temperatur Sensor NTC Widerstand

  NTC MF58 3950 5K 5% Tempbereich -45° bis 300° oder 250° ?
  
  Quelle: NTC temperature calculation by "jurs", German Arduino forum
  http://forum.arduino.cc/index.php?topic=155733.msg1170722#msg1170722
  ------------------------------------------------------------

  Spannungsteiler:

           5V 
           |
      Vorwiderstand
           |
  PinAx ---|
           |
          NTC
           |
          GND

GND-> NTC 1k :10K = 931 =  25°
Vcc-> NTC 1k :10K =  91 = 116°
  
  ***************************************************************************************
  ***************************************************************************************

  Temperatur Sensor MLX90614, Infrarot / Berührungslos

  Tempbereich -45° bis 300° oder 250° ?
  
  Infos zum Sensor:
  https://chrisramsay.co.uk/posts/2017/09/arduino-and-multiple-mlx90614-sensors-take-two/
  ------------------------------------------------------------

  Copyright (C) 2017 Thomas Lehmann
  
  Version history:
  1.00   05.12.2017  created

***************************************************************/

#include <Wire.h>

#include "SensTemp.h"


#if _SENS_TEMP_ == 1


// NTC Widerstand
#define ABSZERO           273.15

#if ADC_GND == 1
#define ADC_INV           0x3ff                 // Board mit A0-A3 mit 1K Vorwiderstand gegen GND bei neuen Board, somit auch für Spg./Strom Messung geeignet
#else
#define ADC_INV           0                     // gegen Vcc bei alten Board, dort keine Invertierung notwendig
#endif



// Klasse indizierbar, Aufruf je Tempsensor
// ========================================
bool SensorTemp::Init( uint8_t Id, uint8_t SensIdx )
{
  SensTyp = GData.EEprom.Data.CfgSensTypTemp[SensIdx];
  if( SensTyp == STYP_TEMP_NTC )
    {                                           // NTC analog Pin fortlaufend
    AnaFiltNTC = new AnalogFilter();            // gefilterter Analog Port
    AnaFiltNTC->Open( PIN_TEMP_NTC1 + SensIdx );  // gefilterter Analog Port
    }
  else       // IR Sensor am I2C Bus
    {
    IRAddr = 0x5A + SensIdx;                    // I2C Startadresse 1. Sensor, jeder weitere Sensor dahinter
    // Sensor vorhanden / ansprechbar ?
    Wire.begin();                               // I2C als Master starten
    Wire.beginTransmission( IRAddr );           // Sensor andingsen
    if( Wire.endTransmission() )                // 0 == korrekte Antwort
      {
      DbgPrintf("%02x > Temp IR not found\n\n", IRAddr );
      return( false );                          // Sensor nicht angeschlossen
      }
    }
  
  TmoRefresh = TEMP_TMO_REFRESH + Id;           // Zeit bis zur 1. Messung
  return( true );
}



// ==========
// 10mS Timer
// ==========
void SensorTemp::Tim10mS()
{
  if( TmoRefresh == 0 )                         // Sensor nicht angeschlossen/aktiviert
    return;

  // Refresh Tempmessung ?
  if( --TmoRefresh == 0 )
    {
    TmoRefresh = TEMP_TMO_REFRESH;             // Zeit bis zur nächsten Messung

    if( SensTyp == STYP_TEMP_NTC )
      {
      // Vorwiderstand (1K) zu NTC (10K) im Verhältnis 1:10 für eine möglichst hohe Auflösung des ADC
      #define _T0     26.0                      // Nenntemperatur des NTC-Widerstands in °C
      #define _R0  10000.0                      // Nennwiderstand des NTC-Sensors in Ohm
      #define _B    3950.0                      // Materialkonstante B
      #define _RV   1000.0                      // Vorwiderstand in Ohm  
      // Messungen bei 23°
      // NTC 5K
      // #define _RV   5100.0                   // 1:1  ADC ~530
      // #define _RV   1000.0                   // 1:5  ADC ~850
      // NTC 10K
      // #define _RV   5100.0                   // 1:2  ADC ~710
      // #define _RV   1000.0                   // 1:10 ADC ~940
      #define _T0_ABS  (ABSZERO + _T0)          // Nenntemp Celsius in absoluter Temperatur
  
      float aVal = (AnaFiltNTC->Read() ^ ADC_INV);  // A/D Wandler lesen 0-1023 = 0-5V
//      printFloat("aVal: ", aVal);
//      DbgPrintf("\n");
        
      if( aVal >= MAX_ANALOG_READ - 10 )        // kein NTC angeschlossen ?
        {
        TVal.Temp = 0;
        Failure = true;
        }
      else
        {
        aVal /= MAX_ANALOG_READ;                // Spannungsverhältnis "Spannung am NTC zu Betriebsspannung"
    
        float RN = _RV*aVal / (1.0-aVal);       // aktueller Widerstand des NTC
        TVal.Temp = _T0_ABS * _B / (_B + _T0_ABS * log(RN / _R0)) - ABSZERO;
        if( TVal.Temp > 500 )                   // Minusgrade
          TVal.Temp = 0;
        }
      }
    // --------------------
    // IR Sensor am I2C Bus
    // --------------------
    else
      {
      // Objekt Temperatur in °C lesen
      // -----------------------------
      #define TEMP_AMB  0x06                    // Umgebungs Temperatur lesen
      #define TEMP_OBJ  0x07                    // Objekt Temperatur lesen
      int data_low, data_high;  //, pec;
      double tempData;
  
      Wire.beginTransmission( IRAddr );         // Begin transmission with device
      Wire.write( TEMP_OBJ );                   // Send register address to read - 0x06 for ambient, 0x07 for object
      Failure = Wire.endTransmission( 0 );      // The 'mysterious' repeated start
      Wire.requestFrom( IRAddr, (uint8_t)3 );   // Request three bytes from device
        
      uint32_t WaitMax = millis() + 10;         // falls mal was verloren geht, max 10mS warten
      while( Wire.available() < 3 )             // Wait for three bytes to become available
        if( millis() > WaitMax )
          {
          Wire.endTransmission();               // Terminate transmission
          Failure = true;
          return;                               // beim nächsten Mal haben wir vllt mehr Glück
          }
      data_low = Wire.read();                   // Read first byte
      data_high = Wire.read();                  // Read second byte
      //pec = Wire.read();                        // Read checksum 
      Wire.read();                        // Read checksum 
         
      // This masks off the error bit of the high byte, then moves it left 8 bits and adds the low byte.
      // Taken from bildr forum on MLX90614
      tempData = (double) (((data_high & 0x007F) << 8) + data_low);
      // Multiply by resolution and account for error to convert to Kelvin
      // MLX90614 has a resolution of .02
      TVal.Temp = ((tempData * 0.02) - 0.01)  - 273.15;
      }


    // Telemetrieanzeige stabil halten, Rauschunterdrückung
    // kleine Änderungen ausfiltern, Werte stabilisieren/glätten
    #define TRAEGHEIT_IN 0.3                     // je kleiner desto träger
    TVal.Temp = ((1.0-TRAEGHEIT_IN) * FiltTemp) + (TRAEGHEIT_IN*TVal.Temp);
    FiltTemp = TVal.Temp;
    //DbgPrintf(" > Temp:%d C\n-----\n\n", TVal.Temp);
    }
}


#endif




/*

// analog input pin
#define TEMPERATURE_PIN     A6

// Thermistor nominal resistance at 25ÂºC
#define THERMISTORNOMINAL   10000

// temp. for nominal resistance (almost always 25ÂºC)
#define TEMPERATURENOMINAL  25

// thermistor's beta coefficient
#define BCOEFFICIENT        3950

// Self-heat compensation (K)
#define SELF_HEAT           1.2

// Value of the series resistor
#define SERIESRESISTOR      10000


                    vcc
                     |
                     |
                    | |
                    | |  Resistor
                    | |
                     |
  analog Pin  <------+
                     |
                    | |
                    | |  NTC
                    | |
                     |
                     |
                    GND


    if(enableExtTemp){
      // convert the value to resistance
      float aIn = 1023.0 / analogRead(TEMPERATURE_PIN) - 1.0;
      aIn = SERIESRESISTOR / aIn;

      // convert resistance to temperature
      float steinhart;
      steinhart = aIn / THERMISTORNOMINAL;                // (R/Ro)
      steinhart = log(steinhart);                         // ln(R/Ro)
      steinhart /= BCOEFFICIENT;                          // 1/B * ln(R/Ro)
      steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15);   // + (1/To)
      steinhart = 1.0 / steinhart;                        // Invert
      steinhart -= 273.15 - SELF_HEAT;                    // convert to °C and self-heat compensation 
      
      #ifdef UNIT_US
        // EU to US conversions
        steinhart = steinhart * 1.8 + 320;
      #endif
      
      jetiEx.SetSensorValue( ID_EXT_TEMP, steinhart*10);
    }
*/






/*
// Ermittlung der Temperatur mittels NTC-Widerstand
// Version der Funktion bei gegebener Materialkonstante B
// Erklärung der Parameter:
// T0           : Nenntemperatur des NTC-Widerstands in °C
// R0           : Nennwiderstand des NTC-Sensors in Ohm
// B            : Materialkonstante B
// Vorwiderstand: Vorwiderstand in Ohm  
// VA_VB        : Spannungsverhältnis "Spannung am NTC zu Betriebsspannung"
// Rückgabewert : Temperatur
float SensorTemp::TemperatureNTCB( float T0, float R0, float B, float RV, float VA_VB )
{
  T0 += ABSZERO;                    // umwandeln Celsius in absolute Temperatur
  float RN = RV*VA_VB / (1-VA_VB);  // aktueller Widerstand des NTC
  return( T0 * B / (B + T0 * log(RN / R0)) - ABSZERO );
}
*/





/*

// NTC temperature calculation by "jurs" for German Arduino forum
#define ABSZERO 273.15
#define MAXANALOGREAD 1023.0

float temperature_NTCB(float T0, float R0, float B, float RV, float VA_VB)
// Ermittlung der Temperatur mittels NTC-Widerstand
// Version der Funktion bei gegebener Materialkonstante B
// Erklärung der Parameter:
// T0           : Nenntemperatur des NTC-Widerstands in °C
// R0           : Nennwiderstand des NTC-Sensors in Ohm
// B            : Materialkonstante B
// Vorwiderstand: Vorwiderstand in Ohm  
// VA_VB        : Spannungsverhältnis "Spannung am NTC zu Betriebsspannung"
// Rückgabewert : Temperatur
{
 T0+=ABSZERO;  // umwandeln Celsius in absolute Temperatur
 float RN=RV*VA_VB / (1-VA_VB); // aktueller Widerstand des NTC
 return T0 * B / (B + T0 * log(RN / R0))-ABSZERO;
}


float temperature_NTC(float T0, float R0, float T1, float R1, float RV, float VA_VB)
// Ermittlung der Temperatur mittels NTC-Widerstand
// Version der Funktion bei unbekannter Materialkonstante B
// Erklärung der Parameter:
// T0           : Nenntemperatur des NTC-Widerstands in °C
// R0           : Nennwiderstand des NTC-Sensors in Ohm
// T1           : erhöhte Temperatur des NTC-Widerstands in °C
// R1           : Widerstand des NTC-Sensors bei erhöhter Temperatur in Ohm
// Vorwiderstand: Vorwiderstand in Ohm  
// VA_VB        : Spannungsverhältnis "Spannung am NTC zu Betriebsspannung"
// Rückgabewert : Temperatur
{
 T0+=ABSZERO;  // umwandeln Celsius in absolute Temperatur
 T1+=ABSZERO;  // umwandeln Celsius in absolute Temperatur
 float B= (T0 * T1)/ (T1-T0) * log(R0/R1); // Materialkonstante B
 float RN=RV*VA_VB / (1-VA_VB); // aktueller Widerstand des NTC
 return T0 * B / (B + T0 * log(RN / R0))-ABSZERO;
}


void setup()
{
 Serial.begin(9600);
}


#define ANALOGPIN A2

void loop()
{
 float T0=60;    // Nenntemperatur des NTC-Widerstands in °C
 float R0=221.2; // Nennwiderstand des NTC-Sensors in Ohm
 float T1=120;   // erhöhte Temperatur des NTC-Widerstands in °C
 float R1=36.5;  // Widerstand des NTC-Sensors bei erhöhter Temperatur in Ohm
 float Vorwiderstand=100; // Vorwiderstand in Ohm  
 float temp;    
 int aValue=analogRead(ANALOGPIN);
 
 // Berechnen bei unbekannter Materialkonstante
 temp=temperature_NTC(T0, R0, T1, R1, Vorwiderstand, aValue/MAXANALOGREAD);
 TermPutTxt("NTC : ");TermPut(temp);TermPutTxt(" C\n");
 // Berechnen bei bekannter Materialkonstante B=3933;
 temp=temperature_NTCB(T0, R0, 3933, Vorwiderstand, aValue/MAXANALOGREAD);
 TermPutTxt("NTCB: ");TermPut(temp);TermPutTxt(" C\n");
 delay(1000);
}
*/
