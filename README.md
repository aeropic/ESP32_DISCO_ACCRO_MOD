
# ESP32_DISCO_ACCRO_MOD
A way to perform basic aerobatics with the disco controlled only by the skycontroller !

[![Youtube video](https://uavpal.com/img/yt_thumbail_github.png)](https://www.youtube.com/watch?v=nRvVFVoBn40)

[![Youtube video]()](https://www.youtube.com/watch?v=nRvVFVoBn40)
## Basic idea

An ESP32 is connected to the manual mode port of the CHUCK.

In the CHUCK is installed the 4G LTE mod (many thanks and respect to the authors). The modem is not required, only the software mod.

In addition of the 4G LTE, there is also a dedicated script to spy the SC2.

Once powered, the ESP32 will connect to the chuck WIFI. It then establishes a telnet session to remotely launch the spySC2.sh script. Then it connects to the CHUCK in UDP.
The script sends any change of the sticks and buttons values of the SC2 to the ESP32 (with UDP packets).

The ESP32 analyzes the buttons presses and when a sequence of keys is found (eg "right trigger + A button"), it generates  SBUS frames to switch the CHUCK into "manual mode" with the appropriate sequence of orders to generate the aerobatic figure (eg: full gaz and full right stick to perform a right barrel roll)...

This lasts a maximum of 1 second. It can be stopped by a move of right stick.

Once stopped, the ESP32 sends SBUS frames to switch back the CHUCK into autopilot mode... 

Of course it is not perfect as the depth is not controlled during the roll :-)

## Hardware

You need only 
-	an ESP32 board with a USB plug. I use the ESP32 DEVKIT V1 but any would do the job.
-	A female servo wire

Solder the servo wire :
-	red to Vin pin
-	black or brown to GND
-	orange or yellow (signal) to GPIO13 (D13 on the DEVKIT board)

## software

### DISCO

the 4G LTE mod shall be installed even if not used.
The accro mod will work fine without the 4G LTE modem.

The spysc2.sh script must be installed into the skycontroller2. 
The simplest way to do it is to add the file into the “skycontroller2/uavpal/bin” directory of the 4G LTE install directory. Then when installing the 4G LTE the install script will do the job and place the file in the right folder 

### ESP32

The ESP32 shall be programmed with the “ESP32_disco_accro_mod.ino” script.
This script requires the “SBUS_master” library to be downloaded here: https://github.com/TheDIYGuy999/SBUS  
(it is included in the zip to facilitate the process).

This line shall be changed to your disco’s wifi SSID :
Const char* ssid = “DISCO-xxxxxx”;

That’s all you have to do. Compile and upload to the ESP32 and you’re done.

## Users manual

### INIT SEQUENCE AND THE BLUE LED
Connect the ESP32 to the CHUCK manual mode port (the horizontal servo plug close the battery plug)
Turn ON the CHUCK. After few seconds, the blue LED of the ESP shall turn ON.
Turn ON the SC2.
Double press on the Pitot tube to start the disco’s telnet server (the Pitot LED shall flash twice in purple)
After few seconds, the ESP32 blue LED shall turn OFF, this means it is properly connected to the DISCO in wifi and that it has done its job to start the spySC2.sh script…

To finish with the blue LED, it will flash when executing manual mode aerobatics sequences controlled by the ESP32.


### HOW TO TRIM

The disco cannot fly in manual mode if it is not trimmed properly. So there is way to trim it...

Before turning the ESP ON, connect GPIO23 of the ESP (pin labelled D23 on my devkit) to GND, connect the ESP to the CHUCK, boot the disco, switch ON the telnet protocol (double press on the Pitot tube, it shall blink in purple twice).
The ESP is then in trim mode on ground. The disco servos reach their neutral position. Most probably, if the disco is with stock servos, the servo arms should be in correct position (same level for right and left arms, both pointing upwards). If this is the case, don’t touch anything, just remove the GPIO23 wire.

If a trim is needed, just move roll stick and depth stick to reach the required neutral position (right arm and left arm shall be level, both pointing few mm upwards) . Keep the position, press LEFT_TRIGGER and release. The servo will slightly move and if you release the sticks (let them go to the center position), the servos will reach the required trimmed neutral position.
The programming is done and stored into a preference file into the ESP.
You can unplug the GPIO23 pin. 

### THE “AEROBATICS” SEQUENCES
They can be triggered with a sequence of 2 keys from the skycontroller2.

There are 3 manual mode aerobatics sequences:
-	Right barrel roll: press together RIGHT_TRIGGER + A (in this order) and the disco shall do a right barrel roll during one second
-	left barrel roll: press together LEFT_TRIGGER + A (in this order) and the disco shall do a left barrel roll during one second
-	in flight trim test: press together RIGHT_TRIGGER + LEFT_TRIGGER (in this order) and the disco shall do a flat level flight during two second. You shall hear the motor slightly accelerating… This is a way to test the trims are correct. Do it in no wind condition. Allow an attitude not fully flat (~20 deg) at the end of the two seconds. 

Once started, any aerobatics sequence can be stopped simply moving the roll stick or the throttle stick. Otherwise they will automatically end after their default duration. And there is a 3rd safety measure, if no frames are received from the skycontroller during 2,5 seconds, the manual mode is exited too.

I would advise you test all this on ground after having removed the propeller, just to train and check everything is working as expected.
If you keep the propeller, if the GPS is fixed, the motor will rotate and rotate fast, you’re warned!

## ONE LAST WORD
This mod is in a very experimental state. You have to understand the disco will fly uncontrolled during one or two seconds. At the end of a barrel roll, according to the wind condition, the attitude might be surprising (diving, not flat…) to recover such a situation, the CHUCK will perform some oscillations which may last one or two seconds. During this, the wing may lose some altitude and this is the reason why I strongly recommend to play with this only above 60 m or even higher 150m for your first trys.

Finally, I cannot be taken for responsible in case you crash your precious disco…

This being said, it’s so fun 

PS: I tried also to program a looping with moderate success… The duration of a loop is longer than the one second barrel roll and the uncontrolled roll with some wind heavily disturbs the shape of the loop (to say the least). So unless if we add a gyro to the ESP, this kind of aerobatics cannot be done…
