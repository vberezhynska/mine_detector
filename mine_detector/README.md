# Mine Detector (`/mine_detector`)

Firmware for the ESP32 edge device. It samples onboard sensor readings (touch, GPS, buzzer) and streams telemetry to a Raspberry Pi over Ethernet.

---

## 1. Hardware Setup

### Pinout & Wiring

| Component Pin | ESP32 Pin | Wire Color | Description |
| :--- | :--- | :--- | :--- |
| **VCC / VIN** | `3V3` / `+` | — | Common 3.3V power bus |
| **Touch Sensor** | `GPIO 4` | Green | Digital input |
| **Buzzer** | `GPIO 5` | Blue | Output |
| **GPS TX** | `GPIO 21` | Yellow | ESP32 RX |
| **GPS RX** | `GPIO 22` | Blue | ESP32 TX |

> **Connection:** Connect the laptop directly to the ESP32 via the onboard **UART USB-C** port for power, flashing, and serial logging.

---

## 2. Environment Setup (Windows WSL2)

If building and flashing through WSL2 or a DevContainer on Windows, forward the serial bridge using `usbipd`.

### PowerShell (Run as Administrator)

1. Identify the device Bus ID:
   ```powershell
   usbipd list
   ```
   *Look for: `CP2102N USB to UART Bridge Controller` (e.g., `1-1`).*

2. Bind the device (persists across sessions):
   ```powershell
   usbipd bind --busid 1-1
   ```

3. Attach the device to WSL:
   ```powershell
   usbipd attach --wsl --busid 1-1
   ```

4. Confirm state:
   ```powershell
   usbipd list
   ```
   *The target device state should now report as **`Attached`**.*

---

## 3. Build, Flash & Monitor (WSL / Docker)

From the project directory:

```bash
cd mine_detector
```

### Build Firmware

```bash
idf.py build
```

> **Troubleshooting:** If the build fails due to memory exhaustion or toolchain concurrency issues, limit the compiler to a single worker:
> ```bash
> idf.py build -- -j1
> ```

---

### Flash & Monitor

1. Verify that the virtual port is recognized:
   ```bash
   ls /dev/ttyUSB*
   ```
   *You should see `/dev/ttyUSB0`. If missing, verify the `usbipd attach` step or `reload the container`.*

2. Ensure read/write device permissions:
   ```bash
   sudo chmod 666 /dev/ttyUSB0
   ```

3. Flash the binary and stream monitor logs:
   ```bash
   idf.py -p /dev/ttyUSB0 flash monitor
   ```

---

### Standalone Monitoring

To open the serial console without reflashing:

```bash
sudo chmod 666 /dev/ttyUSB0
idf.py -p /dev/ttyUSB0 monitor
```

#### ESP-IDF Monitor Shortcuts

| Shortcut | Action |
| :--- | :--- |
| `Ctrl + ]` | Exit monitor |
| `Ctrl + R` | Soft-reset the ESP32 chip |

---

## 4. Running Unit Tests

Run local GPS parser tests using CMake:

```bash
mkdir -p test/build && cd test/build
cmake ..
make -j$(nproc)
./run_gps_tests
```