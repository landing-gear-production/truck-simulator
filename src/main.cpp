#include "main.h"

void setup() {
  Serial.begin(115200);
  lidarI2C.begin(LIDAR_SDA, LIDAR_SCL);
  gamepad.begin();
  if (!brakeSensor.begin(&lidarI2C)) {
    printf("Failed to init VL6180X\n");
    errorLoop();
  }
  // if(!FFat.begin(true)){
  //   Serial.println("Mount Failed");
  //   if(FFat.format(FFAT_WIPE_FULL)){
  //       Serial.println("Format Success");
  //     } else {
  //       Serial.println("Format Failed");
  //     }
  //   errorLoop();
  //   return;
  // }
  // initWiFi();
  xTaskCreatePinnedToCore(canTask, "canTask", 4096, NULL, 10, NULL, 1);
}

void loop() {
  rawTransmissionShifter.add(analogRead(TRANSMISSION_SHIFTER_PIN));

  rawAcceleratorPedal.add(analogRead(ACCELERATOR_PEDAL_PIN));
  long constrainedAcceleratorPedalPosition = constrain(rawAcceleratorPedal.get(), MIN_RAW_ACCELERATOR, MAX_RAW_ACCELERATOR);
  state.acceleratorPedalPosition = map(constrainedAcceleratorPedalPosition, MIN_RAW_ACCELERATOR, MAX_RAW_ACCELERATOR, 0, 100);

  rawBrakePedal.add(brakeSensor.readRange());
  // printf("%d\n", rawBrakePedal.get());
  long constrainedBrakePedalPosition = constrain(rawBrakePedal.get(), MIN_RAW_BRAKE, MAX_RAW_BRAKE);
  state.brakePedalPosition = 100 - map(constrainedBrakePedalPosition, MIN_RAW_BRAKE, MAX_RAW_BRAKE, 0, 100);
  // printf("Brake pedal distance: %d\n", state.brakePedalPosition);

  uint16_t shifter = rawTransmissionShifter.get();
  if (shifter >= 2200) {
    state.transmission = TransmissionState::Reverse;
  } else if (shifter >= 2000) {
    state.transmission = TransmissionState::Low;
  } else if (shifter >= 1200) {
    state.transmission = TransmissionState::Drive;
  } else {
    state.transmission = TransmissionState::Neutral;
  }

  memcpy(&oldState, &state, sizeof(VehicleState));
  neopixelWrite(LED_PIN, (!started || !receivingData) ? 127 : 0, receivingData ? 127 : 0, connected ? 127 : 0);
  auto now = millis();

  gameState.steering = static_cast<int16_t>(map(static_cast<long>(state.steeringWheelAngle), -STEERING_RANGE, STEERING_RANGE, -32767, 32767));
  gameState.accelerator = static_cast<int16_t>(map(state.acceleratorPedalPosition, 0, 100, 0, 32767));
  gameState.brake = static_cast<int16_t>(map(state.brakePedalPosition, 0, 100, 0, 32767));

  // if (oldState != state) {
    gamepad.send(
      0, 0,
      gameState.steering,
      0,
      gameState.accelerator,
      gameState.brake,
      0, state.transmission
    );

    // if (now - lastPrint > PRINT_PERIOD) {
      // printf("Steering: %.2f, %ld, %ld\n", state.steeringWheelAngle, static_cast<int16_t>(state.steeringWheelAngle), gameState.steering);
    //   printf("accelerator: %d, brake: %d, steering: %d\n", gameAccelerator, gameBrake, gameSteeringAngle);
    //   // printf("Brake: %d, Accelerator: %d, Steering: %f, Current Gear: %d\n", state.brakePedalPosition, state.acceleratorPedalPosition, state.steeringWheelAngle, state.transmission);
      // lastPrint = now;
    // }
  // }
}

void errorLoop() {
  while (true) {
    neopixelWrite(LED_PIN, 127, 0, 0);
    delay(500);
    neopixelWrite(LED_PIN, 0, 0, 0);
    delay(500);
  }
}

// void initWiFi() {
//   WiFi.mode(WIFI_STA);
//   WiFi.begin(WIFI_SSID, WIFI_PASS);
//   Serial.printf("Trying to connect [%s] ", WiFi.macAddress().c_str());
//   while (WiFi.status() != WL_CONNECTED) {
//     Serial.print(".");
//     neopixelWrite(0, 0, 127);
//     delay(500);
//     neopixelWrite(0, 0, 0);
//     delay(500);
//   }
//   Serial.printf(" %s\n", WiFi.localIP().toString().c_str());
// }

// void initWebServer() {
//   server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
//   server.begin();
// }

void setupCAN() {
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  //Install TWAI driver
  if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
    printf("Driver installed\n");
    started = true;
  } else {
    printf("Failed to install driver\n");
    started = false;
    return;
  }

  //Start TWAI driver
  if (twai_start() == ESP_OK) {
    printf("Driver started\n");
    started = true;
  } else {
    printf("Failed to start driver\n");
    started = false;
    return;
  }

  neopixelWrite(LED_PIN, !started ? 127 : 0, receivingData ? 127 : 0, 0);
}

SteeringDirection getSteeringDirection(float last, float current) {
  if (last - current < 0) return SteeringDirection::Increasing;
  else if (last - current > 0) return SteeringDirection::Decreasing;
  return SteeringDirection::DirectionUnknown;
}

void onData(twai_message_t *message) {
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

  switch (header.pgn) {
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
      if (degs >= -STEERING_RANGE && degs <= STEERING_RANGE) {
        // if (state.steeringWheelAngle == 0 )
        if (!steeringInitialized) {
          lastSteering[steeringSamples] = degs;
          steeringSamples++;
          if (steeringSamples == 2) {
            steeringInitialized = true;
            state.steeringWheelAngle = degs;
          }
        }
        else {
          SteeringDirection lastDirection = getSteeringDirection(lastSteering[0], lastSteering[1]);
          SteeringDirection nextDirection = getSteeringDirection (lastSteering[1], degs);

          if (nextDirection != SteeringDirection::DirectionUnknown && lastDirection != nextDirection) {
            if (abs(lastSteering[1] - degs) < STEERING_RANGE) {
              state.steeringWheelAngle = degs;
              lastSteering[0] = lastSteering[1];
              lastSteering[1] = degs;
            }
          }
          else {
            state.steeringWheelAngle = degs;
            lastSteering[0] = lastSteering[1];
            lastSteering[1] = degs;
          }
      }
    }
    break;
  }
}

void canTask(void* args) {
  setupCAN();

  for (;;) {
    // oldFeedbackData = feedbackData;
    twai_message_t message; 

    if (twai_receive(&message, 100) == ESP_OK) {
      receivingData = true;
      onData(&message);
    }
    else {
      // printf("Failed to receive message\n");
      receivingData = false;
    }

    delay(5);
  }

  vTaskDelete(NULL);
}