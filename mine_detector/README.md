/mine_detector - shoudl be deployed on ESP32 and run there. It reads sensors and send data via ethernet to RPi
/mine_tracker - listens to notifications from ESP32, analize them, save points to the file and sends them via MavLink


*** Flash or monitore ESP32 ****
--- in PowerShell (Admin) ---
usbipd list // find "USB to UART Bridge Controller" : 1-1    10c4:ea60  CP2102N USB to UART Bridge Controller
//if it's Not shared, do: 
usbipd bind --busid 1-1

// When Shared
usbipd attach --wsl --busid 1-1
usbipd list
// shoudl be  : 1-1    10c4:ea60  CP2102N USB to UART Bridge Controller                         Attached

--- In WSL ---
cd mine_detector

/// To Flash from WSL ///
sudo chmod 666 /dev/ttyUSB0
idf.py -p /dev/ttyUSB0 flash monitor

/// To check logs from WSL ///
sudo chmod 666 /dev/ttyUSB0
idf.py -p /dev/ttyUSB0 monitor