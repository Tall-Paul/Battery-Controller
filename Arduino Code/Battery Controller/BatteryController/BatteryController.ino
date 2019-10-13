#include "heltec.h"
#include "WiFi.h"
#include "images.h"
#include <ArduinoMqttClient.h>


#define CUTOFF 11.7
#define CUTON 11.9

#define STATE_ON 1
#define STATE_OFF 0

#define VOLTAGE 36
#define RELAY 17
#define MAX_VALS 10
#define MAX_VALS_FLOAT 10.00

int current_state = 0;

float volts = 0;
int loop_val = 0;
float read_vals = 0.0;
float voltArray[MAX_VALS];

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

const char broker[] = "";
int        port     = 1883;
const char topic[]  = "car/voltage";



float readVoltage() {
    float total_volts = 0.0;
    volts = ((analogRead(VOLTAGE) / 1024.0) * 3.3) * 5.37;
    voltArray[loop_val] = volts;
    loop_val++;
    read_vals = read_vals + 1.0;
    if (loop_val == MAX_VALS){
        loop_val = 0;
    }
    if (read_vals >= MAX_VALS){
        read_vals = MAX_VALS_FLOAT;
    }
    for (int i = 0; i < read_vals; i++){
        total_volts = total_volts + voltArray[i];
    }
    float average_volts =  total_volts / read_vals;
    //bit hacky, but correct voltage based on measurements here
    float correction_factor = 0.0;
    if (average_volts <= 11.50){
        correction_factor = 0.15;
    } else if (average_volts <= 12.00) {
        correction_factor = 0.10;
    } else if (average_volts <= 12.50){
        correction_factor = 0;
    } else if (average_volts <= 12.80){
        correction_factor = -0.05;
    }
    return round((average_volts + correction_factor) * 10) / 10;
}



void switchOn(){
    digitalWrite(LED,HIGH);
    digitalWrite(RELAY,HIGH);
    current_state = STATE_ON;
    if (WiFi.status() == WL_CONNECTED){
        mqttClient.beginMessage("car/5v",true);
        mqttClient.print("on");
        mqttClient.endMessage();
    }
}

void switchOff(){
      digitalWrite(LED,LOW);
      digitalWrite(RELAY,LOW);
      current_state = STATE_OFF;
      if (WiFi.status() == WL_CONNECTED){
        mqttClient.beginMessage("car/5v",true);
        mqttClient.print("off");
        mqttClient.endMessage();
      }
}

void clientConnect(){
  byte count = 0;
  while(!mqttClient.connect(broker,port) && count < 10)
  {
    count ++;
    delay(500);
  }
}

void setup() {
   pinMode(VOLTAGE,INPUT);
   analogSetClockDiv(1); // 1338mS
   analogReadResolution(10);
   analogSetSamples(1);
   pinMode(LED,OUTPUT);
   pinMode(RELAY,OUTPUT);
   Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Enable*/, true /*Serial Enable*/);
    WiFi.mode(WIFI_STA);
	  WiFi.setAutoConnect(true);
	  WiFi.begin("SSID","KEY");
	  delay(100);
    clientConnect();
}


void loop(){
    volts = readVoltage();
    if (!mqttClient.clientConnected()){
      clientConnect();
    }
    mqttClient.poll();
    if (current_state == STATE_OFF){
        if (volts >= CUTON){
            switchOn();
        }
    } else if (current_state == STATE_ON){
        if (volts <= CUTOFF){
            switchOff();
        }
    }
    Heltec.display->clear();
    Heltec.display->setFont(ArialMT_Plain_24);
    Heltec.display->drawString(35, 20, String(volts) + "V");
    if (WiFi.status() == WL_CONNECTED){
        Heltec.display->drawXbm(112, 0, WIFI_width, WIFI_height, WIFI_bits);
        mqttClient.beginMessage(topic,true);
        mqttClient.print(String(volts));
        mqttClient.endMessage();
    }
    Heltec.display -> display();

    delay(1000);
}
