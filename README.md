# Automated Mine Localization & Telemetry Tracking System

An end-to-end humanitarian demining telemetry system designed to accelerate the detection and mapping of landmines using bio-detection (trained HeroRATs) paired with low-power edge sensing and real-time GIS mapping.

---

## 1. Project Objective

Landmines and unexploded ordnance (UXO) remain a severe humanitarian hazard across war-affected regions, denying safe access to fertile agricultural land and essential infrastructure. Traditional manual demining is dangerous, labor-intensive, and slow.

This project aims to **speed up humanitarian demining operations** by automating detection logging and field telemetry. Inspired by operational frameworks like [APOPO](https://apopo.org/) and their deployment of scent-detecting animals (see [HeroRATs in Action](https://youtu.be/UE94Sxp6mY8?is=pz8ypWkEKs3injI2)), this system creates a digital bridge between field search activity and map coordinates, minimizing human operator exposure to suspect hazardous areas.

---

## 2. Localization Flow & Working Mechanism

The system operates using two parallel pipelines:
1. **Continuous Real-Time Tracking:** Streams the current position every 1 second to monitor the search animal's path in real time.
2. **Event-Driven Localization Alert:** Instantly triggered when an explosive odor is identified and the touch sensor is pressed.

---

### Operational Data Flow Diagram

```
===================================================================================================
 PIPELINE 1: CONTINUOUS POSITION TRACKING (Every 1s)        PIPELINE 2: LOCALIZATION ALERT (Event-Driven)
===================================================================================================

       [ GPS Receiver ]                                        [ HeroRAT Scent Indication ]
              |                                                            |
              | (1 Hz NMEA/Coordinates)                                    v
              v                                                    [ Touch Sensor Active ]
  +-----------------------+                                                |
  | ESP32 Tracking Loop   |                                                v
  |  - Reads current fix  |                                    +-----------------------+
  |  - Formats beacon UDP |                                    | ESP32 Audio Feedback  |
  +-----------+-----------+                                    |  - Fires buzzer click |
              |                                                |  - Rewards the rat    |
              |                                                +-----------+-----------+
              |                                                            |
              |                                                            v
              |                                                +-----------------------+
              |                                                | ESP32 Alert Telemetry |
              |                                                |  - Locks GPS coords   |
              |                                                |  - Packs alert UDP    |
              |                                                +-----------+-----------+
              |                                                            |
              | (UDP: Current Position / 1 Hz)                             | (UDP: Alert Packet)
              +-----------------------------\   /--------------------------+
                                             \ /
                                              v
                              +-------------------------------+
                              |    BASE STATION (RPi 4)       |
                              |       /mine_tracker           |
                              |-------------------------------|
                              | - Updates live rat position   |
                              | - Clusters detection points   |
                              |   into candidate mine groups  |
                              | - Serves REST API / Web UI    |
                              +---------------+---------------+
                                              |
                                              | (MAVLink telemetry: UDP 14550)
                                              v
                              +-------------------------------+
                              |   GROUND CONTROL STATION      |
                              |       QGroundControl          |
                              |-------------------------------|
                              | - Live marker updates (1 Hz)  |
                              | - Mine target cluster pins    |
                              +-------------------------------+
```

---

### Detailed Pipeline Descriptions

#### 1. Continuous Position Tracking (1 Hz)
* **Periodic Polling:** Every 1s, the ESP32 samples current latitude, longitude and GPS fix quality (only GPS with accepted confidence and quality are processed)
* **Stream to Base Station:** Coordinates are packaged into a periodic heartbeat UDP datagram and transmitted over Wi-Fi to `/mine_tracker` running on the Raspberry Pi.
* **MAVLink Forwarding:** The Raspberry Pi translates the continuous coordinates into MAVLink global position messages (`GLOBAL_POSITION_INT` / `HEARTBEAT`) forwarded to UDP port `14550`.

#### 2. Event-Driven Localization Alert
1. **Target Identification:** Upon detecting explosive vapors, the HeroRAT scratches/taps the harness-mounted touch sensor.
2. **Instant Acoustic Reward Cue:** The ESP32 triggers a precise piezo buzzer tone within milliseconds. This serves as conditioned reinforcement indicating a food reward is pending at the boundary line.
3. **Hazard Telemetry Capture:** Simultaneously, the ESP32 logs the coordinate fix and transmits an alert payload to `/mine_tracker`.
4. **Spatial Correlation:** The Raspberry Pi checks the radius threshold (e.g., `15 m` or default `30 m`). Points within the cluster radius update the existing cluster centroid; coordinates outside establish a new suspect cluster.
5. **Map Pinning:** QGroundControl instantly renders a high-visibility hazard waypoint on the satellite view for the disposal team.

## 3. Localization & Clustering Radius

> **Note on GPS Precision:**  
> This prototype currently utilizes a standard, commercial-grade GPS module that exhibits positional drift and accuracy variations ($\pm 5$ to $15$ meters) under dynamic field conditions.  
> 
> Because of this hardware limitation, the tracking software (`mine_tracker`) implements an adjustable clustering radius (e.g., `--radius 30`). Spatial detections falling within this threshold are correlated as belonging to the same candidate target cluster rather than generating duplicate hazard markers.
Each target cluster maintains a dynamic confidence score based on the frequency and timing of incoming detection triggers:
>
>* **Spatial Consolidation**: Repeated triggers registered within the configured cluster radius reinforce the likelihood of a genuine target rather than a sensor artifact.
>
>* **Temporal Weighting (Rapid Triggers)**: If multiple signals occur in rapid succession—specifically arriving within less than 5 seconds of one another—the cluster's confidence score receives a slight bonus increase. This models the animal's focused scratch/touch behavior over an active scent plume without artificially inflating single-event noise.
>
> Alert and mine location are send to QGControl, when confidence is more or equal 80%, which corresponds to 3 clear alerts in the area.


---

## 4. System Architecture

The project consists of two core sub-modules:

* **[`/mine_detector`](./mine_detector):** C++ / ESP-IDF firmware running on an ESP32 micro-controller. Interfaces with the touch sensor, GPS module, and buzzer, streaming telemetry over UDP.
* **[`/mine_tracker`](./mine_tracker):** C++ backend running on a Raspberry Pi. Listens for incoming UDP alerts, clusters detection events, exposes an HTTP REST API via Crow, and forwards markers to GIS software via MAVLink.

---

## 5. Quick Start & Operational Walkthrough

### Step 1: Start `mine_tracker` on the Raspberry Pi
Power up the Raspberry Pi and launch the tracking service (ensure hotspot or shared Wi-Fi is active):

```bash
cd ~/Documents/mine_tracker/build
./mine_tracker --radius 35
```

### Step 2: Power the ESP32 Node
Connect the battery or power source to the ESP32 harness. The ESP32 will auto-connect to the Pi's Wi-Fi network and begin polling GPS data.

### Step 3: Connect QGroundControl
1. Launch **QGroundControl** on your field monitoring laptop or tablet.
2. Open **Application Settings** $\rightarrow$ **Comm Links**.
3. Add a new **UDP Connection** using the default port (`14550`).
4. Connect and switch to **Fly Mode** to observe detection coordinates and target clusters plotting onto the live satellite map.

---

## 6. Roadmap & Future Improvements

- [ ] **Persistent Storage & State Recovery:** Store all alert points, cluster groups, and confidence histories in a persistent database on the Raspberry Pi so no alert telemetry is lost across service restarts.
- [ ] **Startup State Sync:** Automatically reload active spatial clusters into the detection analyzer on startup to prevent cold-start data gaps.
- [ ] **Robust Offline & Sensor Fail-safes:** Implement error handling and local caching (e.g., flash ring buffer) on the ESP32 to retain timestamps and sensor triggers if Wi-Fi drops, plus fallback logic for handling stale or lost GPS fixes during an active run.
- [ ] **Autonomous Feeder Rover:** Integrate an automated mobile reward rover that follows the animal  to dispense food rewards in the field.
- [ ] **RTK Precision Upgrade:** Replace standard GPS with RTK to fix the current $30\text{ m}$ position drift (based on feeder rover). With centimeter-level accuracy, deminers can pinpoint exact mine locations instead of searching wide radius zones.