neue Boards in Arduino IDE zV stellen
-------------------------------------

Verzeichnis "hardware":
dort sind alternative Boards hinterlegt, diese in das Arduino Installationsverzeichnis \Arduino\hardware kopieren. 
Danach erscheinen die Boards in der Werzeugleiste bei Board.


boards.txt:
\Arduino\hardware\arduino\avr\boards.txt editieren um bei zB existierenden Boards einen neuen Eintrag zuzuf�gen.
Beispiel in boards.txt f�r ProMini und optiboot


# Optiboot Arduino support
# http://optiboot.googlecode.com
# Peter Knight, 2010
# Bill Westfield, 2013 - now includes build.variant for 1.0.2 and later

##############################################################
Bootloader Optiboot
- schaltet die Watchdog aus
- stellt ca. 1K5 mehr an Codeplatz f�r die Appl. zur Verf�gung

Die boards.txt Datei ist eine modifizierte Original Datei der Arudino IDE,
es wird ein zus�tzlicher Eintrag f�r ProMini 5V / 16MhZ f�r OptiBoot erstellt (optiboot_atmega328.hex)
Update des Bootloaders mittels ISP Programmer, Bootloader brennen (bei meinem ISP funktioniert "Atmel STK500 development board")

Boards.txt nach \Arduino\hardware\arduino\avr\ kopieren oder besser die Eintr�ge erg�nzen
optiboot Verz. nach \Arduino\hardware\arduino\avr\bootloaders kopieren


