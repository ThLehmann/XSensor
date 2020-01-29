# XSensor

Multi Telemetriesensor für Jeti, auf Basis von Arduino

#### Highlights
- alle Sensoren im Code vorhanden und durch Jetibox aktivier und konfigurierbar
- einfachen "in den Code nehmen" durch Änderung eines einzelnen Defines (def.h)
- Leiterkarte zum einfachen Anschluss der Sensoren
- Leiterkarte MUL-6S zum Anschluss eines 6S Akkus mit Einzelzellenüberwachung


#### Sensoren
- Drehzahl
- Durchfluss, Benzin/Kerosin ...
- Temperatur 4x NTC / 7x IR
- Höhe
- Vario
- Pitot Rohr, Geschwindigkeit
- GPS NEMA Format
- PWM Messung
- 6x Spannung
- 3x MUI (Strom, Leistung, Kapazität)
- G-Force, vorbereitet

#### Hardware
- Motherboard: Arudino Mini Pro 5V, CPU 328P
- Drehzahl   : Hallsensor zB TLE4905L, Conrad Bestellnr: 153751-62
- Durchfluss : Turbine: BioTech FCH-m-POM-LC / Durchflussmenge 50mL - 3000mL
- Durchfluss : Benzin : BioTech FCH-m-PP-LC / Durchflussmenge 15mL - 800mL
- Temperatur : NTC MF58 3950 5K 5% / Tempbereich -45° bis 300°
- Temperatur : MLX90614 per I2C, Infrarot / Berührungslos
- Drucksensor: MS5611
- Pitot Rohr : MPXV7002DP siehe RC-Thought
- GPS        : NEO 6M
- Strom      : diverse ACSxx, siehe Code oder JetiBox
- MUL6S      : Einzelzellenspannung
- PWM        : PWM Impulsmessung
- G-Force    : ADXL345, not implemented yet

#### PCB
- Adapterboard für steckbare 
- MUL-6S für Akku Einzelzellenüberwachung


#### Software
Bootloader Optiboot bringt 1,5kB zusätzlichen Speicherplatz
Arduino IDE Version 1.8.9


#### Drehzahl (1x)
- Anzahl Magnete / Auslösungen je Umdrehung
- Genauigkeit / Abrunden des Drehzahlwertes
- Anzeige der Auslösungen auf JB zum Sensor Test

**++Sensorwerte:++**
- aktuelle  Drehzahl
- Anzahl Umdrehungen gesamt
- Betriebszeit


#### Durchfluss (1x)
- Tankvolumen
- Rücksetzen
-- automatisch nach jedem Sensor Neustart (typ. Turbine)
-- manuell per Taster (typ. Schleppmodell)
- Anzahl Impulse je Liter
-- manuell änderbar
-- Kalibrierbar
- Anzeige der Auslösungen auf JB zum Sensor Test

**++Sensorwerte:++**
- verbleibende Tankmenge in mL
- verbrauchte Menge in mL
- akt. Durchfluss in ml/Min
- gesamt verbrauchte Menge


#### Temperatur (4xNTC / 7x IR)
- NTC (Wärmewiderstand) an Analogport
- IR (Infrarot) per I2C

**++Sensorwerte:++**
- Temperatur 1-7


#### Höhe / Vario (1x)
- Vario Sensibilität (Filter) und Totzone

**++Sensorwerte:++**
- relative Höhe bezogen auf Starthöhe
- gesamt Höhe, Summe aller gemachten Höhenmeter


#### GPS (1x)

**++Sensorwerte:++**
- Anzahl aktiver Satelliten
- Position, Längen und Breitengrad
- relative Höhe bezogen auf Starthöhe
- gesamt Höhe, Summe aller gemachten Höhenmeter


