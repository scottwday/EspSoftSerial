# EspSoftSerial
Software serial receiver for ESP8266

Only supports 8N1 (or 7E1 or 7O1 if you're willing to check parity yourself.)

Uses a pin change interrupt and the CPU tick timer as a uart receiver.

Supports up to 4 receivers
Up to about 19200 baud, results may vary

Be sure to call service() every 10s or so or you may lose the last received byte if it has a high MSB, and you may have glitches when the tick counter rolls over.
