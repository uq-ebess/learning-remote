# learning-remote
The source code for EBESS' learning remote kit. It's definitely a bit rough in places, but feel free to go through it to see if you get anything out of it.

If you need to repogram your remote control for some reason, simply download remoteControl.hex and remoteControl.eep. First, you will need to disable the CLKDIV8 fuse so that your Atmega328 will have an 8MHz clock. You will program the flash with remoteControl.hex, and the EEPROM with remoteControl.eep
