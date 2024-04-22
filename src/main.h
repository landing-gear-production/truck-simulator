#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include "utils.h"
#include "data.h"
#include "j1939.h"
#include "driver/twai.h"
#include "USB.h"
#include "USBHIDGamepad.h"
#include "USBHIDVehicleGamepad.h"
#include "MovingAverage.h"
#include <Adafruit_VL6180X.h>
#include <Wire.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <FFat.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>

#define CAN_TX_PIN GPIO_NUM_18
#define CAN_RX_PIN GPIO_NUM_17
#define LED_PIN 38

#define LIDAR_SDA 1
#define LIDAR_SCL 2

#define ACCELERATOR_PEDAL_PIN 15
#define TRANSMISSION_SHIFTER_PIN 16

#define HORN_PIN 10
#define IGNITION_PIN 9
#define START_PIN 12
#define IGNITION_OUTPUT_PIN 13

#define HTTP_PORT 80

long steeringRange, minBrake, maxBrake, minAccelerator, maxAccelerator, reverseThreshold, driveThreshold, lowThreshold;
double steeringScale = 1.0f;

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

bool horn = false;
uint32_t hornMessage;
void setHorn();

uint32_t engineMessage;
void setEngineLast();
unsigned long lastEngine;
unsigned long engineLength = 50;
unsigned long engineBufferLength = 800;

bool ignition;
bool lastIgnition;

TwoWire lidarI2C = TwoWire(0);
Adafruit_VL6180X brakeSensor;

float acceleratorPedalAlpha = 0.05;
float transmissionShifterAlpha = 0.01;

static VehicleState oldState;
static VehicleState state;

static GameState gameState;
bool initValues = false;
static bool started = false, receivingData = false, connected;

bool startupError = false;
bool brakeSensorError = false;
bool wifiConnected = false;

USBHIDVehicleGamepad gamepad;

void errorLoop();
void canTask(void *args);
void brakeTask(void *args);

std::string ssid;
std::string password;
void initSPIFFS();
void initWiFi();
void initWebServer();
void initWebSocket();

AsyncWebServer server(HTTP_PORT);
AsyncWebSocket ws("/ws");
void onEvent(AsyncWebSocket       *server,
             AsyncWebSocketClient *client,
             AwsEventType          type,
             void                 *arg,
             uint8_t              *data,
             size_t                len);

void notifyClients();

inline std::vector<std::string> split(std::string s, std::string delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
        token = s.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back (token);
    }

    res.push_back (s.substr (pos_start));
    return res;
}