//přijímač ve vznášedle
#include <WiFi.h>
#include <esp_now.h>
#include <ESP32Servo.h>

Servo ServoZataceni;
Servo ESRVrtule;
Servo ESRTurbina;

typedef struct {
  int16_t x;
  int16_t y;
  int16_t t;
} ESPNPacket;
ESPNPacket incoming;

volatile bool bylPrijem = false; //kvůli watchdogu
bool ztrataSignalu = false; //kvůli autostopu
hw_timer_t *wdt = NULL; 


void IRAM_ATTR kontrolawd() {
//přerušení od časovače, jednou za sekundu zkontroluje, zda za poslední sekundu byl alespoň jeden příjem
//interval vysílání je 10/s, tedy muselo by dojít k deseti výpadkům příjmu po sobě
  if (!bylPrijem) ztrataSignalu=true;
  bylPrijem = false; 
} 


void onReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
  bylPrijem = true; //anulace autostopu od watchdogu

  if (len == sizeof(incoming)) {
    int servoZataceniHodnota;
    int ESRVrtuleHodnota;
    int ESRTurbinaHodnota;
    memcpy(&incoming, data, sizeof(incoming));
    //TODO: watchdog
    
    servoZataceniHodnota=map(incoming.x, -1000, 1000, 1000, 2000);
    if(incoming.y<0) incoming.y=0; //zatím necouvám
    ESRVrtuleHodnota=incoming.y+1000; //klid je 0=>1000us, plný výkon je 1000=>2000us
    ESRTurbinaHodnota=incoming.t+1000; //klid je 0=>1000us, plný výkon je 1000=>2000us
    
    //Serial.printf("zataceni:%d|tah:%d|turbina:%d\n",servoZataceniHodnota,ESRVrtuleHodnota,ESRTurbinaHodnota);
    ServoZataceni.writeMicroseconds(servoZataceniHodnota);
    ESRVrtule.writeMicroseconds(ESRVrtuleHodnota);
    ESRTurbina.writeMicroseconds(ESRTurbinaHodnota);

  } else {
    Serial.println("Bordel v přijatých datech!");
  }

  
}

void setup() {
  Serial.begin(115200);
  
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init selhal");
    return;
  }

  esp_now_register_recv_cb(onReceive);

  ServoZataceni.attach(25,1000,2000);
  ServoZataceni.writeMicroseconds(1500); //startovní pozice střed
  ESRVrtule.attach(26,1000,2000);
  ESRVrtule.writeMicroseconds(1000); //startovní rychlost nula
  ESRTurbina.attach(27,1000,2000);
  ESRTurbina.writeMicroseconds(1000); //startovní rychlost nula

  //časovač pro watchdog příjmu ovládání
  wdt = timerBegin(1000000); //1 MHz
  timerAttachInterrupt(wdt, &kontrolawd);
  timerAlarm(wdt, 1000000, true,0); // 1s, tedy milion mikrosekund

}

void loop() {

  //Serial.printf("zataceni:%d|tah:%d|turbina:%d\n",servoZataceni.read(),ESRVrtuleHodnota.read(),ESRTurbinaHodnota.read());

  if (ztrataSignalu){ //klidový stav
    ServoZataceni.writeMicroseconds(1500); //klidová pozice střed
    ESRVrtule.writeMicroseconds(1000); //klidová rychlost nula
    ESRTurbina.writeMicroseconds(1000); //klidová rychlost nula
    Serial.println("Autostop");
    ztrataSignalu=false; //serva a motory jsou nastavené na klid, do příštího příjmu je nic nerozhýbe
  }
}
