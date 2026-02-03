# ELEC4740 BLE Interface Contract
Version: 0.1
Platform: Particle Photon 2
Applies to: SN1, SN2 (Part 1) and CN (Part 2)

## 1. Purpose
This document defines the Bluetooth LE interface between the two sensor
nodes (SN1 and SN2) and the control node (CN). The intent is to lock in
the data model early so each sensor node can be developed independently
in Part 1 and integrated with minimal friction in Part 2.

## 2. Roles and Device OS Requirements
- Part 1:
	- SN1: BLE Peripheral
	- SN2: BLE Peripheral
	- No control node required
- Part 2:
	- CN: BLE Central
	- SN1: BLE Peripheral
	- SN2: BLE Peripheral
- Device OS requirement:
	- All devices MUST run Device OS >= 5.1.0
	- Reason: Photon 2 central mode support requires >= 5.1.0
	- We will be using 6.3.4

## 3. Connection and Update Behaviour
- Advertising:
	- Sensor nodes advertise continuously while powered
	- Advertising includes a device name identifying node type (SN1 or SN2)
- Connection status:
	- On sensor nodes, comms LED:
		- Flashing green: powered, not connected
		- Solid green: BLE connected
- Telemetry:
	- Sent at a fixed rate (default 1 Hz)
- Events:
	- Sent immediately on detection (help toggle, motion, sound)
	- Event transmissions are rate-limited to prevent saturation

## 4. GATT (Generic Attribute) Layout
Each sensor node exposes one primary service with three characteristics.

### 4.1 Service UUID
- TODO: assign final 128-bit UUID shared by both sensor nodes
- Placeholder:
	- SERVICE_UUID = "XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX"

### 4.2 Characteristics
1) Telemetry characteristic
- UUID: TODO (shared layout on both nodes)
- Properties: Notify
- Payload: `telemetry_packet_t`

2) Event characteristic
- UUID: TODO
- Properties: Notify
- Payload: `event_packet_t`

3) Control characteristic
- UUID: TODO
- Properties: Write (Write Without Response could be acceptable?)
- Payload: `control_packet_t`

## 5. Payload Rules (Wire Format)
- All payload fields are integers (no floats transmitted)
- Endianness: Little-endian
- Boolean flags packed into bitfields where practical
- All packets include:
	- node_id
	- protocol_version
- Scaling:
	- Temperature: centi-degrees C (°C * 100)
	- Lux: deci-lux (lux * 10)
	- Duty cycle: 0..1000 (per-mille)

## 6. Packet Definitions

### 6.1 Node IDs
- SN1: node_id = 1
- SN2: node_id = 2
- CN: node_id = 0 (not transmitted as peripheral)

### 6.2 Telemetry Packet (SN -> CN)
Sent periodically (default 1 Hz). This is the current state "snapshot".

`telemetry_packet_t` (packed):
- protocol_version: uint8
- node_id: uint8
- flags: uint16
	- bit0: help_active
	- bit1: override_active
	- bit2: sensor_fault
	- bit3..15: reserved
- primary_value: int16
	- SN1: lux_deci (lux * 10), valid range 100..2000 (10..200 lux)
	- SN2: temp_centi (°C * 100), valid range 500..4500 (5..45°C)
- secondary_value: int16
	- SN1: motion_state (0 or 1) in LSB, remaining bits reserved
	- SN2: sound_state (0 or 1) in LSB, remaining bits reserved
- potentiometer_raw: uint16
	- Raw ADC or scaled pot reading used for local duty request
- duty_commanded: uint16
	- Actual duty applied by sensor node (0..1000)
- reserved: uint16
	- Set to 0

Notes:
- `duty_commanded` MUST reflect the duty actually applied after
  considering override_active from control node.

### 6.3 Event Packet (SN -> CN)
Sent immediately on edge-triggered events.

`event_packet_t` (packed):
- protocol_version: uint8
- node_id: uint8
- event_type: uint8
	- 1: help_toggled
	- 2: motion_detected (SN1 only)
	- 3: sound_detected (SN2 only)
	- 4: sensor_fault
- event_value: int16
	- help_toggled: 0=cleared, 1=active
	- motion_detected: 1
	- sound_detected: 1
	- sensor_fault: fault code (implementation TBD)
- timestamp_ms_mod: uint16
	- Millisecond timer modulo 65536 for basic ordering to avoid race conditions

Rate limiting:
- motion/sound events may retrigger only after the 5 second indication window
  expires (default). If retriggering is enabled, the window resets.

### 6.4 Control Packet (CN -> SN)
Written by CN to request override behaviour.

`control_packet_t` (packed):
- protocol_version: uint8
- target_node_id: uint8
	- 1=SN1, 2=SN2
- command_flags: uint16
	- bit0: override_enable
	- bit1: clear_help_request
	- bit2..15: reserved
- duty_override: uint16
	- 0..1000, used when override_enable=1
- reserved: uint16
	- Set to 0

Rules:
- If clear_help_request=1, the sensor node MUST clear help_active.
- If override_enable=0, the sensor node MUST revert to potentiometer
  control.

## 7. Local Behaviour Requirements (Sensor Nodes)
- Help button:
	- Press toggles help_active (debounced)
	- On toggle, send event_packet_t with event_type=help_toggled
	- Help LED red when help_active=1
- Motion/sound:
	- On detection, send event_packet_t immediately
	- Help LED flashes for 5 s:
		- green if help_active=0
		- red if help_active=1
- Comms LED:
	- Flash green when not connected
	- Solid green when connected

## 8. Integration Checklist
Part 1 (each sensor node):
- Implements advertising and GATT service + characteristics
- Sends telemetry at 1 Hz
- Sends events for help and motion/sound
- Accepts control writes (may be stubbed to set override_active and
  duty_override)

Part 2 (control node):
- Scans for SN1 and SN2 advertisements
- Connects to both peripherals
- Subscribes to telemetry and event notifications
- Writes control commands to override duty and clear help

## 9. Open Items (to finalise early)
- Assign final 128-bit UUIDs:
	- SERVICE_UUID
	- TELEMETRY_UUID
	- EVENT_UUID
	- CONTROL_UUID
- Confirm telemetry rate (default 1 Hz)
- Confirm event retrigger policy (default: lockout during 5 s window)
- Confirm whether help can be cleared locally only or also from CN
