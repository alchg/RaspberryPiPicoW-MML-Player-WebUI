# RaspberryPiPicoW-MML-Player-WebUI
For Arduino IDE and Raspberry Pi Pico W. A browser-operated multi-track MML player and editor.
# How to use
1. Connect GPIO 0 and GND to earphones, etc.
2. Correct the SSID and password described at the beginning of the INO file.
3. Upload RaspberryPiPicoW-MML-Player-WebUI.ino with file system in ArduinoIDE.
4. Upload the contents of the data folder using "PicoLittleFS-0.2.0" etc.
5. Run the serial monitor at a baud rate of 115200.
6. Reboot Raspberry Pi Pico W.
7. HTTP access to the IP displayed on the serial monitor to control the player.
