# BLE Protocol Interface

Shared Bluetooth Low Energy (BLE) protocol definition for the ELEC4740 sensor network.

This protocol will be used by:
- Sensor Node 1 (SN1)
- Sensor Node 2 (SN2)
- Control Node (CN, Part 2)

---

## Purpose

This directory defines the **fixed BLE interface contract** between all
nodes in the system. Its goal is to allow SN1 and SN2 to be developed
independently, while ensuring (hopefully) smooth integration in Part 2.

The protocol defines:
- UUIDs
- Packet layouts and sizes
- Scaling rules for telemetry
- Event and control semantics

---

## Platform Assumptions

- BLE roles:
  - Part 1: SN1 and SN2 operate as BLE peripherals
  - Part 2: CN operates as BLE central, SN1/SN2 remain peripherals
- Endianness: little-endian on the wire

---

## Protocol Overview

Each sensor node exposes:
- One primary BLE service
- Three characteristics:
  - Telemetry (Notify)
  - Event (Notify)
  - Control (Write)

All BLE payloads use fixed-size, packed structures. See `ble_protocol.hpp`

---

## Design Rules

- Integer-only BLE payloads (no floating point on the wire)
- Explicit scaling (e.g. centi-degrees, deci-lux, per-mille duty)
- Compile-time validation of packet sizes
- Versioned protocol
- No dynamic memory allocation
- Deterministic behaviour

---

## Modifying the Protocol

Changes to `ble_protocol.hpp` would affect **all nodes**.

If changes are required:
1. Update the protocol version if compatibility is broken
2. Update packet size assertions as needed
3. Update test vectors

Hopefully this will not be necessary in Part 2.
