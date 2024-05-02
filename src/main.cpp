#include "main.h"

void setup()
{
  Serial.begin(115200);
  lidarI2C.begin(LIDAR_SDA, LIDAR_SCL);
  gamepad.begin();
  USB.productName("Freightliner Cascadia");
  USB.begin();

  initSPIFFS();
  initWiFi();

  initWebSocket();
  initWebServer();

  pinMode(HORN_PIN, INPUT_PULLDOWN);
  pinMode(IGNITION_PIN, INPUT_PULLDOWN);
  pinMode(START_PIN, INPUT_PULLDOWN);

  attachInterrupt(digitalPinToInterrupt(HORN_PIN), setHorn, CHANGE);
  attachInterrupt(digitalPinToInterrupt(START_PIN), setEngineLast, RISING);
  attachInterrupt(digitalPinToInterrupt(IGNITION_PIN), setEngineLast, FALLING);

  xTaskCreatePinnedToCore(canTask, "canTask", 4096, NULL, 10, NULL, 1);
  xTaskCreatePinnedToCore(brakeTask, "brakeTask", 4096, NULL, 1, NULL, 1);
}

void loop()
{
  rawTransmissionShifter.add(analogRead(TRANSMISSION_SHIFTER_PIN));

  rawAcceleratorPedal.add(analogRead(ACCELERATOR_PEDAL_PIN));
  long constrainedAcceleratorPedalPosition = constrain(rawAcceleratorPedal.get(), minAccelerator, maxAccelerator);
  state.acceleratorPedalPosition = map(constrainedAcceleratorPedalPosition, minAccelerator, maxAccelerator, 0, 100);

  // printf("%d\n", rawBrakePedal.get());
  long constrainedBrakePedalPosition = constrain(rawBrakePedal.get(), minBrake, maxBrake);
  state.brakePedalPosition = 100 - map(constrainedBrakePedalPosition, minBrake, maxBrake, 0, 100);
  // printf("Brake pedal distance: %d\n", state.brakePedalPosition);

  uint16_t shifter = rawTransmissionShifter.get();
  if (shifter >= reverseThreshold)
  {
    state.transmission = TransmissionState::Reverse;
  }
  else if (shifter >= lowThreshold)
  {
    state.transmission = TransmissionState::Low;
  }
  else if (shifter >= driveThreshold)
  {
    state.transmission = TransmissionState::Drive;
  }
  else
  {
    state.transmission = TransmissionState::Neutral;
  }

  hornMessage = horn ? 0x10 : 0x00;

  engineMessage = 0x00;
  auto now = millis();
  if (now - lastEngine <= engineLength)
  {
    engineMessage = 0x20;
  }

  memcpy(&oldState, &state, sizeof(VehicleState));
  neopixelWrite(LED_PIN, (!started || !receivingData) ? 127 : 0, receivingData ? 127 : 0, connected ? 127 : 0);

  double constrainedScaledSteeringAngle = constrain((state.steeringWheelAngle + steeringOffset) * steeringScale, -steeringRange, steeringRange);
  gameState.steering = static_cast<int16_t>(map(static_cast<long>(constrainedScaledSteeringAngle), -steeringRange, steeringRange, -32767, 32767));
  gameState.accelerator = static_cast<int16_t>(map(state.acceleratorPedalPosition, 0, 100, 0, 32767));
  gameState.brake = static_cast<int16_t>(map(state.brakePedalPosition, 0, 100, 0, 32767));
  gameState.buttons = state.transmission | hornMessage | engineMessage;

  if (now - lastPrint >= PRINT_PERIOD)
  {
    ws.cleanupClients();

    if (connectedClients > 0)
      notifyClients();

    lastPrint = now;
  }

  if (now - lastGamepadUpdate >= UPDATE_PERIOD)
  {
    gamepad.send(
        0, 0,
        gameState.steering,
        0,
        gameState.accelerator,
        gameState.brake,
        0, gameState.buttons);
    
    lastGamepadUpdate = now;
  }

  // if (now - lastPrint > PRINT_PERIOD) {
  // printf("Steering: %.2f, %ld, %ld\n", state.steeringWheelAngle, static_cast<int16_t>(state.steeringWheelAngle), gameState.steering);
  // printf("accelerator: %d, brake: %d, steering: %d\n", gameAccelerator, gameBrake, gameSteeringAngle);
  // printf("Brake: %d, Accelerator: %d, Steering: %f, Current Gear: %d\n", state.brakePedalPosition, state.acceleratorPedalPosition, state.steeringWheelAngle, state.transmission);
  // lastPrint = now;
  // }
  // }
}

