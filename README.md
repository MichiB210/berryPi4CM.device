# berrypi4cm.device

*[English version below](#english)*

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

### NTFS-Sticks: NTFS-3G aktualisieren
Für Sticks mit NTFS die aktuelle **NTFS-3G** (Fredrik Wikstrom, 53.5 vom 04.03.2026) von Aminet nehmen: [disk/misc/ntfs3g.m68k-amigaos](https://aminet.net/package/disk/misc/ntfs3g.m68k-amigaos).
- Die Datei aus `L/` (für Emu68 die `.020`-Fassung) als `L:NTFileSystem3G` kopieren.
- Braucht die `filesysbox.library` (ebenfalls auf Aminet).

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

---

<a name="english"></a>
# English

**USB 2.0 host controller driver for Poseidon on PiStorm32-lite with Raspberry Pi CM4 / Pi 4 under Emu68 1.0.x**

Version **1.0** · License: **GPL-2.0-or-later** · © 2026 MichiB210 · Based on [berrypi3ap.device](https://github.com/MichiB210/berryPi3Ap.device) 105.1 (same USB core)

## Tested
- **CaffeineOS_Storm 9317** on PiStorm32-lite with **CM4 Lite, 1 GB**, Emu68 **1.0.7**, Poseidon 4.5
- Stick, mouse, keyboard, USB 2.0 hub with mouse + stick + USB LAN adapter at the same time
- 82.9 MB copied from USB stick to SD: **7 s** (Pi 3A+ with berrypi3ap: about 20 s)

## Known issues
- **AmigaOS 3.2.x (created with the Emu68 Imager):** the driver runs, but something is wrong when copying / transferring, although the settings are the same. Under investigation – until then please use it on CaffeineOS_Storm only, or verify copied files.
- Under Emu68 1.0.x the USB interrupt does not reach the Amiga on CM4 / Pi 4 (Emu68 uses the GIC there). The driver detects this at start and polls mouse, keyboard and hubs in its own task. This works, but is a little less smooth than with interrupts.

## What for?
Under Emu68 1.0.x there is no other Poseidon USB driver for CM4 / Pi 4. From Emu68 1.1 on there is rondoval's `xhci.device` – then this driver is not needed.

It uses the **DWC2** USB block of the BCM2711:
- **CM4:** the USB 2.0 port of the carrier board (default, **without** `otg_mode=1` in config.txt)
- **Pi 4 Model B:** only the **USB-C port** (the USB-A ports are on the VL805/xHCI)

For the **Pi 3A+** please use **berrypi3ap.device** – this driver deliberately refuses to start there (error -7).

## Installation
1. Copy `berrypi4cm.device` to `DEVS:USBHardware/`.
2. Copy `B4Diag` to `C:` (optional).
3. In **Trident**, under *Hardware*, add `berrypi4cm.device`, unit 0, save, reboot.
4. `version DEVS:USBHardware/berrypi4cm.device` must show **1.0**.

### Important: hid.class (keyboard repeat)
Use the **hid.class from the Poseidon 4.5 package by individual Computers**: `PoseidonV4.lha` in the [RapidRoad wiki](https://wiki.icomp.de/wiki/RapidRoad).
- With another hid.class "V4.5", keyboard repeat did not work.
- `HIDclassV44.lha` (for OS 3.1.4/3.2) made **all** keyboards stop under CaffeineOS after the first key press, including the internal one.

### NTFS sticks: update NTFS-3G
For NTFS sticks use the current **NTFS-3G** (Fredrik Wikstrom, 53.5 from 2026-03-04) from Aminet: [disk/misc/ntfs3g.m68k-amigaos](https://aminet.net/package/disk/misc/ntfs3g.m68k-amigaos).
- Copy the file from `L/` (for Emu68 the `.020` build) to `L:NTFileSystem3G`.
- Requires `filesysbox.library` (also on Aminet).

## Error codes at start
- **-7:** no CM4 / Pi 4 detected → use berrypi3ap.device
- **-8:** DMA buffer above 1 GB (only possible on boards with 2 GB or more) → please report

## Diagnostics
`B4Diag` shows statistics like B3Diag, plus board revision, SoC, RAM, the result of the interrupt self-test and DMA requests refused above 1 GB.
`B4Diag CLEAR` resets the counters (recommended before each measurement).

## Building
```
GCC=/path/to/m68k-amigaos-gcc sh build.sh
```
bebbo toolchain (tested with m68k-amigaos-gcc 16.2). Output: `berrypi4cm.device` and `B4Diag`.
