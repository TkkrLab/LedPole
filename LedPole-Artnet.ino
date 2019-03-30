/*
 * Author: Duality
 * Mail: robertvdtuuk@gmail.com
 * Github: https://github.com/Duality4Y
 * 
 * TODO:
 *  Startup test cycle through rgbw to verify leds
 *  Possibly display status on leds if it couldn't connect to wifi.
 *  (if it did connect turn off leds.
 *  Timer clearning leds if not received data for X time.
 *  Timer for refreshing (fastled show) only refresh 60 times a second.
*/

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ArtNode.h>
#include <FastLED.h>

#define BUFFERSIZE 1024
#define VERSION_HI 0
#define VERSION_LO 1

char *ssid = "SomeKindOfSSID";
char *pass = "SomeKindOfPassword";
// char *hostName = "Led-PoleX";

byte buffer[BUFFERSIZE];

IPAddress subnetmask;
IPAddress localIp;
WiFiUDP udp;


uint8_t ledmap[] = {
  0, 60, 59, 117, 1, 61, 58, 116, 2, 62, 57, 115, 3, 63, 56, 114, 4, 64, 55, 113, 5, 65, 54, 112, 6, 66, 53, 111, 7, 67, 
  52, 110, 8, 68, 51, 109, 9, 69, 50, 108, 10, 70, 49, 107, 11, 71, 48, 106, 12, 72, 47, 105, 13, 73, 46, 104, 14, 74, 45, 103, 
  15, 75, 44, 102, 16, 76, 43, 101, 17, 77, 42, 100, 18, 78, 41, 99, 19, 79, 40, 98, 20, 80, 39, 97, 21, 81, 38, 96, 22, 82, 
  37, 95, 23, 83, 36, 94, 24, 84, 35, 93, 25, 85, 34, 92, 26, 86, 33, 91, 27, 87, 32, 90, 28, 88, 31, 89, 29, 30, 
};

ArtNode node;

ArtConfig ArtnetConfig = {
  .mac =  { 0, 0, 0, 0, 0, 0 },
  .ip =   { 0, 0, 0, 0 },
  .mask = { 0, 0, 0, 0 },
  .udpPort = 0x1936,
  .dhcp = true,
  .net = 0,
  .subnet = 0,
  "Led-Pole",
  "Led-Pole Artnet Software by Duality at Tkkrlab",
  .numPorts = 1,
  .portTypes = { PortTypeDmx | PortTypeInput,
                 PortTypeDmx | PortTypeInput,
                 PortTypeDmx | PortTypeInput,
                 PortTypeDmx | PortTypeInput },
  .portAddrIn = { 0, 0, 0, 0 },
  .portAddrOut = { 0, 0, 0, 0 },
  .verHi = VERSION_HI,
  .verLo = VERSION_LO
};

// const int numLeds = 88;
const int numLeds = 118;
const int numberOfChannels = numLeds * 3;
const byte dataPin = 13;
const byte clockPin = 14;
CRGB leds[numLeds];

