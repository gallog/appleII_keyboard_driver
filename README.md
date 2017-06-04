# appleII_keyboard_driver

## Descrizione
Firmware per Arduino micro per ricreare l'encoder SMC KR3600 tastiera Apple IIe.
Pilota righe/colonne del connettore 25poli della tastiera e fornisce in out i 7bit di decodifica del tasto premuto. 

**OUT_SERIAL_D** invia la decodifica del tasto in serie (input per shift register 74164)
**OUT_SERIAL_CLK** invia l'impulso per il campionamento del bit OUT_SERIAL_D (clock per shift register 74164)
**STROBE** indica il termine del caricamento dello shift register e comunica la presenza dei 7 bit di dato validi sull'out D0-D6
