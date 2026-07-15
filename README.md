# NFC Asset Tracking Node

A standalone, dual-core NFC asset tracking node built on an ESP32-S3 running FreeRTOS. The device automates chain-of-custody logging for shared assets (laptops, test devices, equipment) by pairing an asset tag scan with an owner badge scan, then reporting the transaction to a backend service.

## System Overview

The node uses a two-stage scan flow, where an asset tag is presented first, followed by the badge of the person taking custody. Both are passive NFC tags/badges identified by their hardware UID, identifying which UID took which UID, and reports that pairing to the backend. This allows the system to work with any kind of RFID tag, since UIDs are standard and unencrypted. 

The initial backend target is **Atlassian Confluence**: transactions are appended to a Confluence page via its REST API. Integrating general/custom backends is a future goal.

## Hardware Architecture

- **MCU:** Freenove ESP32-S3-WROOM dev board — dual-core, 240 MHz, Wi-Fi, with onboard PSRAM.
- **NFC reader:** MFRC522 (or PN532), connected over SPI, used to read tag/badge UIDs during anti-collision.
- **Status indicator:** an RGB LED (PWM-driven) shows the device's current state at a glance.
- **Inputs:** two tactile buttons for on-device tag provisioning (mapping an unknown tag as an asset or as an owner).
- **Host interface:** USB-Serial (CDC) for initial Wi-Fi/credential staging from a desktop.

## Firmware Architecture

Firmware is structured as independent FreeRTOS tasks communicating over queues, so network activity never blocks real-time tag scanning:

- **Main Control & NFC Scanner** (high priority) — polls the reader, tracks in-progress transactions, and handles button input.
- **Network & Webhook Handler** (medium priority) — owns Wi-Fi state and sends completed transactions to the backend over TLS.
- **UI & LED Indicator** (low priority) — drives the RGB LED to reflect current device state.
- **Heartbeat & Telemetry** (low priority) — periodically reports device health (uptime, signal strength, memory, queue depth).

## System Features

- **State & LED feedback:** the device moves through states like idle, awaiting the second scan, processing, success, and error. Each are shown with a distinct LED color/pattern, so status is visible without a screen.
- **Provisioning:** an unrecognized tag triggers an on-device prompt to press one of the two buttons to register it as an asset or an owner.
- **Registration completion:** provisioning only creates a bare UID record. A person still has to fill in the asset/owner's details on the backend afterward. Scanning a tag that's known but still missing those details is treated as an error (LED flashes red) until it's completed.
- **Offline resilience:** if Wi-Fi drops, transactions are buffered locally and flushed once connectivity returns, so scanning isn't interrupted by network outages.
- **Reliability:** a hardware watchdog covers the firmware's tasks, and periodic heartbeats surface problems before a user hits them.
- **Security:** credentials are kept out of source code and stored in non-volatile storage; network traffic is encrypted; the device is intended for use within a trusted internal environment rather than as a hardened access-control system.
- **Testing:** basic checks cover scan responsiveness, offline/reconnect behavior, watchdog recovery, and button debounce.