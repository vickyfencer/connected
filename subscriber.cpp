#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "MQTTClient.h"
#include <pigpio.h>
#include <cmath>


#define ADDRESS     "tcp://192.168.121.207:1883"
#define CLIENTID    "rpi2"
#define AUTHMETHOD  "vignesh"
#define AUTHTOKEN   "212217121060"
#define TOPIC       "ee513/CPUTemp"
#define PAYLOAD     "Hello World!"
#define QOS         1
#define TIMEOUT     10000L

volatile MQTTClient_deliveryToken deliveredtoken;
int token_count = 0; 
 float pitch, roll;
void controlLED() {
    const int LED_PIN = 26; // Assuming GPIO pin 17 for the LED

    if (gpioInitialise() < 0) {
        fprintf(stderr, "Failed to initialize pigpio\n");
        return;
    }

    gpioSetMode(LED_PIN, PI_OUTPUT);

    if (token_count >= 20) {
        // If token count reaches 20, stop blinking and turn on the LED
        gpioWrite(LED_PIN, 1);
        printf("LED is glowing\n");
    } else {
        // Blink the LED
        gpioWrite(LED_PIN, 1);
        printf("LED is blinking\n");
        gpioDelay(500000); // 500ms delay
        gpioWrite(LED_PIN, 0);
        gpioDelay(500000); // 500ms delay
    }

//    gpioTerminate();
}
void controlLEDByCompass(float pitch, float roll) {
    const int LED_PIN = 16;
    float heading = atan2(sin(roll), cos(pitch) * cos(roll)) * 180 / M_PI;
    
    if ( heading <= 270) {
        // East or West: Blink LED faster
        gpioDelay(250000); // 250ms delay
        gpioWrite(LED_PIN, 1);
        printf("Pointing East\n");
        gpioDelay(250000); // 250ms delay
        gpioWrite(LED_PIN, 0);
    } else {
        // Other directions: Blink LED slower
        gpioDelay(1000000); // 1s delay
        gpioWrite(LED_PIN, 1);
        printf("Pointing Other Directions\n");
        gpioDelay(1000000); // 1s delay
        gpioWrite(LED_PIN, 0);
    }
}

void delivered(void *context, MQTTClient_deliveryToken dt) {
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
    
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    int i;
token_count++;
 float heading;
    
    char* payloadptr;
    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: ");
    payloadptr = (char*) message->payload;
    for(i=0; i<message->payloadlen; i++) {
        putchar(*payloadptr++);
    }
    putchar('\n');
controlLED();
  sscanf((char *)message->payload, "{\"d\":{\"pitch\": %f, \"roll\": %f", &pitch, &roll);
   controlLEDByCompass(pitch, roll);
 MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
   

}

void connlost(void *context, char *cause) {
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}
 
int main(int argc, char* argv[]) {
    MQTTClient client;
    MQTTClient_connectOptions opts = MQTTClient_connectOptions_initializer;
    int rc;
    int ch;
    MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    opts.keepAliveInterval = 20;
    opts.cleansession = 1;
    opts.username = AUTHMETHOD;
    opts.password = AUTHTOKEN;

    MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);
    if ((rc = MQTTClient_connect(client, &opts)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect, return code %d\n", rc);
        exit(-1);
    }
    printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", TOPIC, CLIENTID, QOS);
    MQTTClient_subscribe(client, TOPIC, QOS);
 
    do {
        ch = getchar();
    } while(ch!='Q' && ch != 'q');
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}
