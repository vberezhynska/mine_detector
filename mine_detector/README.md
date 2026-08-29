/mine_detector - shoudl be deployed on ESP32 and run there. It reads sensors and send data via ethernet to RPi
/mine_tracker - listens to notifications from ESP32, analize them, save points to the file and sends them via MavLink



*** Flash or monitore ESP32 ****    
**HARDWARE**
ESP32 3V3 -> '+' -> VCC / VIN
Touch sensor (green) ->  GPIO 4
Buzzer (blue) -> GPIO 5
GPS TX (yellow) -> GPIO 21
GPS RX (blue) -> GPIO 22

To flash connect Laptop -> UART USB-C

**SOFTWARE**
--- in PowerShell (Admin) ---
usbipd list // find "USB to UART Bridge Controller" : 1-1    10c4:ea60  CP2102N USB to UART Bridge Controller
//if it's Not shared, do: 
usbipd bind --busid 1-1

// When Shared
usbipd attach --wsl --busid 1-1
usbipd list
// shoudl be  : 1-1    10c4:ea60  CP2102N USB to UART Bridge Controller                         Attached

--- In WSL ---
cd mine_detector //main project file
idf.py build
cd mine_detector

/// To Flash from WSL ///
ls /dev/ttyUSB* // should have dev/ttyUSB0
sudo chmod 666 /dev/ttyUSB0
idf.py -p /dev/ttyUSB0 flash monitor

/// To check logs from WSL ///
sudo chmod 666 /dev/ttyUSB0
idf.py -p /dev/ttyUSB0 monitor

/// Exit from monitor ///
Ctrl + ]

/// Soft Reboot ESP32 ///
Ctrl + R
