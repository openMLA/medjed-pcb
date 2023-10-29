## Medjed Stage

The controller board for the Medjed stage is built around a Teensy 4.1, Snapmaker linear axes and external RLC2IC encoders. The signal of the encoders is processed and converted to step/dir signals for the snapmaker axes by Trinamic `TLC4631`

![](board-layout-preview-3D.JPG)

### I2C Connection

A STEMMA-QT connector is present on the board for any I2C peripherals. A planned connection would be a UV sensor to be fitted on the stage that will serve as stage calibration. 

### CAN Bus

The Snapmaker linear axes use a CAN bus to talk to the MCU inside the axes (reporting on endstop status and such). It would be quite inefficient to put a CAN transceiver for each of the axes, but running them as part of a single CAN bus means we need to go slightly outside of recommended CAN bus parameters. 

The cables for the linear axes are 40cm, and I assume that the transmission line is terminated with 120Ohm impedance. Running three terminated nodes may prove to be too much for the transceiver to power. In addition, the 40cm cable length is a little over the recommended max length for CAN nodes. I think both concerns are not fundamentally unworkable. The bus speed may need to be reduced to compensate, but this shouldn't really be an issue for the targeted application.

> Future designs should probably expose a CAN bus for other peripherals to be connected; or to handle communication with devices on the Z-axi.

### Safety

The Emergency Stop (`ESTOP`) signal is controllable from the teensy, but features manual trigger in the form of a simple PCB switch and a terminal for external switches. 

> Future revisions will need a proper EMO switch that kills power to all systems (axes and optics)

### Power considerations

The snapmaker linear axes use 24V supply. The snapmaker power supply comes with plenty of overhead in terms of power (as it need to be able to run a 3D printer heated bed) so that is not a concern. 

We also need 5V and 3.3V supply on the board.

5V:

* Teensy 4.1: 100mA
* [3X] RLC2IC encoders: <30mA (no termination resistors) or <130mA with 120ohm termination.
* [3X] AM26LS32A: max 70mA, typ 50mA

3.3V:

* TMC424: 1.5mA
* TMC429: 10mA (running in 3.3V CMOS mode)
* (for future revision: IMXRT60: 53mA)

1.5V:

* TMC424: 10.5mA