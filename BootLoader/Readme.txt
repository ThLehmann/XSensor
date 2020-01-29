neue Boards in Arduino IDE zV stellen
-------------------------------------

Verzeichnis "hardware":
dort sind alternative Boards hinterlegt, diese in das Arduino Installationsverzeichnis \Arduino\hardware kopieren. 
Danach erscheinen die Boards in der Werzeugleiste bei Board.


boards.txt:
\Arduino\hardware\arduino\avr\boards.txt editieren um bei zB existierenden Boards einen neuen Eintrag zuzufügen.
Beispiel in boards.txt für ProMini und optiboot


# Optiboot Arduino support
# http://optiboot.googlecode.com
# Peter Knight, 2010
# Bill Westfield, 2013 - now includes build.variant for 1.0.2 and later

##############################################################
Bootloader Optiboot
- schaltet die Watchdog aus
- stellt ca. 1K5 mehr an Codeplatz für die Appl. zur Verfügung

Die boards.txt Datei ist eine modifizierte Original Datei der Arudino IDE,
es wird ein zusätzlicher Eintrag für ProMini 5V / 16MhZ für OptiBoot erstellt (optiboot_atmega328.hex)
Update des Bootloaders mittels ISP Programmer, Bootloader brennen (bei meinem ISP funktioniert "Atmel STK500 development board")

Boards.txt nach \Arduino\hardware\arduino\avr\ kopieren oder besser die Einträge ergänzen
optiboot Verz. nach \Arduino\hardware\arduino\avr\bootloaders kopieren


