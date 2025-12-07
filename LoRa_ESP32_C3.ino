// ESP32-C3 — RFM95W + INA219 + DS18B20
// Wiring:
// RFM95W      ESP32-C3
// 3.3V        3.3V
// GND         GND
// NSS         GPIO10
// SCK         GPIO6
// MOSI        GPIO7
// MISO        GPIO2
// DIO0        GPIO3
// RST         GPIO4
//
// INA219      GPIO0 (SDA), GPIO1 (SCL)
//
// DS18B20 DQ  GPIO5 (with 4.7k pullup)
//
// MOSFET Gate GPIO8 (P-channel; LOW = sensors ON)

#include <RHReliableDatagram.h>
#include <RH_RF95.h>            // Low-level RFM95 driver
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define MOSFET_PIN    5       // LOW at boot → sensors ON
#define ONE_WIRE_BUS  8

// LoRa pins (RFM95W on ESP32-C3)
#define LORA_SS       10      // NSS
#define LORA_RST      4
#define LORA_DIO0     3

// === MUST MATCH YOUR GATEWAY ===
#define MY_ADDRESS      100    // Your device's address (1-253)
#define GATEWAY_ADDRESS 254   // Gateway address
#define LORA_FREQ       868.1 // 868.1 MHz
#define SLEEP_MINUTES   15
// =================================
Adafruit_INA219 ina219(0x40);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
RH_RF95 rf95(LORA_SS, LORA_DIO0);
RHReliableDatagram manager(rf95, MY_ADDRESS);

void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for serial monitor
  // Sensors ON (GPIO 8 = LOW at boot)
  pinMode(MOSFET_PIN, OUTPUT);
  digitalWrite(MOSFET_PIN, LOW);
  delay(500);
  SPI.begin(6, 2, 7, 10);   // SCK=6, MISO=2, MOSI=7, SS=10
  // I2C on remapped pins
  Wire.begin(0, 1);  // SDA=GPIO0, SCL=GPIO1
  ina219.begin();
  // DS18B20
  sensors.begin();
  // LoRa init
  pinMode(LORA_RST, OUTPUT);
  digitalWrite(LORA_RST, LOW);
  delay(10);
  digitalWrite(LORA_RST, HIGH);
  delay(10);

  if (!manager.init()) {
    digitalWrite(MOSFET_PIN, HIGH);
    esp_sleep_enable_timer_wakeup(SLEEP_MINUTES * 60ULL * 1000000ULL);
    esp_deep_sleep_start();
  }
  rf95.setFrequency(LORA_FREQ);
  rf95.setModemConfig(RH_RF95::Bw125Cr45Sf128);  // SF7, CR4/5, BW125kHz
  rf95.setTxPower(14, false);                    // Same as gateway
  // Read sensors
  float voltage = ina219.getBusVoltage_V() + ina219.getShuntVoltage_mV() / 1000.0f;
  sensors.requestTemperatures();
  delay(800);
  float temp = sensors.getTempCByIndex(0);
  if (temp == DEVICE_DISCONNECTED_C) temp = -127.0;
  // ←←← THIS IS THE EXACT FORMAT YOUR GATEWAY LOVES ←←←
  char payload[64];
  snprintf(payload, sizeof(payload),"{\"V\":%.2f,\"T\":%.1f}", voltage, temp);
  uint8_t result = manager.sendtoWait((uint8_t*)payload, strlen(payload), GATEWAY_ADDRESS);
  Serial.print("Sent: ");
  Serial.println(payload);
  if (!result) {
    Serial.println("SUCCESS – packet sent + ACK received");
  } else {
    Serial.print("No ACK, error 0x");
    Serial.println(result, HEX);
  // packet was still transmitted – you will see it on the gateway log below
  }
  // Sensors OFF
  digitalWrite(MOSFET_PIN, HIGH);
  // 5-minute deep sleep
  esp_sleep_enable_timer_wakeup(SLEEP_MINUTES * 60ULL * 1000000ULL);
  esp_deep_sleep_start();
}
void loop() {
  // Never reached
}
