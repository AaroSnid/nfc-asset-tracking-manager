# NFC Asset Tracking Node

A standalone, NFC-based asset tracking node built on an ESP32 running FreeRTOS through ESP-IDF. The device automates chain-of-custody logging for shared assets (laptops, test devices, equipment) by pairing an asset tag scan with an owner badge scan, then reporting the transaction to a cloud hosted backend service.

## System Overview

The node uses a two-stage scan flow, where an asset tag is presented first, followed by the badge of the person taking custody. Both are passive NFC tags/badges identified by their hardware UID, identifying which UID took which UID, and reports that pairing to the backend. This allows the system to work with any kind of RFID tag, since UIDs are standard and unencrypted. This would theoretically allow existing employee badges to be reused for the tracking system. 

The initial backend target is **Atlassian Confluence**: transactions are appended to a Confluence page via its REST API. Integrating general/custom backends is a future goal.

## Hardware Architecture

- **MCU:** Freenove FREENOVE-ESP32-WROOM dev board — 240 MHz, Wi-Fi, with onboard NVM.
- **NFC reader:** MFRC522 connected over SPI, used to read tag/badge UIDs during anti-collision step of tag reading.
- **Status indicator:** RGB LED (PWM-driven) shows the device's current state.
- **Inputs:** Two tactile buttons for on-device tag provisioning (mapping an unknown tag as an asset or as an owner).
- **Host interface:** USB-Serial (CDC) for initial Wi-Fi/credential staging from a desktop using an external script.

## Firmware Architecture

Firmware is structured as independent FreeRTOS producer-consumer tasks communicating over queues, so network activity never blocks real-time tag scanning:

- **NFC Reader** — Reads the UUID off the MFRC-522 and pushes it to a queue for database posting. Holds no transaction state and doesn't touch buttons.
- **Database posting** — Consumes UUIDs from the queue, performs DB lookups/writes, and drives the asset-owner linking flow. Blocks new scans during a fetch unless the connection is already known down.
- **LED Status** — Owns only the common PWM pin, running brightness patterns like pulsate/flash and returning to idle when done. The 3 color pins are driven directly by any task that needs them.
- **Heartbeat & Telemetry** — Blocks on a mailbox for DB failure sources and dispatches the matching check, and on idle runs the heartbeat check that's the sole source of "known up/down" status, with a single device reset if reconnection fails.
- **Button Input** — GPIO interrupts are unmasked only during a pending classification, with software debounce. Pressing the button after a first scan will attempt a re-classification.

## System Features

- **State & LED feedback:** The device moves through states like idle, awaiting the second scan, processing, success, and error. Each are shown with a distinct LED color/pattern, so status is visible without a screen.
- **Provisioning:** An unrecognized tag triggers an on-device prompt to press one of the two buttons to register it as an asset or an owner.
- **Registration completion:** provisioning only creates a bare UID record.  Manual database modification is required to associate any additional information with the UID such as name or description. Scanning a known tag that is still missing those details is a valid operation.
- **Offline resilience:** If Wi-Fi connection drops, transactions are buffered locally and flushed once connectivity returns, so scanning isn't interrupted by network outages. If this queue fills all scans will result in an error. The data is temporarily stored in NVM so that it is not lost on a power failure. 
- **Reliability:** A watchdog covers the firmware's tasks, and periodic network checks will ensure that the device's internet connection status is known.
- **Security:** Credentials (WiFi, database) are kept out of source code and stored in non-volatile storage. The device is intended for use within a trusted internal environment rather than as a hardened access-control system.