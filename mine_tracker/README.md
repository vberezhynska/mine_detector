# Mine Tracker (`/mine_tracker`)

Backend service running on the Raspberry Pi. It listens for UDP detection packets from the ESP32 node, analyzes/clusters the detection points, exposes a REST API via Crow, sends currnt location and forwards target markers upstream using MAVLink.

---

## Table of Contents
- [1. Raspberry Pi Initial Setup](#1-raspberry-pi-initial-setup)
- [2. Dependencies & Build](#2-dependencies--build)
- [3. Deployment & Process Control](#3-deployment--process-control)
- [4. Wi-Fi & Hotspot Configuration](#4-wi-fi--hotspot-configuration)
- [5. Running the Application](#5-running-the-application)
- [6. Testing & Verification](#6-testing--verification)

---

## 1. Raspberry Pi Initial Setup

From your host workstation, verify connectivity:
```bash
ping raspberry.local
```

On the Raspberry Pi, update packages and install core build tools (should be connected to the Internet):
```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build git
```

---

## 2. Deployment & Process Control

### Copy Source Code to Raspberry Pi
From your development workstation (e.g., Ubuntu terminal):
```bash
cd ~/repos/mine_detector

# Via standard LAN IP:
scp -r mine_tracker/ user_name@192.168.1.xxx:~/Documents

# Or via Hotspot/Ad-Hoc IP:
scp -r mine_tracker/ user_name@10.42.0.x:~/Documents
```

### Build on Raspberry Pi
Ensure network connectivity is active (if packages need fetching), then configure and build:
```bash
cd ~/Documents/mine_tracker

# Generate build files with Ninja
cmake -S . -B build -G Ninja

# Compile project
cmake --build build
```

### Process Management
Stop running `mine_tracker`:
```bash
Ctrl + C
```

Terminate running instances of `mine_tracker`:
```bash
# Locate PID
pgrep mine_tracker

# Graceful termination (SIGTERM)
kill -15 <PID>

# Force termination (SIGKILL)
kill -9 <PID>
```

---

## 3. Wi-Fi & Hotspot Configuration

The Pi should run in **Access Point (AP)** mode to communicate with the ESP32. The Pi can also run **Client Mode** (for home/lab Wi-Fi).

### Create & Manage Offline Hotspot
```bash
# Create hotspot interface
sudo nmcli device wifi hotspot ifname wlan0 con-name "OfflineAP" ssid "ESP32_Pi_Network" password "PiSecretKey123"

# Enable auto-start on boot
sudo nmcli connection modify "OfflineAP" connection.autoconnect yes

# Verify active interfaces
nmcli connection show --active

# Stop/Start the hotspot manually
sudo nmcli connection down "OfflineAP"
sudo nmcli connection up "OfflineAP"

# Check assigned IP address
ip addr show wlan0
```

### Switch to Home Wi-Fi & Configure Failover
```bash
# Connect to home network
sudo nmcli device wifi connect "YOUR_HOME_WIRELESS_NETWORK" password "YOUR_HOME_PASSWORD"

# Set high priority on Home Wi-Fi (prefers Home Wi-Fi when in range)
sudo nmcli connection modify "YOUR_HOME_WIRELESS_NETWORK" connection.autoconnect-priority 10

# Set lower priority on AP (falls back to AP when out of home range)
sudo nmcli connection modify "OfflineAP" connection.autoconnect-priority 1
```

---

## 4. Running the Application

`mine_tracker` accepts CLI arguments and environment variables for MAVLink telemetry targets.

```bash
cd ~/Documents/mine_tracker/build

# 1. Custom MAVLink IP with custom detection clustering radius (e.g., 15m)
MAVLINK_TARGET_IP=192.168.1.xxx ./mine_tracker --radius 15

# 2. Custom radius with default MAVLink target
./mine_tracker --radius=5

# 3. Default configuration (default IP, default 30-meter radius)
./mine_tracker
```

---

## 5. Testing & Verification

### 5.1 UDP Sensor Ingestion (Port `5005`)

#### From PowerShell:
```powershell
$u = [System.Net.Sockets.UdpClient]::new();
$b = [byte[]](0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00);
[System.BitConverter]::GetBytes([uint32]504512000).CopyTo($b, 1);
[System.BitConverter]::GetBytes([uint32]305234000).CopyTo($b, 5);
$b[9] = [byte]2;
[System.BitConverter]::GetBytes([uint32]1700000000).CopyTo($b, 10);
$u.Send($b, $b.Length, "raspberry.local", 5005);
$u.Close()
```

#### From Docker / Linux Container:
```bash
python3 -c "import socket, struct; \
data = struct.pack('<BIIBI', 0x01, 504512200, 305235000, 2, 1700000000); \
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM); \
sock.sendto(data, ('127.0.0.1', 5005)); \
print('Sent', len(data), 'bytes')"
```

---

### 5.2 HTTP REST API (Port `8080`)

#### Trigger Alert Ingestion (`POST /v1/api/alerts`)

**Sample Request:**
```powershell
curl.exe -i -X POST http://192.168.1.199:8080/v1/api/alerts `
  -H "Content-Type: application/json" `
  -d '{"event": "detection", "lat_int": 504512000, "lon_int": 305219874, "gpType": 2}'
```

**Additional Payload Samples:**
```json
// Point A
{
  "event": "detection",
  "lat_int": 504512000,
  "lon_int": 305219874,
  "gpType": 2
}

// Point B
{
  "event": "detection",
  "lat_int": 504512000,
  "lon_int": 305262247,
  "gpType": 2
}

// Point C
{
  "event": "detection",
  "lat_int": 504512000,
  "lon_int": 305299874,
  "gpType": 2
}
```