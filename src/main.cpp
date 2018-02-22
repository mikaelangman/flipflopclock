//
// Created by angman on 2/22/18.
//
#include <WiFiUdp.h>
#include "Arduino.h"
#include "stdio.h"
#include <WiFiUdp.h>
#include <TimeLib.h>

#include "ESP8266WiFi.h"
#include "ESP8266WebServer.h"
#include "ESP8266WiFiScan.h"

const char ssid[] = "FlipConfig";
const char pass[] = "flipflop";
char i_ssid[] = "Micci";
char i_pass[] = "kastadart";

const char ntpServerAdress[] = "pool.ntp.org";
const long ntpFirstFourBytes = 0xEC0600E3; // NTP request header
static WiFiUDP udp;
unsigned long ntpTime;
const byte tz = 1;



ESP8266WebServer server(80);



int wStatus = WL_IDLE_STATUS;


unsigned long inline getTime (UDP &udp){

    static int udpInited = udp.begin(1337);

    if(!udpInited){
        return 0;
    }

    udp.flush();

    if(!(udp.beginPacket(ntpServerAdress, 123)
        && udp.write((byte *)&ntpFirstFourBytes, 48)
        && udp.endPacket()))
        return 0;

    const int pollIntv = 150;
    const byte maxPoll = 15;
    int pktLen;

    for(byte i = 0; i< maxPoll; i++){
        if((pktLen = udp.parsePacket()) == 48)
            break;
        delay(pollIntv);
    }

    if(pktLen != 48)
        return 0;

    const byte useless = 40;
    for(byte i = 0; i < useless; i++){
        udp.read();
    }

    unsigned long time = udp.read();
    for (byte i = 1; i < 4; i++)
        time = time << 8 | udp.read();

    time += (udp.read() > 115 - pollIntv/8);
    udp.flush();

    return time;
}

void printWifiStatus() {
    // print the SSID of the network you're attached to:
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    // print your WiFi shield's IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");
}

void setup() {

    Serial.begin(115200);
    Serial.println("Attempting to set up WiFi");
    Serial.print("SSID: ");
    Serial.println(ssid);
    Serial.println("Pass: ");
    Serial.println(pass);



    WiFi.persistent(false);
    WiFi.mode(WIFI_OFF);   // this is a temporary line, to be removed after SDK update to 1.5.4
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(i_ssid, i_pass);

    while ( WiFi.status() != WL_CONNECTED) {
        Serial.print("Attempting to connect to SSID: ");
        Serial.println(i_ssid);
        // Connect to WPA/WPA2 network. Change this line if using open or WEP network:

        // wait 10 seconds for connection:
        delay(10000);
    }


    server.on("/", [](){

        server.send( 200, "text/plain", String(ntpTime) + " H:" + String(hour()) + " M: " + String(minute()) + " S: " + String(second()) );

    });

    server.begin();

    WiFi.softAP(ssid, pass);

    printWifiStatus();

    pinMode(BUILTIN_LED, OUTPUT);
}
void loop() {

    //--------------NTP-----------------

    if (WiFi.status() == WL_CONNECTED){

        unsigned long response = getTime(udp);
        if (response != 0)
            ntpTime = response;
            setTime(ntpTime - 2208988800UL + (3600 * tz));
    } else {
        //Reconnect?
    }

    //-----------SERVER PART-------------

    server.handleClient();

}
