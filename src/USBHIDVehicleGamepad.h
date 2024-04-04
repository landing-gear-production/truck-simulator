// Copyright 2015-2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once
#include "USBHID.h"
#if CONFIG_TINYUSB_HID_ENABLED

/// Standard Gamepad Buttons Naming from Linux input event codes
/// https://github.com/torvalds/linux/blob/master/include/uapi/linux/input-event-codes.h
#define BUTTON_A       0
#define BUTTON_B       1
#define BUTTON_C       2
#define BUTTON_X       3
#define BUTTON_Y       4
#define BUTTON_Z       5
#define BUTTON_TL      6
#define BUTTON_TR      7
#define BUTTON_TL2     8
#define BUTTON_TR2     9
#define BUTTON_SELECT  10
#define BUTTON_START   11
#define BUTTON_MODE    12
#define BUTTON_THUMBL  13
#define BUTTON_THUMBR  14

#define BUTTON_SOUTH   BUTTON_A
#define BUTTON_EAST    BUTTON_B
#define BUTTON_NORTH   BUTTON_X
#define BUTTON_WEST    BUTTON_Y

/// Standard Gamepad HAT/DPAD Buttons (from Linux input event codes)
#define HAT_CENTER     0
#define HAT_UP         1
#define HAT_UP_RIGHT   2
#define HAT_RIGHT      3
#define HAT_DOWN_RIGHT 4
#define HAT_DOWN       5
#define HAT_DOWN_LEFT  6
#define HAT_LEFT       7
#define HAT_UP_LEFT    8

#define TUD_HID_REPORT_DESC_VEHICLE_GAMEPAD(...) \
  HID_USAGE_PAGE ( HID_USAGE_PAGE_DESKTOP     )                 ,\
  HID_USAGE      ( HID_USAGE_DESKTOP_GAMEPAD  )                 ,\
  HID_COLLECTION ( HID_COLLECTION_APPLICATION )                 ,\
    /* Report ID if any */\
    __VA_ARGS__ \
    /* 16 bit X, Y, Z, Rz, Rx, Ry (min -32767, max 32767 ) */ \
    HID_USAGE_PAGE     ( HID_USAGE_PAGE_DESKTOP                 ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_X                    ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_Y                    ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_Z                    ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_RZ                   ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_RX                   ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_RY                   ) ,\
    HID_LOGICAL_MIN    ( 0x8001                                 ) ,\
    HID_LOGICAL_MAX    ( 0x7fff                                 ) ,\
    HID_REPORT_COUNT   ( 6                                      ) ,\
    HID_REPORT_SIZE    ( 16                                     ) ,\
    HID_INPUT          ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
    /* 8 bit DPad/Hat Button Map  */ \
    HID_USAGE_PAGE     ( HID_USAGE_PAGE_DESKTOP                 ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_HAT_SWITCH           ) ,\
    HID_LOGICAL_MIN    ( 1                                      ) ,\
    HID_LOGICAL_MAX    ( 8                                      ) ,\
    HID_PHYSICAL_MIN   ( 0                                      ) ,\
    HID_PHYSICAL_MAX_N ( 315, 2                                 ) ,\
    HID_REPORT_COUNT   ( 1                                      ) ,\
    HID_REPORT_SIZE    ( 8                                      ) ,\
    HID_INPUT          ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
    /* 32 bit Button Map */ \
    HID_USAGE_PAGE     ( HID_USAGE_PAGE_BUTTON                  ) ,\
    HID_USAGE_MIN      ( 1                                      ) ,\
    HID_USAGE_MAX      ( 32                                     ) ,\
    HID_LOGICAL_MIN    ( 0                                      ) ,\
    HID_LOGICAL_MAX    ( 1                                      ) ,\
    HID_REPORT_COUNT   ( 32                                     ) ,\
    HID_REPORT_SIZE    ( 1                                      ) ,\
    HID_INPUT          ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
  HID_COLLECTION_END \

typedef struct TU_ATTR_PACKED
{
  int16_t  x;         ///< Delta x  movement of left analog-stick
  int16_t  y;         ///< Delta y  movement of left analog-stick
  int16_t  z;         ///< Delta z  movement of right analog-joystick
  int16_t  rz;        ///< Delta Rz movement of right analog-joystick
  int16_t  rx;        ///< Delta Rx movement of analog left trigger
  int16_t  ry;        ///< Delta Ry movement of analog right trigger
  uint8_t hat;       ///< Buttons mask for currently pressed buttons in the DPad/hat
  uint32_t buttons;  ///< Buttons mask for currently pressed buttons
}hid_vehicle_gamepad_report_t;

class USBHIDVehicleGamepad: public USBHIDDevice {
private:
    USBHID hid;
    int16_t  _x;         ///< Delta x  movement of left analog-stick
    int16_t  _y;         ///< Delta y  movement of left analog-stick
    int16_t  _z;         ///< Delta z  movement of right analog-joystick
    int16_t  _rz;        ///< Delta Rz movement of right analog-joystick
    int16_t  _rx;        ///< Delta Rx movement of analog left trigger
    int16_t  _ry;        ///< Delta Ry movement of analog right trigger
    uint8_t _hat;       ///< Buttons mask for currently pressed buttons in the DPad/hat
    uint32_t _buttons;  ///< Buttons mask for currently pressed buttons
    bool write();
public:
    USBHIDVehicleGamepad(void);
    void begin(void);
    void end(void);

    bool leftStick(int16_t x, int16_t y);
    bool rightStick(int16_t z, int16_t rz);

    bool leftTrigger(int16_t rx);
    bool rightTrigger(int16_t ry);

    bool hat(uint8_t hat);

    bool pressButton(uint8_t button);
    bool releaseButton(uint8_t button);

    bool send(int16_t x, int16_t y, int16_t z, int16_t rz, int16_t rx, int16_t ry, uint8_t hat, uint32_t buttons);

    // internal use
    uint16_t _onGetDescriptor(uint8_t* buffer);
};

#endif
