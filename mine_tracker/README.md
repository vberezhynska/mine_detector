Testing UDP SERVER from PowerShell:
$u = [System.Net.Sockets.UdpClient]::new(); $b = [Text.Encoding]::UTF8.GetBytes("TOUCH_EVENT_FROM_PC"); $u.Send($b, $b.Length, "10.42.0.1", 5005); $u.Close()

$u = [System.Net.Sockets.UdpClient]::new(); $b = [Text.Encoding]::UTF8.GetBytes("TOUCH_EVENT_FROM_PC"); $u.Send($b, $b.Length, "192.168.1.199", 5005); $u.Close()

Lat/Long UDP:
$u = [System.Net.Sockets.UdpClient]::new(); $b = [Text.Encoding]::UTF8.GetBytes('{"event":"MINE_FOUND","lat":50451200,"lon":30523400}'); $u.Send($b, $b.Length, "10.42.0.1", 5005); $u.Close(); $u.Dispose()

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

