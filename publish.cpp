// Based on the Paho C code example from www.eclipse.org/paho/
#include <iostream>
#include <sstream>
#include <fstream>
#include <string.h>
#include <cmath> 
#include "MQTTClient.h"
#include "ADXL345.h"
#define  CPU_TEMP "/sys/class/thermal/thermal_zone0/temp"
using namespace std;
using namespace exploringRPi;
//Please replace the following address with the address of your server
#define ADDRESS    "tcp://192.168.121.207:1883"
#define CLIENTID   "rpi1"
#define AUTHMETHOD "vignesh"
#define AUTHTOKEN  "212217121060"
#define TOPIC      "ee513/CPUTemp"
#define QOS        2
#define TIMEOUT    10000L
float pitch;
float roll;
float heading;
float getCPUTemperature() {        // get the CPU temperature
   int cpuTemp;
                    // store as an int
   fstream fs;
   fs.open(CPU_TEMP, fstream::in); // read from the file
   fs >> cpuTemp;
   fs.close();
   return (((float)cpuTemp)/1000);
}
// Function to calculate compass heading from pitch and roll
float calculateCompassHeading(float &pitch, float &roll) {
    // Calculate tilt-compensated compass heading
    float x = cos(pitch * M_PI / 180.0) * sin(roll * M_PI / 180.0);
    float y = sin(pitch * M_PI / 180.0);
    float heading = atan2(y, x) * 180.0 / M_PI;
    if (heading < 0) {
        heading += 360.0;
    }
    return heading;
}

int main(int argc, char* argv[]) {
 ADXL345 sensor(1,0x53);
   sensor.setResolution(ADXL345::NORMAL);
   sensor.setRange(ADXL345::PLUSMINUS_4_G);
//   sensor.displayPitchAndRoll();

   char str_payload[100];          // Set your max message size here
   MQTTClient client;
   MQTTClient_connectOptions opts = MQTTClient_connectOptions_initializer;
   MQTTClient_message pubmsg = MQTTClient_message_initializer;
   MQTTClient_deliveryToken token;
   MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
   opts.keepAliveInterval = 20;
   opts.cleansession = 1;
   opts.username = AUTHMETHOD;
   opts.password = AUTHTOKEN;

// Set the callback function for disconnection events
//    MQTTClient_setCallbacks(client, NULL, NULL, onConnectionLost, NULL);
   int rc;

   if ((rc = MQTTClient_connect(client, &opts)) != MQTTCLIENT_SUCCESS) {
      cout << "Failed to connect, return code " << rc << endl;
      return -1;
   }
while(true){
   sensor.storePitchAndRollData(pitch,roll,1);
heading = calculateCompassHeading(pitch, roll);

   sprintf(str_payload, "{\"CPUTemp\": %f,\"pitch\": %f,\"roll\": %f}}",
                getCPUTemperature(), pitch, roll);
   pubmsg.payload = str_payload;
   pubmsg.payloadlen = strlen(str_payload);
   pubmsg.qos = QOS;
   pubmsg.retained = 0;
   MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
   cout << "Waiting for up to " << (int)(TIMEOUT/1000) <<
        " seconds for publication of " << str_payload <<
        " \non topic " << TOPIC << " for ClientID: " << CLIENTID << endl;
   rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
   cout << "Message with token " << (int)token << " delivered." << endl;
  
}   
MQTTClient_disconnect(client, 10000);
   MQTTClient_destroy(&client);
   return rc;
}