void printArtnetConfig()
{
  Serial.printf(
    "ArtnetConfig.mac         = 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x \r\n"
    "ArtnetConfig.ip          = %d.%d.%d.%d \r\n"
    "ArtnetConfig.mask        = %d.%d.%d.%d \r\n"
    "ArtnetConfig.udpPort     = %d \r\n"
    "ArtnetConfig.dhcp        = %s \r\n"
    "ArtnetConfig.net         = %d \r\n"
    "ArtnetConfig.subnet      = %d \r\n"
    "ArtnetConfig.shortName   = %s \r\n"
    "ArtnetConfig.longName    = %s \r\n"
    "ArtnetConfig.numPorts    = %d \r\n"
    "ArtnetConfig.portTypes   = 0x%x, 0x%x, 0x%x, 0x%x \r\n"
    "ArtnetConfig.portAddrIn  = 0x%x, 0x%x, 0x%x, 0x%x \r\n"
    "ArtnetConfig.portAddrOut = 0x%x, 0x%x, 0x%x, 0x%x \r\n"
    "ArtnetConfig.verHi       = %d \r\n"
    "ArtnetConfig.verLi       = %d \r\n",
    ArtnetConfig.mac[0], ArtnetConfig.mac[1], ArtnetConfig.mac[2], ArtnetConfig.mac[3], ArtnetConfig.mac[4], ArtnetConfig.mac[5],
    ArtnetConfig.ip[0], ArtnetConfig.ip[1], ArtnetConfig.ip[2], ArtnetConfig.ip[3],
    ArtnetConfig.mask[0], ArtnetConfig.mask[1], ArtnetConfig.mask[2], ArtnetConfig.mask[3],
    ArtnetConfig.udpPort,
    ArtnetConfig.dhcp ? "True" : "False",
    ArtnetConfig.net,
    ArtnetConfig.subnet,
    ArtnetConfig.shortName,
    ArtnetConfig.longName,
    ArtnetConfig.numPorts,
    ArtnetConfig.portTypes[0], ArtnetConfig.portTypes[1], ArtnetConfig.portTypes[2], ArtnetConfig.portTypes[3],
    ArtnetConfig.portAddrIn[0], ArtnetConfig.portAddrIn[1], ArtnetConfig.portAddrIn[2], ArtnetConfig.portAddrIn[3],
    ArtnetConfig.portAddrOut[0], ArtnetConfig.portAddrOut[1], ArtnetConfig.portAddrOut[2], ArtnetConfig.portAddrOut[3],
    ArtnetConfig.verHi,
    ArtnetConfig.verLo
    );
}

void setupSerial()
{
  // Serial debug connection.
  Serial.begin(115200);
  Serial.println();
}

void setupWifi()
{
  // start wifi in station mode and connect.
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  // WiFi.hostname(hostName);
  while(WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(500);
  }
  Serial.println("Connected to Acces Point.");
}

void setupArtnet()
{
  // set the mac address in the artnet config.
  byte wifimac[6];
  WiFi.macAddress(wifimac);
  for(int i = 0; i < 6; i++) ArtnetConfig.mac[i] = wifimac[i];
  // set the ip/broadcast address in the ArtnetConfig.
  localIp = WiFi.localIP();
  subnetmask = WiFi.subnetMask();
  for(int i = 0; i < 4; i++) ArtnetConfig.ip[i] = localIp[i];
  for(int i = 0; i < 4; i++) ArtnetConfig.mask[i] = subnetmask[i];
  
  // initialize the artnet node.
  printArtnetConfig();
  node = ArtNode(ArtnetConfig, BUFFERSIZE, buffer);
}

void setupUdpService()
{
  // start udp listening service on the artnet port.
  udp.begin(ArtnetConfig.udpPort);
}

void setupLeds()
{
  FastLED.addLeds<APA102, dataPin, clockPin, BGR>(leds, numLeds).setCorrection(TypicalLEDStrip);
  // cycle through some patterns to show it's booting.
}

void setup()
{
  setupSerial();
  setupWifi();
  setupArtnet();
  setupUdpService();
  setupLeds();
}

void loop()
{
  // check for udp data.
  while(udp.parsePacket())
  {
    int n = udp.read(buffer, min(udp.available(), BUFFERSIZE));
    // check if there is atleast header data and if it is a valid packet.
    if(n >= sizeof(ArtHeader) && node.isPacketValid())
    {
      // parse opcodes so that we can act accordingly
      switch(node.getOpCode())
      {
        case OpPoll:
        {
          // construct a ArtPoll reply
          ArtPoll* poll = (ArtPoll*)buffer;
          node.createPollReply();
          
          // send the reply right back to the remote (no need to broadcast)
          udp.beginPacket(udp.remoteIP(), ArtnetConfig.udpPort);
          udp.write(buffer, sizeof(ArtPollReply));
          udp.endPacket();
        } break;
        case OpDmx: {
          // parse dmx data.
          ArtDmx* dmx = (ArtDmx *)buffer;
          int port = node.getPort(dmx->Net, dmx->SubUni);
          int len = dmx->getLength();
          
          // access pointer to the data.
          byte *data = dmx->Data;
          
          if(port >= 0 && port < ArtnetConfig.numPorts)
          {
            for(int i = 0; i < numLeds; i++)
            {
              int p = ledmap[i];
              leds[p] = CRGB(data[i * 3], data[i * 3 + 1], data[i * 3 + 2]);
            }
            FastLED.show();
          }
        } break;
      }
    }
  }
}
