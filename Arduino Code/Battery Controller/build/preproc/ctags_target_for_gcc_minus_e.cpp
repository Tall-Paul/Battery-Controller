# 1 "e:\\Google Drive\\Arduino\\Battery Controller\\BatteryController\\BatteryController.ino"
# 1 "e:\\Google Drive\\Arduino\\Battery Controller\\BatteryController\\BatteryController.ino"
# 2 "e:\\Google Drive\\Arduino\\Battery Controller\\BatteryController\\BatteryController.ino" 2
# 3 "e:\\Google Drive\\Arduino\\Battery Controller\\BatteryController\\BatteryController.ino" 2
# 4 "e:\\Google Drive\\Arduino\\Battery Controller\\BatteryController\\BatteryController.ino" 2
# 5 "e:\\Google Drive\\Arduino\\Battery Controller\\BatteryController\\BatteryController.ino" 2


#define CUTOFF 12.0
#define CUTON 12.2

#define STATE_ON 1
#define STATE_OFF 0

#define VOLTAGE 36
#define RELAY 2
#define MAX_VALS 10
#define MAX_VALS_FLOAT 10.00

int current_state = 0;

float volts = 0;
int loop_val = 0;
float read_vals = 0.0;
float voltArray[10];

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

const char broker[] = "deicist.co.uk";
int port = 1883;
const char topic[] = "car/voltage";



float readVoltage() {
    float total_volts = 0.0;
    volts = ((analogRead(36) / 1024.0) * 3.3) * 5.37;
    voltArray[loop_val] = volts;
    loop_val++;
    read_vals = read_vals + 1.0;
    if (loop_val == 10){
        loop_val = 0;
    }
    if (read_vals >= 10){
        read_vals = 10.00;
    }
    for (int i = 0; i < read_vals; i++){
        total_volts = total_volts + voltArray[i];
    }
    float average_volts = total_volts / read_vals;
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
    digitalWrite(LED,0x1);
    digitalWrite(2,0x1);
    current_state = 1;
    if (WiFi.status() == WL_CONNECTED){
        Heltec.display->drawXbm(112, 0, 14, 8, WIFI_bits);
        mqttClient.beginMessage("car/5v",true);
        mqttClient.print("on");
        mqttClient.endMessage();
    }
}

void switchOff(){
      digitalWrite(LED,0x0);
      digitalWrite(2,0x0);
      current_state = 0;
      if (WiFi.status() == WL_CONNECTED){
        mqttClient.beginMessage("car/5v",true);
        mqttClient.print("off");
        mqttClient.endMessage();
      }
}

void setup() {
   pinMode(36,0x01);
   analogSetClockDiv(1); // 1338mS
   analogReadResolution(10);
   analogSetSamples(1);
   pinMode(LED,0x02);
   pinMode(2,0x02);

   Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Enable*/, true /*Serial Enable*/);

    WiFi.mode(WIFI_MODE_STA);
 WiFi.setAutoConnect(true);
 WiFi.begin("PrettyFlyForAWifi","gy3UwuaJPcAv");
 delay(100);
    byte count = 0;
 while(!mqttClient.connect(broker,port) && count < 10)
 {
  count ++;
  delay(500);
 }
}


void loop(){
    volts = readVoltage();
    mqttClient.poll();
    if (current_state == 0){
        if (volts >= 12.2){
            switchOn();
        }
    } else if (current_state == 1){
        if (volts <= 12.0){
            switchOff();
        }
    }

    Heltec.display->clear();
    Heltec.display->setFont(ArialMT_Plain_24);
    Heltec.display->drawString(35, 20, String(volts) + "V");
    if (WiFi.status() == WL_CONNECTED){
        Heltec.display->drawXbm(112, 0, 14, 8, WIFI_bits);
        mqttClient.beginMessage(topic,true);
        mqttClient.print(String(volts));
        mqttClient.endMessage();
    }
    Heltec.display -> display();

    delay(1000);
}
