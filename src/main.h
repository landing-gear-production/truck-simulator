#include <Arduino.h>
#include <SPI.h>
#include "utils.h"
#include "data.h"
#include "j1939.h"
#include "driver/twai.h"
#include "USBHIDGamepad.h"
#include "USBHIDVehicleGamepad.h"
#include "MovingAverage.h"
#include <Adafruit_VL6180X.h>
#include <Wire.h>
// #include <WiFi.h>
// #include <SPIFFS.h>
// #include <FFat.h>
// #include <AsyncTCP.h>
// #include <ESPAsyncWebServer.h>

#define CAN_TX_PIN GPIO_NUM_18
#define CAN_RX_PIN GPIO_NUM_17
#define LED_PIN 38

#define LIDAR_SDA 1
#define LIDAR_SCL 2

#define ACCELERATOR_MAX_VALUE 255
#define BRAKE_MAX_VALUE 255
#define STEERING_RANGE 780
#define ACCELERATOR_PEDAL_PIN 15
#define TRANSMISSION_SHIFTER_PIN 16

#define HORN_PIN 10
#define IGNITION_PIN 11
#define START_PIN 12
#define IGNITION_OUTPUT_PIN 13

#define MIN_RAW_ACCELERATOR 395
#define MAX_RAW_ACCELERATOR 1680
#define MIN_RAW_BRAKE 55
#define MAX_RAW_BRAKE 90

void onData(twai_message_t *message);
void setupCAN();
unsigned long lastGamepadUpdate = millis();
unsigned long lastPrint = millis();
unsigned long PRINT_PERIOD = 100;
unsigned long UPDATE_PERIOD = 10;
MovingAverage <uint16_t, 64> rawAcceleratorPedal;
MovingAverage <uint16_t, 64> rawTransmissionShifter;
MovingAverage <uint8_t, 64> rawBrakePedal;
float lastSteering[2];
uint8_t steeringSamples;
bool steeringInitialized;
float smoothedAcceleratorPedal;
float smoothedTransmissionShifter;

uint32_t hornMessage;
void setHornLast();
unsigned long lastHorn;
unsigned long hornLength = 500;

bool engine;
bool lastEngine;
uint32_t engineMessage;

bool ignition;

TwoWire lidarI2C = TwoWire(0);
Adafruit_VL6180X brakeSensor;

float acceleratorPedalAlpha = 0.05;
float transmissionShifterAlpha = 0.01;

static VehicleState oldState;
static VehicleState state;

static GameState gameState;
bool initValues = false;
static bool started = false, receivingData = false, connected;

USBHIDVehicleGamepad gamepad;

void errorLoop();
void canTask(void *args);

// const char *WIFI_SSID = "BLDG269 Fios";
// const char *WIFI_PASS = "fish1234";
// void initWiFi();

// AsyncWebServer server(HTTP_PORT);
// #define HTTP_PORT 80