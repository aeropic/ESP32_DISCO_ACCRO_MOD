[ESP32_disco_accro_mod-V101.zip](https://github.com/aeropic/ESP32_DISCO_ACCRO_MOD/files/6987193/ESP32_disco_accro_mod-V101.zip)
# ESP32_DISCO_ACCRO_MOD
A way to perform basic aerobatics with the disco controlled only by the skycontroller !

1.	Basic idea

An ESP32 is connected to the manual mode port of the CHUCK.

In the CHUCK is installed the 4G LTE mod (many thanks and respect to the authors). The modem is not required, only the software mod.

In addition of the 4G LTE, there is also a dedicated script to spy the SC2.

Once powered, the ESP32 will connect to the chuck WIFI. It then establishes a telnet session to remotely launch the spySC2.sh script. Then it connects to the CHUCK in UDP.
The script sends any change of the sticks and buttons values of the SC2 to the ESP32 (with UDP packets).

The ESP32 analyzes the buttons presses and when a sequence of keys is found (eg "right trigger + A button"), it generates  SBUS frames to switch the CHUCK into "manual mode" with the appropriate sequence of orders to generate the aerobatic figure (eg: full gaz and full right stick to perform a right barrel roll)...

This lasts a maximum of 1 second. It can be stopped by a move of right stick.

Once stopped, the ESP32 sends SBUS frames to switch back the CHUCK into autopilot mode... 

Of course it is not perfect as the depth is not controlled during the roll :-)

