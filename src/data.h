#pragma once
#include <stdint.h>

enum TransmissionState : uint32_t {
  Unknown = 0x00,
  Reverse = 0x01,
  Neutral = 0x02,
  Drive = 0x04,
  Low = 0x08
};

struct GameState {
  int16_t accelerator;
  int16_t brake;
  int16_t steering;
  uint32_t transmission;
};

enum SteeringDirection {
  DirectionUnknown,
  Increasing,
  Decreasing
};

struct VehicleState {
  uint8_t brakePedalPosition;
  uint8_t acceleratorPedalPosition;
  TransmissionState transmission;
  double steeringWheelAngle;
  SteeringDirection steeringDirection;  // hacky BS for the Cascadia

  bool operator==(const VehicleState &other) const {
    return brakePedalPosition == other.brakePedalPosition &&
           acceleratorPedalPosition == other.acceleratorPedalPosition &&
           transmission == other.transmission &&
           static_cast<long>(steeringWheelAngle) == static_cast<long>(other.steeringWheelAngle);
  }

  bool operator!=(const VehicleState &other) const {
    return !(*this == other);
  }
};