void errorLoop()
{
  while (true)
  {
    neopixelWrite(LED_PIN, 127, 0, 0);
    delay(100);
    neopixelWrite(LED_PIN, 0, 0, 0);
    delay(100);
  }
}

void initSPIFFS()
{
  if (!SPIFFS.begin())
  {
    Serial.println("Cannot mount SPIFFS volume...");
    errorLoop();
    startupError = true;
  }

  auto file = SPIFFS.open("/wifi.txt", FILE_READ);
  if (!file)
  {
    Serial.println("Cannot open wifi.txt file...");
    errorLoop();
    startupError = true;
  }

  auto lines = split(file.readStringUntil('\n').c_str(), ":");
  file.close();

  ssid = lines[0];
  password = lines[1];

  auto configFile = SPIFFS.open("/config.json", FILE_READ);
  std::string json = configFile.readString().c_str();

  JsonDocument document;
  if (!deserializeJson(document, json.c_str()))
  {
    steeringRange = document["steering_range"];
    steeringScale = document["steering_scale"];
    minBrake = document["brake_min"];
    maxBrake = document["brake_max"];
    minAccelerator = document["accelerator_min"];
    maxAccelerator = document["accelerator_max"];
    reverseThreshold = document["reverse_threshold"];
    driveThreshold = document["drive_threshold"];
    lowThreshold = document["low_threshold"];
    steeringOffset = document["steering_offset"];
  }
}

void initWiFi()
{
  WiFi.softAP(ssid.c_str(), password.c_str());

  if (!MDNS.begin("cascadia"))
  {
    Serial.println("Error setting up MDNS responder!");
    errorLoop();
    startupError = true;
  }
}

String templateProcessor(const String &var)
{
  if (var == "IP_ADDRESS")
  {
    return WiFi.localIP().toString();
  }
  return String();
}

void onRootRequest(AsyncWebServerRequest *request)
{
  request->send(SPIFFS, "/index.html", "text/html", false, templateProcessor);
}

void initWebServer()
{
  server.on("/", onRootRequest);
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
  server.begin();
}

