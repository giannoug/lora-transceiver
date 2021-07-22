#include <Arduino.h>
#include <LoRa.h>

#ifdef IS_NODE
#include "Button2.h"
#endif

#ifdef HAS_POWER_MGMT
#include <Wire.h>
#include "axp20x.h"
#endif

#ifdef HAS_SCREEN
#include "SSD1306Wire.h"
#endif

#ifdef IS_NODE
#define BUTTON_PIN 38
Button2 button = Button2(BUTTON_PIN);
#endif

#ifdef HAS_POWER_MGMT
#define I2C_SDA 21
#define I2C_SCL 22
#define PMU_IRQ 35
AXP20X_Class axp;
#endif

#ifdef HAS_SCREEN
#define SSD1306_ADDRESS 0x3C
SSD1306Wire *display;
#endif

int messageCount = 0;

void sendMessage(String outgoing)
{
  Serial.print("Sending message: ");
  Serial.println(outgoing);

#ifdef HAS_SCREEN
  // display->println("S: " + outgoing);
#endif

  LoRa.beginPacket();   // start packet
  LoRa.print(outgoing); // add payload
  LoRa.endPacket();     // finish packet and send it
  messageCount++;       // increment message ID
}

void onReceive(int packetSize)
{
  if (packetSize == 0)
    return;

  String incoming = "";

  while (LoRa.available())
  {
    incoming += (char)LoRa.read();
  }

  Serial.println("Message: " + incoming);
  Serial.println("RSSI: " + String(LoRa.packetRssi()));
  Serial.println("SNR: " + String(LoRa.packetSnr()));
  Serial.println("Packet Frequency Error: " + String(LoRa.packetFrequencyError()));

#ifdef HAS_SCREEN
  display->println("R: " + incoming + "\nrssi= " + LoRa.packetRssi() + ", snr=" + LoRa.packetSnr());
#endif

#ifdef IS_BASE
  sendMessage(incoming);
#endif
}

#ifdef IS_NODE
void releasedButtonHandler(Button2 &btn)
{
  Serial.printf("Button released: %d\n", btn.wasPressedFor());
  sendMessage(String(btn.wasPressedFor()));
}
#endif

void setup()
{
  Serial.begin(9600);
  Serial.println("LoRa transceiver sketch");

#ifdef IS_NODE
  button.setReleasedHandler(releasedButtonHandler);
#endif

#ifdef HAS_POWER_MGMT
  Wire.begin(I2C_SDA, I2C_SCL);

  if (!axp.begin(Wire, AXP192_SLAVE_ADDRESS))
  {
    Serial.println("AXP192 Begin PASS");

    axp.setPowerOutPut(AXP192_LDO2, AXP202_ON);  // LORA radio
    axp.setPowerOutPut(AXP192_LDO3, AXP202_OFF); // GPS main power
    axp.setPowerOutPut(AXP192_DCDC2, AXP202_ON);
    axp.setPowerOutPut(AXP192_EXTEN, AXP202_ON);
    axp.setPowerOutPut(AXP192_DCDC1, AXP202_ON);
    axp.setDCDC1Voltage(3300);

    pinMode(PMU_IRQ, INPUT_PULLUP);

    attachInterrupt(
        PMU_IRQ, []
        { Serial.println("PMU irq!"); },
        FALLING);

    axp.adc1Enable(AXP202_BATT_CUR_ADC1, 1);
    axp.enableIRQ(AXP202_VBUS_REMOVED_IRQ | AXP202_VBUS_CONNECT_IRQ | AXP202_BATT_REMOVED_IRQ | AXP202_BATT_CONNECT_IRQ, 1);
    axp.clearIRQ();
  }
  else
  {
    Serial.println("AXP192 Begin FAIL");
    while (true)
      ;
  }
#endif

#ifdef HAS_SCREEN
  display = new SSD1306Wire(SSD1306_ADDRESS, I2C_SDA, I2C_SCL);
  display->init();
  display->flipScreenVertically();
  display->setLogBuffer(5, 30);
  display->println("Hello.");
#endif

  LoRa.setPins(NSS, RST, DIO0);

  if (!LoRa.begin(868E6))
  {
    Serial.println("LoRa init failed. Check your connections.");
    while (true)
      ;
  }

  LoRa.setSyncWord(0x12);
  LoRa.setSpreadingFactor(12);
  Serial.println("LoRa init succeeded.");
}

void loop()
{
#ifdef IS_NODE
  button.loop();
#endif

#ifdef HAS_SCREEN
  display->clear();
  display->drawLogBuffer(0, 0);
  display->display();
#endif

  onReceive(LoRa.parsePacket());
}