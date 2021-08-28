// AEROPIC's ESP32 disco accro mod (august 2021)
// ============================================

// many thanks to :
// - based on Parachute mod (for the evtest command)
// - with contributions of freedom 2000 to automatically start the script from EPS in telnet

// V104 : 210818 : changed UDP packet read
// V103 : 210816 : added depth compensation during barrel rolls
// V102 : 210815 : changed the trims to raw SBUS value (neutral at 1250): trims could be useless
// V101 : 210814 : changed the servo law for SBUS (neutral at 1300): trims could be useless
// V100 : 210814 : added test trim in flight
//          added trim process on GPIO23
//          added exit manual mode with right stick
// V010 : 210812 : added duration of manoeuvers

#include <WiFi.h>
#include <WiFiUdp.h>
#include <math.h>

const char* ssid     = "DISCO-xxxxxx";
const char* password = "";
IPAddress local_IP(192, 168, 42, 25);
IPAddress gateway(192, 168, 42, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8); //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional

const uint16_t port = 23;            //telnet port
const char * host = "192.168.42.1"; // ip of chuck
// Use WiFiClient class to create TCP connections
WiFiClient client;

unsigned int localPort = 8888;      // local port to listen on
#define UDP_TX_PACKET_MAX_SIZE 2048
uint8_t packetBuffer[UDP_TX_PACKET_MAX_SIZE]; //buffer to hold incoming packet,
int packetSize;
WiFiUDP Udp;
long Xtime;
int Xcount = 0;
long watchdog ;


int connectionStatus = 0;
int maxloops = 0;
int keyCode;
int keyValue;
String line2 = "";

uint16_t channels[6];
int sticks[5];
int trims[5];
boolean trimming = false;
enum sticksName {ROLL, PITCH, THROTTLE, LOITER, WHEEL };
int channelIndex = 0;
int buttons[8];
boolean RCmode = false;
boolean prevRCmode = false;
enum buttonsName {A, B, SET, HOME, PINKIE_RIGHT, PINKIE_LEFT, WHEELR_RIGHT, WHEELR_LEFT};

#define WATCHDOG_DURATION 2500
#define ACCRO_BARREL_DURATION 100  // 150 x 10 ms = 1500 ms
#define ACCRO_TRIM_DURATION 200  // 200 x 10 ms = 2000 ms
#define ACCRO_BARREL 0
#define ACCRO_BARREL_LEFT 1
#define ACCRO_TRIM 2
#define ACCRO_HIMMELMAN 3  // todo ?

int accroCount = 0;  // x10ms : to limit the RCmode duration
int accroDuration; // will be affected with ACCRO_HIMMELMAN_DURATION or ACCRO_BARREL_DURATION
int accroMode;  // 0 : Barrel  1: Barrel left 2: trim 3 :Himmelman
int depthCompensation;

//servos
#define TIMER_WIDTH 16
#include "esp32-hal-ledc.h"


// define PINS for servo 1..8 for ESP 
#define  LED_PIN 2 
#define TRIM_PIN 23

//SBUS
#include "SBUS.h"

#define TX_PIN 13 // rather use pin 13 but dead on my ESP
#define RX_PIN 22 //useless

// a SBUS object, which is on hardware
// serial port 13
SBUS x8r(Serial1);
long pulse;

#include <Preferences.h> //ESP32 to store preferences
Preferences preferences;