void initWebSocket()
{
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void notifyClients()
{
  JsonDocument doc;
  doc["type"] = "data";
  doc["input"]["steering"] = constrain((state.steeringWheelAngle + steeringOffset) * steeringScale, -steeringRange, steeringRange);
  doc["input"]["accelerator"] = state.acceleratorPedalPosition;
  doc["input"]["brake"] = state.brakePedalPosition;
  doc["input"]["transmission"] = state.transmission;
  doc["input"]["horn"] = hornMessage;
  doc["input"]["ignition"] = engineMessage;
  doc["sensors"]["brake"] = rawBrakePedal.get();
  doc["sensors"]["accelerator"] = rawAcceleratorPedal.get();
  doc["sensors"]["transmission"] = rawTransmissionShifter.get();
  doc["sensors"]["steering"] = state.steeringWheelAngle;
  doc["sensors"]["horn"] = horn;
  doc["errors"]["brake"] = brakeSensorError;
  doc["ranges"]["brake_min"] = minBrake;
  doc["ranges"]["brake_max"] = maxBrake;
  doc["ranges"]["accelerator_min"] = minAccelerator;
  doc["ranges"]["accelerator_max"] = maxAccelerator;
  doc["ranges"]["steering_scale"] = steeringScale;
  doc["ranges"]["steering_range"] = steeringRange;
  doc["ranges"]["reverse_threshold"] = reverseThreshold;
  doc["ranges"]["drive_threshold"] = driveThreshold;
  doc["ranges"]["low_threshold"] = lowThreshold;
  doc["ranges"]["steering_offset"] = steeringOffset;
  doc["wifi"]["ssid"] = ssid;
  doc["wifi"]["password"] = password;

  std::string data;
  serializeJson(doc, data);
  ws.textAll(data.c_str());
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  printf("Updated\n");
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    JsonDocument json;
    DeserializationError err = deserializeJson(json, data);
    if (err)
    {
      Serial.print(F("deserializeJson() failed with code "));
      Serial.println(err.c_str());
      return;
    }

    const char *action = json["action"];
    if (strcmp(action, "calibrateBrakes") == 0)
    {
    }
    else if (strcmp(action, "calibrateAccelerator") == 0)
    {
    }
    else if (strcmp(action, "setRanges") == 0)
    {
      steeringRange = json["data"]["steering_range"].as<long>();
      steeringScale = json["data"]["steering_scale"].as<double>();
      steeringOffset = json["data"]["steering_offset"].as<double>();
      minBrake = json["data"]["brake_min"].as<long>();
      maxBrake = json["data"]["brake_max"].as<long>();
      minAccelerator = json["data"]["accelerator_min"].as<long>();
      maxAccelerator = json["data"]["accelerator_max"].as<long>();
      reverseThreshold = json["data"]["reverse_threshold"].as<long>();
      driveThreshold = json["data"]["drive_threshold"].as<long>();
      lowThreshold = json["data"]["low_threshold"].as<long>();

      auto file = SPIFFS.open("/config.json", FILE_WRITE);
      if (file)
      {
        serializeJson(json["data"], file);
        file.close();
      }
    }
    else if (strcmp(action, "setWiFi") == 0)
    {
      Serial.printf("Setting WiFi\n");
      ssid = json["data"]["ssid"].as<std::string>();
      password = json["data"]["password"].as<std::string>();

      auto file = SPIFFS.open("/wifi.txt", FILE_WRITE);
      if (file)
      {
        file.printf("%s:%s", ssid.c_str(), password.c_str());
        file.close();
      }
    }
    else if (strcmp(action, "reboot") == 0)
    {
      ESP.restart();
    }
  }
}

void setHorn()
{
  horn = !digitalRead(HORN_PIN);
}

void setEngineLast()
{
  if (lastEngine + engineBufferLength <= millis())
  {
    lastEngine = millis();
  }
}

void onEvent(
    AsyncWebSocket *server,
    AsyncWebSocketClient *client,
    AwsEventType type,
    void *arg,
    uint8_t *data,
    size_t len)
{

  switch (type)
  {
  case WS_EVT_CONNECT:
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    connectedClients++;
    break;
  case WS_EVT_DISCONNECT:
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
    connectedClients--;
    break;
  case WS_EVT_DATA:
    handleWebSocketMessage(arg, data, len);
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
  }
}

void setupCAN()
{
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  // Install TWAI driver
  if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK)
  {
    printf("Driver installed\n");
    started = true;
  }
  else
  {
    printf("Failed to install driver\n");
    started = false;
    return;
  }

  // Start TWAI driver
  if (twai_start() == ESP_OK)
  {
    printf("Driver started\n");
    started = true;
  }
  else
  {
    printf("Failed to start driver\n");
    started = false;
    return;
  }

  neopixelWrite(LED_PIN, !started ? 127 : 0, receivingData ? 127 : 0, 0);
}

SteeringDirection getSteeringDirection(float last, float current)
{
  if (last - current < 0)
    return SteeringDirection::Increasing;
  else if (last - current > 0)
    return SteeringDirection::Decreasing;
  return SteeringDirection::DirectionUnknown;
}

