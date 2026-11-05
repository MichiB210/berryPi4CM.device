# berrypi4cm.device

**USB-2.0-Host-Treiber für Poseidon auf PiStorm32-lite mit Raspberry Pi CM4 / Pi 4 unter Emu68 1.0.x**

Version **1.0** · Lizenz: **GPL-2.0-or-later** · © 2026 MichiB210
Basis: [berrypi3ap.device](https://github.com/MichiB210/berryPi3Ap.device) 105.1 (gleicher USB-Kern)

## Getestet
- **CaffeineOS_Storm 9317** auf PiStorm32-lite mit **CM4 Lite, 1 GB**, Emu68 **1.0.7**, Poseidon 4.5
- Stick, Maus, Tastatur, USB-2.0-Hub mit Maus + Stick + USB-LAN-Adapter gleichzeitig
- 82,9 MB vom USB-Stick auf SD kopiert: **7 s** (zum Vergleich Pi 3A+ mit berrypi3ap: ca. 20 s)

## Bekannte Probleme
- **AmigaOS 3.2.x (mit dem Emu68 Imager erstellt):** Der Treiber läuft, beim Kopieren / bei der Übertragung stimmt aber etwas nicht, obwohl die Einstellungen gleich sind. Wird untersucht – bis dahin bitte nur unter CaffeineOS_Storm verwenden bzw. kopierte Dateien prüfen.
- Unter Emu68 1.0.x kommt auf CM4 / Pi 4 der USB-Interrupt nicht beim Amiga an (Emu68 benutzt dort den GIC). Der Treiber erkennt das beim Start und fragt Maus, Tastatur und Hubs im eigenen Task ab. Das funktioniert, ist aber etwas weniger flüssig als mit Interrupt.

## Wofür?
Unter Emu68 1.0.x gibt es für CM4 / Pi 4 sonst keinen USB-Treiber für Poseidon. Ab Emu68 1.1 gibt es rondovals `xhci.device` – dann wird dieser Treiber nicht gebraucht.

Benutzt wird der **DWC2**-USB-Baustein des BCM2711:
- **CM4:** USB-2.0-Anschluss der Trägerplatine (Standard, **ohne** `otg_mode=1` in der config.txt)
- **Pi 4 Model B:** nur die **USB-C-Buchse** (die USB-A-Buchsen hängen am VL805/xHCI)

Für den **Pi 3A+** bitte **berrypi3ap.device** nehmen – dieser Treiber startet dort absichtlich nicht (Fehler -7).

## Installation
1. `berrypi4cm.device` nach `DEVS:USBHardware/` kopieren.
2. `B4Diag` nach `C:` kopieren (optional).
3. In **Trident** unter *Hardware* `berrypi4cm.device`, Unit 0, eintragen, speichern, neu starten.
4. `version DEVS:USBHardware/berrypi4cm.device` muss **1.0** zeigen.

### Wichtig: hid.class (Tastatur-Wiederholung)
Die **hid.class aus dem Poseidon-4.5-Paket von individual Computers** verwenden: `PoseidonV4.lha` im [RapidRoad-Wiki](https://wiki.icomp.de/wiki/RapidRoad).
- Mit einer anderen hid.class „V4.5“ funktionierte die Tastatur-Wiederholung nicht.
- Die `HIDclassV44.lha` (für OS 3.1.4/3.2) ließ unter CaffeineOS nach dem ersten Tastendruck **alle** Tastaturen ausfallen, auch die interne.

## Fehlercodes beim Start
- **-7:** kein CM4 / Pi 4 erkannt → berrypi3ap.device nehmen
- **-8:** DMA-Puffer über 1 GB (nur bei Boards mit 2 GB oder mehr möglich) → bitte melden

## Diagnose
`B4Diag` zeigt Statistiken wie B3Diag, dazu Board-Revision, SoC, RAM, Ergebnis des Interrupt-Selbsttests und abgewiesene DMA-Zugriffe über 1 GB.
`B4Diag CLEAR` setzt die Zähler zurück (vor jeder Messung empfohlen).

## Bauen
```
GCC=/pfad/zu/m68k-amigaos-gcc sh build.sh
```
bebbo-Toolchain (getestet mit m68k-amigaos-gcc 16.2). Ergebnis: `berrypi4cm.device` und `B4Diag`.
