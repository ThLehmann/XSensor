# XSensor

Multi Telemetriesensor für Jeti, auf Basis von Arduino

#### Highlights
- alle Sensoren im Code vorhanden und durch Jetibox aktivier und konfigurierbar
- einfachen "in den Code nehmen" durch Änderung eines einzelnen Defines (def.h)
- Leiterkarte zum einfachen Anschluss der Sensoren
- Leiterkarte MUL-6S zum Anschluss eines 6S Akkus mit Einzelzellenüberwachung


#### Sensoren
- Drehzahl
- Durchfluss, Benzin/Kerosin
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
- Adapterboard für steckbare Sensoren, unterstützt drei verschiedene Pro Mini Boards
 - unten mit 5 Pinnen
 - unten mit 3 Pinnen, rechts 2 Pinne
 - unten keine Pinne, rechts 2x2 Pinne 
- MUL-6S für Akku Einzelzellenüberwachung


#### Software
- Bootloader Optiboot bringt 1,5kB zusätzlichen Speicherplatz
- Arduino IDE Version 1.8.9


#### besondere Konfigurationseigenschaften
- Sensor Nr., sollten mehrere XSensor in einem Modell eingesetzt werden
- Watchdog, Überwachung der ordnungsgemäßen Funktion des Sensors (!! prüfen ob Bootloader das ordnungsgemäß unterstützt)
- Referenzspannung für ADC Mesuungen (Spg/Strom), wichtig um genaue Messergebnisse zu erhalten. Mit Multimeter die Vcc Spg. messen
- Kalibrierung des Stromsensors, im Ruhebetrieb sollte kein Strom fliessen
- Rücksetzen aller Werte auf Auslieferzustand !!! vorher überlegen ob sinnvoll !!!
- Debug, Anzeige des verbleibenden Speichers, dieser sollte größer 100 Byte sein, ansonsten droht Fehlfunktion

#### Konfigurationskonzept
Änderungen werden NICHT direkt gespeichert, erst nachdem wieder ganz nach oben (Pfeil hoch) gegangen ist erfolgt eine Abfrage ob die Änderungene gespeichert oder verworfen werden sollen.
Mit Taste rechts links wird zwichen den Hauptmenues gewechselt, mit Taste runter in das entsprechend ausgeählte.
Um etwas zu aktivieren ist Taste links und rechts gleichzeitig zu betätigen. Leider werden Tastendrücke vom Sender nicht immer korrekt übermittelt, dies ist zu beachten. Mit etwas Gewöhnung sieht man aber schnell ob das Gewünschte ausgeführt wurde.

#### LED Anzeige
Der Sensor Status wird mit vier unterschiedlichen Blinkrhytmen angezeigt.
LED ein / aus
- 100mS / 1500mS Ruhebetrieb
- 1400mS / 200mS kein Sensor aktiv
- 100mS / 100mS Sensor Initialisierungs Fehler
- 800mS / 800mS Sensor Fehler während Betrieb

Eine Anzeige beim entspr. Sensor erfolgt in der Jetibox um zu ermitteln welcher Sensor betroffen ist
- I = Init
- F = Laufzeit


#### Compilieren
Sollte der Codespeicher nicht reichen, weil zB ein anderer Bootloader als Optiboot eingesetzt wird, können nicht gewünschte Eigenschaften ausgeblendet werden. In der def.h sind entsprechende defines zu finden (_SENS_XX_). Den entsprechenden Sensor einfach durch '1' einblenden oder '0' ausblenden.
Die fest zugeordneten Ports sind ab Zeile 160 zu sehen.


# Sensoren

### Drehzahl (1x)
- Anzahl Magnete / Auslösungen je Umdrehung
- Genauigkeit / Abrunden des Drehzahlwertes
- Anzeige der Auslösungen auf JB zum Sensor Test

Sensorwerte:
- aktuelle  Drehzahl
- Anzahl Umdrehungen gesamt
- Betriebszeit


### Durchfluss (1x)
- Tankvolumen
- Rücksetzen
-- automatisch nach jedem Sensor Neustart (typ. Turbine)
-- manuell per Taster (typ. Schleppmodell)
- Anzahl Impulse je Liter
-- manuell änderbar
-- Kalibrierbar
- Anzeige der Auslösungen auf JB zum Sensor Test

Sensorwerte:
- verbleibende Tankmenge in mL
- verbrauchte Menge in mL
- akt. Durchfluss in ml/Min
- gesamt verbrauchte Menge


### Temperatur (4xNTC / 7x IR)
- NTC (Wärmewiderstand) an Analogport
- IR (Infrarot) per I2C

Sensorwerte:
- Temperatur 1-7


### Höhe / Vario (1x)
- Vario Sensibilität (Filter) und Totzone

Sensorwerte:
- relative Höhe bezogen auf Starthöhe
- gesamt Höhe, Summe aller gemachten Höhenmeter
- Temperatur
- Vario


### GPS (1x)

Sensorwerte:
- Anzahl aktiver Satelliten
- Position, Längen und Breitengrad
- relative Höhe bezogen auf Starthöhe
- Geschwindigkeit
- max. Geschwindigkeit
- akt. Kurs / Richtung in Grad 0-359
- Richtung in Grad zum Modell, gemessen vom Startpunkt
- Enfernung zum Modell, gemessen vom Startpunkt
- zurückgelegte Strecke, Trip und gesamt

### Pitot Rohr (1x)

Sensorwerte:
- Geschwindigkeit


### Spannung / MUL (6x)
- analog Port
- Widerstand gegen GND
- Widerstand gegen Vcc

Sensorwerte:
- Spannung


### Spannung-Strom-Leistung / MUI (3x)
- diverse ACS konfigurierbar, uni als auch bidirektionale Typen
- Kalibrierbar
- Rückstellen der verbrauchten Kapazität
-- automatisch nach jedem Sensor Neustart
-- automatisch bei n% höherer Akkuspannung nach neustart (Akku geladen)
-- manuell durch Taster oder Jetibox
- analog Port
- Widerstand gegen GND
- Widerstand gegen Vcc

Sensorwerte:
- Strom
- Spannung
- Leistung
- entnommene Kapazität


### PWM Messung (1x)
Sensorwerte:
- Impulsdauer eines PWM Signals


### Jetibox
Summenwerte sind per Jetibox rücksetzbar
- Anzahl Umdrehungen
- Motor Betriebszeit
- Tankvolumen
- gesamt Durchfluss
- entnommene Akku Kapazität
- max. Geschwindigkeit
- gesamt zurückgelegte Strecke



