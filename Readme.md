# Truck Simulator

## Startup Instructions

1. Turn on power tender. This keeps the battery at a healthy voltage (especially to keep steering consistently).
2. Turn on the speaker. Make sure the input is set to Aux and that the 3.5mm cable is plugged into the cable.
3. Turn on the subwoofer.
4. Turn on the amplifier for the Butt Kicker. The volume should be set between 38-40.
5. Turn on the computer and login. The following should startup automatically:
  - ETS2 Telemetry Server
  - Dashboard communication (Node.js process)
  - GPS telemetry website (self hosted)
  - American Truck Simulator
6. Ensure that the controller (ESP32S3) is connected to the computer using the USB port (not the UART port) of the ESP32.
7. Turn on the truck to accessory power, you should see the green light go on when the controller is receiving CAN bus data.
8. To verify inputs, search for "Set up USB game controllers" from the start menu. Click on properties for the device that shows up to see said inputs.
9. Alternatively, you can check inputs directly from the truck. Assuming the controller is on the WiFi network, you can navigate to http://cascadia.local to view the truck inputs directly. This is also a good resource for the co-driver to view when in the passenger seat when resetting the steering wheel.
10. Ensure that the computer is in "do not disturb".

## Drive Instructions

1. Load the "lng_game_2" drive for the long drive of "srt_game_2" for the short drive.
2. Ensure the fake key is in the off position.
3. Ensure that the transmission is in neutral.
4. Ensure that the steering is at 0 degrees. You can view the controller inputs at http://cascadia.local

## WiFi Setup

1. Connect a USB cable to the UART port of the ESP32S3. In the data folder of this repo, update `wifi.txt` with ssid:password (as shown in wifi.txt.example). Once that's done, 

## Calibration

If the brake sensor has been moved since the last operation, you'll need to recalibrate its range:

1. Go to http://cascadia.local
2. Observe the raw value of the brake sensor when no one is pressing it. Set the max value in the brake range to this value.
3. Press down on the brake as hard as you can with your foot. Observe the raw value and set the min value in the brake range to this value.
4. Press update values. The brake values in game should now work.