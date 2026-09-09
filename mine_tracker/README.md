Testing UDP SERVER from PowerShell:
$u = [System.Net.Sockets.UdpClient]::new(); $b = [byte[]](0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00); [System.BitConverter]::GetBytes([uint32]50451200).CopyTo($b, 1); [System.BitConverter]::GetBytes([uint32]30523400).CopyTo($b, 5); $b[9] = [byte]2; [System.BitConverter]::GetBytes([uint32]1700000000).CopyTo($b, 10); $u.Send($b, $b.Length, "192.168.1.199", 5005); $u.Close()

Testing HTTP SERVER from PowerShell:
GET:
curl.exe -i -X GET http://10.42.0.1:8080/v1/api/status

curl.exe -i -X GET http://192.168.1.199:8080/v1/api/status

POST:
POST http://10.42.0.1:8080/v1/api/alerts
POST http://192.168.1.199:8080/v1/api/alerts
{"event": "MINE_FOUND", "lat": 50451200, "lon": 30523400}


# Clone Crow repository
git clone https://github.com/CrowCpp/Crow.git
cd Crow
mkdir build && cd build
cmake .. -DCROW_BUILD_EXAMPLES=OFF -DCROW_BUILD_TESTS=OFF
sudo make install

*** On clean R Pi ***
sudo apt update
sudo apt install -y build-essential cmake libasio-dev nlohmann-json3-dev git

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

*** Pi AP ***
sudo nmcli device wifi hotspot ifname wlan0 con-name "OfflineAP" ssid "ESP32_Pi_Network" password "PiSecretKey123"

