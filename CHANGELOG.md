# Changelog – berrypi4cm.device

## 1.0 (04.11.2026)
First version, based on berrypi3ap.device 105.1 (same USB core).
- New name `berrypi4cm.device`, own version numbers; diagnostics tool `B4Diag`.
- Board check via the firmware mailbox (board revision): refuses to start on a Pi 3 or older (error -7, use berrypi3ap.device there).
- Interrupt self-test at start: system timer C3 armed 1 ms ahead, 20 ms wait. On Emu68 1.0.x with CM4/Pi 4 no interrupt arrives (Emu68 uses the GIC there), so the driver switches the interrupt path off and the unit task polls mouse, keyboard and hubs itself, woken by timer.device. In 105.1 the periodic engine stayed on although it never ran, so mouse and keyboard never got polled.
- The Pi 3 legacy interrupt controller registers are never written on BCM2711 (different register layout there).
- DMA memory check: the DWC2 on BCM2711 only reaches the first GB. The driver's DMA buffer is checked at start (error -8 if above 1 GB) and every transfer refuses a buffer above 1 GB (counter in B4Diag) instead of writing to the wrong place. Only matters on 2 GB+ boards.
