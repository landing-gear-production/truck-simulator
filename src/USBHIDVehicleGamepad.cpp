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
#include "USBHID.h"

#if CONFIG_TINYUSB_HID_ENABLED

#include "USBHIDVehicleGamepad.h"

static const uint8_t report_descriptor[] = {
  TUD_HID_REPORT_DESC_VEHICLE_GAMEPAD(HID_REPORT_ID(HID_REPORT_ID_GAMEPAD))
};

USBHIDVehicleGamepad::USBHIDVehicleGamepad(): hid(), _x(0), _y(0), _z(0), _rz(0), _rx(0), _ry(0), _hat(0), _buttons(0){
    static bool initialized = false;
    if(!initialized){
        initialized = true;
        hid.addDevice(this, sizeof(report_descriptor));
    }
}

uint16_t USBHIDVehicleGamepad::_onGetDescriptor(uint8_t* dst){
    memcpy(dst, report_descriptor, sizeof(report_descriptor));
    return sizeof(report_descriptor);
}

void USBHIDVehicleGamepad::begin(){
    hid.begin();
}

void USBHIDVehicleGamepad::end(){

}

bool USBHIDVehicleGamepad::write(){
    hid_vehicle_gamepad_report_t report = {
        .x       = _x,
        .y       = _y,
        .z       = _z,
        .rz      = _rz,
        .rx      = _rx,
        .ry      = _ry,
        .hat     = _hat,
        .buttons = _buttons
    };
    return hid.SendReport(HID_REPORT_ID_GAMEPAD, &report, sizeof(report));
}

bool USBHIDVehicleGamepad::leftStick(int16_t x, int16_t y){
    _x = x;
    _y = y;
    return write();
}

bool USBHIDVehicleGamepad::rightStick(int16_t z, int16_t rz){
    _z = z;
    _rz = rz;
    return write();
}

bool USBHIDVehicleGamepad::leftTrigger(int16_t rx){
    _rx = rx;
    return write();
}

bool USBHIDVehicleGamepad::rightTrigger(int16_t ry){
    _ry = ry;
    return write();
}

bool USBHIDVehicleGamepad::hat(uint8_t hat){
    if(hat > 9){
        return false;
    }
    _hat = hat;
    return write();
}

bool USBHIDVehicleGamepad::pressButton(uint8_t button){
    if(button > 31){
        return false;
    }
    _buttons |= (1 << button);
    return write();
}

bool USBHIDVehicleGamepad::releaseButton(uint8_t button){
    if(button > 31){
        return false;
    }
    _buttons &= ~(1 << button);
    return write();
}

bool USBHIDVehicleGamepad::send(int16_t x, int16_t y, int16_t z, int16_t rz, int16_t rx, int16_t ry, uint8_t hat, uint32_t buttons){
    if(hat > 9){
        return false;
    }
    _x = x;
    _y = y;
    _z = z;
    _rz = rz;
    _rx = rx;
    _ry = ry;
    _hat = hat;
    _buttons = buttons;
    return write();
}


#endif /* CONFIG_TINYUSB_HID_ENABLED */
