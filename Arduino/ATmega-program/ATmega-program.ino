#define DVD // use the DVD setting on the remote
#define WIFI // use Serial connection to Wi-Fi chip

//#define DEBUG // print commands and other debug messages to the serial port

const uint8_t IR_REMOTE = 2;
const uint8_t SPEED_SENSE = 3;
const uint8_t LINE_LED = 4;
const uint8_t SPEED_L = 5;
const uint8_t SPEED_R = 6;
const uint8_t DIRECTION_L = 7;
const uint8_t DIRECTION_R = 8;
const uint8_t LIGHTS = 9;

const uint8_t DATA = 11;
const uint8_t LATCH = 10;
const uint8_t CLOCK = 13;

const uint8_t LIGHT_SENSOR = A0;
const uint8_t LINE_LEFT = A1;
const uint8_t LINE_RIGHT = A2;
const uint8_t BUZZER = A3;

#include "IRLib.h"
#include "Commands.h"
//#include "Motors.hpp"
#include "Buzzer.h"
#include "drive.hpp"
#include "lights.hpp"

Drive drive;
Lights lights;

IRrecv My_Receiver(IR_REMOTE);

IRdecode My_Decoder;
unsigned int Buffer[RAWBUF];

volatile unsigned long speed = 0;

void setup() {
  Serial.begin(115200);
  lights.begin();
  delay(20); while (!Serial); //delay for Leonardo
  My_Receiver.enableIRIn(); // Start the receiver
  My_Decoder.UseExtnBuf(Buffer);

  pinMode(SPEED_SENSE, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(SPEED_SENSE), checkSpeed, RISING);

  Serial.write((1 << 7) | RESET);
  delay(500);
}

#ifdef WIFI
uint8_t serialMessage[2];
boolean messageDone = false;
#endif

void loop() {
#ifdef WIFI
  if (Serial.available() > 0) {
    uint8_t data = Serial.read();
#ifdef DEBUG
    Serial.print("Serial data:\t0x");
    Serial.println(data, HEX);
#endif
    if (data >> 7) {
      serialMessage[0] = data;
      if ((data & ~(1 << 7)) >= 0x06) { // if the packet is only one byte long
        messageDone = true;
      }
    } else if (serialMessage[0] & ~(1 << 7) <= 0x06) {
      serialMessage[1] = data;
      messageDone = true;
    }
  }
  if (messageDone) {
#ifdef DEBUG
    Serial.println("Message is being processed");
#else
    // Serial.write(serialMessage, 2);
#endif
    drive.checkIR(serialMessage[0] & ~(1 << 7));
    lights.checkIR(serialMessage[0] & ~(1 << 7));
    messageDone = false;
  }
#endif
  if (My_Receiver.GetResults(&My_Decoder)) {
    My_Receiver.resume();
    My_Decoder.decode();
#ifdef DEBUG
    Serial.println(My_Decoder.value, HEX);
#elif defined WIFI
    // Serial.write(My_Decoder.value | (1<<7));
#endif
    drive.checkIR(My_Decoder.value);
    lights.checkIR(My_Decoder.value);
    My_Decoder.value = 0;
  }
  drive.refresh();
  lights.refresh(drive.getSpeed());
  Buzzer.refresh();
  unsigned long maxSpeed = 0;
  for (int i = 0; i < 100; i++) {
    if (speed > maxSpeed) {
      maxSpeed = speed;
    }
  }
#ifdef DEBUG
  //Serial.println(maxSpeed);
#endif
}

unsigned long speed_interval = 1;
unsigned long prev_speedISR_micros = 0;

void checkSpeed() {
  unsigned long tmp = micros() - prev_speedISR_micros;
  if (tmp > 5000) {
    speed_interval = tmp;
  }
  prev_speedISR_micros = micros();
}