void onData(twai_message_t *message)
{
  // all J1939 frames are extended
  if (!message->extd)
    return;

  J1939Header header = parseHeader(message->identifier);
  char gear;
  uint16_t rawAngle;
  double radians, degs;
  bool changed;

  // printf("%d\t", header.pgn);
  // for (int i = 0; i < message->data_length_code; i++)
  //   printf("%02x ", message->data[i]);
  // printf("\n");

  switch (header.pgn)
  {
  case 61441:
    // printf("%d\t", message->identifier);
    // for (int i = 0; i < message->data_length_code; i++)
    //   printf("%02x ", message->data[i]);
    // printf("\n");
    // brake pedal position - too inconsistent on the Cascadia when brake air pressure is low
    // if (message->identifier == 0x18f0010b)
    //   state.brakePedalPosition = message->data[1];
    break;
  case 61443:
    // accelerator pedal position - too inconsistent on the Cascadia
    // state.acceleratorPedalPosition = message->data[1];
    break;
  case 61445:
    // extract transmission information - too inconsistent on the Cascadia
    // printf("%d\t", header.pgn);
    // for (int i = 0; i < message->data_length_code; i++)
    //   printf("%02x ", message->data[i]);
    // printf("\n");
    // gear = message->data[5];
    // switch (gear) {
    //   case 'R':
    //     state.transmission = Reverse;
    //     break;
    //   case 'N':
    //     state.transmission = Neutral;
    //     break;
    //   case 'D':
    //     state.transmission = Drive;
    //     break;
    //   case 'L':
    //     state.transmission = Low;
    //     break;
    // }
    // break;
  case 61449:
    rawAngle = (static_cast<uint16_t>(message->data[1]) << 8) | message->data[0];
    radians = static_cast<double>(rawAngle) / 1024.0 - 31.374;
    degs = radians * 180.0 / PI;

    // all of this hackiness due to rollover issues in the Cascadia steering reports
    // data jumps at around 740 degrees (maybe a parsing issue)?
    if (degs >= -steeringRange && degs <= steeringRange)
    {
      // if (state.steeringWheelAngle == 0 )
      if (!steeringInitialized)
      {
        lastSteering[steeringSamples] = degs;
        steeringSamples++;
        if (steeringSamples == 2)
        {
          steeringInitialized = true;
          state.steeringWheelAngle = degs;
        }
      }
      else
      {
        SteeringDirection lastDirection = getSteeringDirection(lastSteering[0], lastSteering[1]);
        SteeringDirection nextDirection = getSteeringDirection(lastSteering[1], degs);

        if (nextDirection != SteeringDirection::DirectionUnknown && lastDirection != nextDirection)
        {
          if (abs(lastSteering[1] - degs) < steeringRange)
          {
            state.steeringWheelAngle = degs;
            lastSteering[0] = lastSteering[1];
            lastSteering[1] = degs;
            // lastSteeringUpdate = millis();
          }
        }
        else
        {
          state.steeringWheelAngle = degs;
          lastSteering[0] = lastSteering[1];
          lastSteering[1] = degs;
          // lastSteeringUpdate = millis();
        }
      }
    }

    // printf("Raw Degrees: %.2f, Final Angle: %.2f\n", degs, state.steeringWheelAngle);
    break;
  }
}

void canTask(void *args)
{
  setupCAN();

  auto now = millis();

  for (;;)
  {
    now = millis();
    // oldFeedbackData = feedbackData;
    twai_message_t message;

    if (twai_receive(&message, 0) == ESP_OK)
    {
      receivingData = true;
      onData(&message);
    }
    else
    {
      // printf("Failed to receive message\n");
      receivingData = false;
    }

    // printf("Update Length: %lu\n", now - lastSteeringUpdate);
    // lastSteeringUpdate = now;

    delay(1);
  }

  vTaskDelete(NULL);
}

void brakeTask(void *args)
{
  if (!brakeSensor.begin(&lidarI2C))
  {
    printf("Failed to init VL6180X\n");
    startupError = true;
    brakeSensorError = true;
  }
  else
    printf("Brake sensor initialized\n");

  for (;;)
  {
    rawBrakePedal.add(brakeSensor.readRange());
    // printf("Brake pedal distance: %d, error; %s\n", rawBrakePedal.get(), brakeSensorError ? "true" : "false");
    // delay(95);
    delay(10);
  }

  vTaskDelete(NULL);
}