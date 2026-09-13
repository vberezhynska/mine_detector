# Project TODO List

## 📌 High Priority (deadline 14.09)
## Definition of Done: working UDP with GPS and HTTP with alert
- [ ] Testing gps and http
- [ ] Add parser and location GPS logic within Radius R
- [ ] Identify flags what are in radius R

## 📌 High Priority (deadline 15.09)
## Definition of Done: fully working solution with QG Control and correctly identified flags
- [ ] Last GPS - thing how to treat outdated location. Like is_fresh or last updated timestemp
- [ ] Mine detector: Update send requst with last_gps fight after sensor was triggered
- [ ] Add QG Control
- [ ] Mine detector: Act if send flag failed
- [ ] Heartbit for ESP32, tracked by R_Pi
- [ ] Mine detector - Error handler in case of 3 failed http - may be fail heartbit or so

## 📝 Testing + Medium Priority (deadline 19.09)
- [ ] Think how to store GPS coordinates
- [ ] Write tests
- [ ] Write md project description
- [ ] Move hardcoded values to Configuration

## ☕ Learning & Low Priority
- [ ] Understand socket
- [ ] Understand FreeRTOS tasks

---
*Update on: September 13, 2026*
*Created on: September 6, 2026*


[UDP DATA] Mine Alert Received!
  ├─ Latitude:  47181609
  ├─ Longitude: 8492078
  ├─ GPS Fix:   1
  └─ Timestamp: 409
[UDP DATA] Mine Alert Received!
  ├─ Latitude:  47181609
  ├─ Longitude: 8492078
  ├─ GPS Fix:   2
  └─ Timestamp: 410
(2026-09-13 12:36:11) [INFO    ] Request: 10.42.0.97:58841 0x7ffef4003800 HTTP/1.1 POST /v1/api/alerts
[MINE ALERT] Event: MINE_DETECTED | Lat: 47 | Lon: 8
 | GpType:
[HTTP MINE ALERT] Lat: 47 | Lon: 8
(2026-09-13 12:36:11) [INFO    ] Response: 0x7ffef4003800 /v1/api/alerts 200 1
(2026-09-13 12:36:18) [INFO    ] Request: 10.42.0.97:58842 0x7ffef4002280 HTTP/1.1 POST /v1/api/alerts