void setup()
{
  Serial.begin(115200);

  // Open Preferences with my-app namespace. Each application module, library, etc
  // has to use a namespace name to prevent key name collisions. We will open storage in
  // RW-mode (second parameter has to be false).
  // Note: Namespace name is limited to 15 chars.
  preferences.begin(ssid, false);

//   preferences.putInt("trimRoll", 0);
//   preferences.putInt("trimPitch",0);

  //unsigned int counter = preferences.getUInt("counter", 0); // Get the counter value, if the key does not exist, return a default value of 0 / Note: Key name is limited to 15 chars.
  trims[0] =  preferences.getInt("trimRoll", 0);
  trims[1] =  preferences.getInt("trimPitch", 0);
//  trims[2] =  preferences.getInt("trimThrottle", 0);

  Serial.print("trimRoll ");
  Serial.println(trims[0]); //33
  Serial.print("trimPitch ");
  Serial.println(trims[1]);  //38

  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }

  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("ESP Mac Address: ");
  Serial.println(WiFi.macAddress());
  Serial.print("Subnet Mask: ");
  Serial.println(WiFi.subnetMask());
  Serial.print("Gateway IP: ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("DNS: ");
  Serial.println(WiFi.dnsIP());

  connectionStatus = 0;

  pinMode(LED_PIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  pinMode(TRIM_PIN,  INPUT_PULLUP); // init TRIM pin as input pullup

  for (int i = 0; i < 6; i++) channels[i] = 992;  //init the channels for SBUS between 172 and 1811 (992 middle)


  //led blink
  ledcSetup(1, 1, TIMER_WIDTH);   // channel 1, 1 Hz, 16-bit width
  ledcAttachPin(LED_PIN, 1);      // LED assigned to channel 7
  ledcWrite(1,  65535); // set blue LED ON while not connected to chuck
  
  //SBUS
  // begin the SBUS communication
  x8r.begin(RX_PIN, TX_PIN, true); // optional parameters for ESP32: RX pin, TX pin, inverse mode

  //UDP
  if (Udp.begin(localPort)) {
    Serial.print("UDP Listening on IP: ");
    Serial.println(WiFi.localIP());
  }
}

void loop()
{
  switch (connectionStatus)
  {
    case 0:           //not connected
      Serial.print("Connecting to ");
      Serial.println(host);
      if (!client.connect(host, port))
      {
        //       Serial.println("Connection failed.");
        return;
      }
      connectionStatus++;
      break;

    case 1:           // connected to telnet
      //wait for the server's reply to become available
      while (!client.available() && maxloops < 1000)
      {
        maxloops++;
        delay(1); //delay 1 msec
      }
      maxloops = 0;
      while ((client.available() > 0) && maxloops < 1000 )
      {
        //read back one line from the server
        String line = client.readStringUntil('\r');
        Serial.println(line);
        maxloops++;
      }
      if (maxloops < 1000)
      {
        client.print("./logsc2.sh\r"); //connect to SkyController2
        //client.print("help\r"); //for help
        maxloops = 0;
        connectionStatus++;
        Serial.print("./logsc2.sh\r : connection Status");
        Serial.println(connectionStatus);
      }
      else
      {
        Serial.println("client.available() timed out ");
        Serial.println("Closing connection.");
        client.stop();
        connectionStatus = 0;
      }
      break;

    case 2:           // connected to SC2
      maxloops = 0;
      while (!client.available() && maxloops < 1000)
      {
        maxloops++;
        delay(1); //delay 1 msec
      }
      maxloops = 0;
      while ((client.available() > 0) && maxloops < 1000 )
      {
        String line = client.readStringUntil('\r');
        Serial.println(line);
        maxloops++;
      }
      if (maxloops < 1000)
      {
        client.print("./data/lib/ftp/uavpal/bin/spysc2.sh\r"); //launch event logger (but does not work...)
        maxloops = 0;
        connectionStatus++;
        Serial.print("spysc2.sh : connection Status");
        Serial.println(connectionStatus);
      }
      else
      {
        Serial.println("connect sc2 timed out ");
        Serial.println("Closing connection.");
        client.stop();
        connectionStatus = 0;
      }
      break;

    case 3:           // launch the event logger
      while ((client.available() > 0) && maxloops < 10000 )
      {
        String line = client.readStringUntil('\r');
        Serial.println(line);
        maxloops++;
      }
      client.print("./data/lib/ftp/uavpal/bin/spysc2.sh\r"); //launch event logger
      connectionStatus++;
      Serial.println(connectionStatus);
      watchdog = millis();
      break;

    case 4:           // running
      packetSize = Udp.parsePacket(); // if there's data available, read a packet
      if (packetSize > 0) 
      {
        Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
        //                Serial.print("Udp: ");
        //                Serial.println((char *)packetBuffer);

        //read back one line from the server
        line2 = (char *)packetBuffer;

        //                Serial.print("Udp read line : ");
        //                Serial.println(line2);
        int index = line2.indexOf("g") ;  // search for "gxxx yyyLF" where xxx is the keycode yyy is the keyvalue
        if (index > -1)
        {
          //          Serial.print("code index : ");
          //                Serial.println(index);
          line2 = line2.substring(index + 1);
          index = line2.indexOf(" ");
          keyCode = line2.substring(0, index).toInt();
          //          Serial.print("code found: ");
          //          Serial.println(keyCode);
          index ++;
          line2 = line2.substring(index);
          index = line2.indexOf(char(10));       // search for " yyyLF" where yyy is the keyvalue
          if (index > -1)
          {
            keyValue = line2.substring(0, index).toInt();
            //            Serial.print("keyValue ");
            //            Serial.println(keyValue);

            //          Serial.println("************");
            //          char buf[100];
            //          line2.toCharArray(buf, line2.length());
            //          for (int i = 0; i < line2.length(); i++)
            //          {
            //            Serial.print(buf[i], HEX);
            //            Serial.print(" ");
            //          }
            //          Serial.println(" ");


           watchdog = millis(); // watchdog is set only if no SC2 UDP frame is received
           
            //now affect values to sticks and buttons
            switch (keyCode)
            {
              case 0:           //LX = LOITER (left = -, right = +)
                sticks[LOITER] = keyValue;
                break;
              case 1:           //LY = PITCH (bottom = -, up = +)
                sticks[PITCH] = -keyValue;
                break;
              case 2:           //RX = ROLL (left = -, right = +)
                sticks[ROLL] = keyValue;
                break;
              case 3:          //RY = THROTTLE (bottom = -, up = +)
                sticks[THROTTLE] = keyValue;
                break;
              case 4:          //WHEEL (left = -, right = +)
                sticks[WHEEL] = keyValue;
                break;
              case 288:          //SET
                buttons[SET] = keyValue;
                break;
              case 289:          //HOME
                buttons[HOME] = keyValue;
                break;
              case 291:          //B
                buttons[B] = keyValue;
                break;
              case 292:          //A
                buttons[A] = keyValue;
                break;
              case 293:          //PINKIE_RIGHT
                buttons[PINKIE_RIGHT] = keyValue;
                break;
              case 294:          //PINKIE_LEFT
                buttons[PINKIE_LEFT] = keyValue;
                break;
              case 298:          //WHEELR_LEFT
                buttons[WHEELR_LEFT] = keyValue;
                break;
              case 299:          //WHEELR_RIGHT
                buttons[WHEELR_RIGHT] = keyValue;
                break;
              default:
                break;
            }
          }
        }
      }
      break;
    default:
      break;
  }

  //detect RCmode ACCRO_BARREL or ACCRO_BARREL_LEFT or in flight TRIM
  if (buttons[PINKIE_RIGHT] == 1)      //RCmode BARREL toggle on with both pinkie and A
                                        
  {
    if (buttons[A] == 1) 
    {
      RCmode = true;
     accroDuration = ACCRO_BARREL_DURATION;
     accroMode = ACCRO_BARREL; // Barrel
    }
    else if (buttons[PINKIE_LEFT] == 1) // if both Pinkie left and right then test trim mode in flight
    {
      RCmode = true;
      accroMode = ACCRO_TRIM;
      accroDuration = ACCRO_TRIM_DURATION;
    }
  }  //end if button pinkie right
  else if (buttons[PINKIE_LEFT] == 1)   //RCmode BARREL_LEFT toggle on with both pinkie LEFT and A
  {
    if (buttons[A] == 1) 
    {
      RCmode = true;
     accroDuration = ACCRO_BARREL_DURATION;
     accroMode = ACCRO_BARREL_LEFT; // left barrel roll
    }
  } // end tests accro mode buttons pinkie R or L

  
  // switch back in auto mode after WATCHDOG_DURATION (2.5 sec) // fail safe if connection lost
  if (((millis() - watchdog) > WATCHDOG_DURATION) && RCmode)
  {
    RCmode = false;
  
    watchdog = millis();
    buttons[PINKIE_RIGHT] = 0;
    buttons[PINKIE_LEFT] = 0;
    buttons[A] = 0;
    accroCount = 0;
    
  }

   // exit RC mode if right stick not at neutral
  if (((sticks[ROLL] < -10) || (sticks[ROLL] > 10) || (sticks[THROTTLE] < -10) || (sticks[THROTTLE] > 10) ) && RCmode ) 
  {
    RCmode = false;
     accroCount = 0;
  }
  
  
  if (accroMode == ACCRO_BARREL) 
  {
    depthCompensation = int(100.0 * cos(-0.0628318*accroCount)); //int(20.0 * cos(-3.6*accroCount*PI/180));
    channels[0] = 1811; // full roll right
    channels[1] = 1250 +  trims[1] + depthCompensation ; // neutral on pitch //channels[1] = 992 + 820 * ( trims[1]) / 100; // neutral on pitch
    channels[2] = 1300; // 2/3rd gaz
  }
  else if (accroMode == ACCRO_BARREL_LEFT) 
  {
    depthCompensation = int(100.0 * cos(-0.0628318*accroCount)); //int(20.0 * cos(-3.6*accroCount*PI/180));
    channels[0] = 690;  // full roll left (172 would be too much)
    channels[1] = 1250 +  trims[1] + depthCompensation ; // neutral on pitch
    channels[2] = 1300; // 2/3rd gaz
  }
  else if (accroMode == ACCRO_TRIM) 
   {
    channels[0] = 1250 + trims[0] ; // neutral on roll
    channels[1] = 1250 + trims[1] ; // neutral on pitch 
    channels[2] = 1200; //1811; // mid gaz
  }
  

  if (RCmode) 
  {
    channels[4] = 1811;  //force channel 5 to max PWM value to fly with RC
  }
  else {  // not RC mode
    channels[4] = 172;         //force channel 5 to min PWM value to fly with SkyController2
    accroCount = 0;
  }

// trims management (on ground only): trim offset is limited to +/-100 at a time in raw SBUS value
// move both sticks depth and roll to get the wanted position of the servos arms. 
// then press and release the left trigger (values are stored on release)
// Once done, the servo arms will move by the trims values and go back to the neutral position when centering the sticks.

  if (digitalRead(TRIM_PIN)==0) // trim pin connected to ground: force RCmode and manage trims here
  {
    channels[0] = 1250 + trims[0] + sticks[0] ; 
    channels[1] = 1250 + trims[1] + sticks[1]; 
    channels[2] = 172; // no gaz 
    channels[4] = 1811;  //force channel 5 to max PWM value to fly with RC and switch to RCmode

    if (buttons[PINKIE_LEFT] == 1)  // compute and set the trims
    {
      trimming = true;
    }
    else  // PINKIE_LEFT release ==> if trimming in progress compute and store the trims
    {
      if (trimming)
      {
        trimming = false;
        trims[0] = sticks[0]+ trims[0];
       
        trims[1] = sticks[1] + trims[1];
        preferences.putInt("trimRoll", trims[0]);
        preferences.putInt("trimPitch", trims[1]);
//        preferences.putInt("trimThrottle", trims[2]);
      }
    }  //end if button pinkie left
  } // end trims management
  
  //servos pulse generation
  if ((millis() - pulse) > 10) //every 10ms
  {
    pulse = millis();
    
    x8r.write(&channels[0]); // write the SBUS packet to an SBUS compatible servo
    
    if (RCmode) // manage the accro timeout
    {
      accroCount = accroCount +1;

      
       if (accroCount >= accroDuration) // exit RC mode
      {  
        accroCount = 0;
      //  accroDuration = 0;
        RCmode = false;
      }
    }
  }

// display RCmode on console
  if (prevRCmode != RCmode)
    {
      Serial.print("RCmode ");
      Serial.println(RCmode);
      prevRCmode = RCmode;
    }

 // manage blue LED flashing
  if ((RCmode) && (connectionStatus >3))  // when connected to CHUCK blue LED will turn off, then flash in RCmode
  {
    ledcWrite(1,  65535 / 2); //just to flash the blue led
  }
  else
  {
    ledcWrite(1,  0); //just to turn off the blue led
  }
}
