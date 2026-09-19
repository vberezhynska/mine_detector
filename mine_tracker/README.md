Testing UDP SERVER from PowerShell:
$u = [System.Net.Sockets.UdpClient]::new(); $b = [byte[]](0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00); [System.BitConverter]::GetBytes([uint32]504512000).CopyTo($b, 1); [System.BitConverter]::GetBytes([uint32]305234000).CopyTo($b, 5); $b[9] = [byte]2; [System.BitConverter]::GetBytes([uint32]1700000000).CopyTo($b, 10); $u.Send($b, $b.Length, "vabe-pi", 5005); $u.Close()


from container:
python3 -c "import socket, struct; data = struct.pack('<BIIBI', 0x01, 504512200, 305235000, 2, 1700000000); sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM); sock.sendto(data, ('127.0.0.1', 5005)); print('Sent', len(data), 'bytes')"

Testing HTTP SERVER from PowerShell:
GET:
curl.exe -i -X GET http://10.42.0.1:8080/v1/api/status

curl.exe -i -X GET http://192.168.1.199:8080/v1/api/status

POST:
POST http://10.42.0.1:8080/v1/api/alerts
POST http://192.168.1.199:8080/v1/api/alerts
{
  "event": "detection",
  "lat_int": 504512000,
  "lon_int": 305219874,
  "gpType": 2
}

{
  "event": "detection",
  "lat_int": 504512000,
  "lon_int": 305262247,
  "gpType": 2
}

{
  "event": "detection",
  "lat_int": 504512000,
  "lon_int": 305299874,
  "gpType": 2
}



# Clone Crow repository
git clone https://github.com/CrowCpp/Crow.git
cd Crow
mkdir build && cd build
cmake .. -DCROW_BUILD_EXAMPLES=OFF -DCROW_BUILD_TESTS=OFF
sudo make install

*** On clean R Pi ***
ping vabe-pi.local
sudo apt update
sudo apt install -y build-essential cmake

*** Copy solution to Pi ***
In Ubuntu:
cd ~/repos/mine_detector$
vberezhynska@Thinky: scp -r mine_tracker/ vabe@192.168.1.199:~/Documents
or
scp -r mine_tracker/ vabe@10.42.0.1:~/Documents

*** Build solution to Pi ***
(make sure you are connected to WiFi): sudo nmcli connection up netplan-wlan0-WirelessNet_2
cd mine_tracker/
cmake -S . -B build -G Ninja
cmake --build build

kill process:
pgrep mine_tracker
kill -15 <PID>
kill -9 <PID> //forse

*** Pi AP ***
sudo nmcli device wifi hotspot ifname wlan0 con-name "OfflineAP" ssid "ESP32_Pi_Network" password "PiSecretKey123"
// Automatic startup
sudo nmcli connection modify Pi-AP connection.autoconnect yes
// verify it's active
nmcli connection show --active
// turn off
sudo nmcli connection down Pi-AP
//turn on
sudo nmcli connection up Pi-AP
//check ip address
ip addr show wlan0
# Switch from AP back to Home Wi-Fi directly
sudo nmcli device wifi connect "netplan-wlan0-WirelessNet_2" password "YOUR_HOME_PASSWORD"
# Set high autoconnect priority on Home Wi-Fi
sudo nmcli connection modify "netplan-wlan0-WirelessNet_2" connection.autoconnect-priority 10

# Set lower autoconnect priority on AP
sudo nmcli connection modify "Pi-AP" connection.autoconnect-priority 1

*** Run wiht MavLink variable ***
Run from Local:
MAVLINK_TARGET_IP=192.168.1.222 ./mine_tracker

*** Settings in QGround Control to display circle ***
Enable Fence Display in QGCMake sure the UI layer is set to display fences:In QGC, click the Map Settings / Layers icon (the paper stack icon on the left/top-right of the map view).Look for GeoFence or Inclusion/Exclusion Zones and ensure it is checked/toggled ON.Switch to the Plan view (top-left menu $\rightarrow$ Plan) and click GeoFence to see if any fence items are populated in the mission list.