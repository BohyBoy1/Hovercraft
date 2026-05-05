//Vysílač (ovladač)
#include <WiFi.h>
#include <esp_now.h>

uint8_t receiverMac[] = {0x08, 0xA6, 0xF7, 0xBC, 0x15, 0xF0};

void onSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  //Serial.println(status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
}

//konstanty
const int cJoyXpin = 32; // potenciometr joy osa X je GPIO pin 32 (ADC1 CH4)
const int cJoyYpin = 33; // potenciometr joy osa Y je GPIO pin 33 (ADC1 CH5)
const int cPotTur = 34; // potenciometr turbína je GPIO pin 34 (ADC1 CH6)
const int cJoyCenterX = 1847; //klidová poloha joy, nastaveno konkrétně na tomto kousku
const int cJoyCenterY = 1881; //klidová poloha joy, nastaveno konkrétně na tomto kousku
const int cJoyMin = 0;
const int cJoyMax = 4095;
const int cJoyDZ = 20; //mrtvá zóna ve středu joysticku, na malé hodnoty nereaguj
const int cJoyOkraj = 0; //oříznutí okrajových (maximálních) hodnot na výstupu, pokud chci (zatím nechci)
const int cPotMin = 0;
const int cPotMax = 4095;
const int cRefresh = 100; //počet milisekund mezi vysíláním

typedef struct {
  int16_t x; //osa x
  int16_t y; //osa x
  int16_t t; //turbína
} ESPNPacket;
ESPNPacket data;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init selhalo");
    return;
  }

  esp_now_register_send_cb(onSent);
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

}

void loop() {
  int NDx = 0;
  int NDy = 0;
  int NDt = 0;
  
  prectiJoy(NDx,NDy);
  prectiPot(NDt);
  Serial.printf("x:%d|y:%d|t:%d\n", NDx, NDy, NDt);
  data.x=NDx;
  data.y=NDy;
  data.t=NDt;

  delay(cRefresh);
  esp_now_send(receiverMac, (uint8_t *)&data, sizeof(data));

}

void prectiJoy(int &NDx, int &NDy) {

  int joyX = analogRead(cJoyXpin);
  int joyY = analogRead(cJoyYpin);

  NDx = osetriJoy(joyX, cJoyCenterX);
  NDy = -osetriJoy(joyY, cJoyCenterY);//otáčím znaménko, protože na joysticku je + směrem dolů

}

void prectiPot(int &NDt) {

  int potT = analogRead(cPotTur);
  
  NDt = osetriPot(potT);
  
}


int osetriJoy(int joyRaw, int center) {
//Normalizuje hodnoty z joysticku na rozsah +/-1000
//Ošetří mrtvou zónu, korekci středu a okraje
int val = 0;
int diff = 0;
  
  diff = joyRaw - center;

  // deadzone na RAW
  if (abs(diff) < cJoyDZ) return 0;

  if (diff > 0) {
    val = map(diff, 0, cJoyMax - center, 0, 1000);
  } else {
    val = map(diff, cJoyMin - center, 0, -1000, 0);
  }

  // deadzone
  if (abs(val) < cJoyDZ) val = 0;

  val = constrain(val, -1000+cJoyOkraj, 1000-cJoyOkraj);

  return val;
}

int osetriPot(int potRaw) {
//Normalizuje hodnoty z potenciometru na rozsah 0-1000

int val = 0;
  
  val = map(potRaw,cPotMin,cPotMax, 0, 1000);
  
  val = constrain(val, cPotMin, cPotMax);

  return val;
}
