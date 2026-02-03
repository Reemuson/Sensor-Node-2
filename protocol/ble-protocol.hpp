/**
 * @file	ble_protocol.hpp
 * @brief	Bluetooth LE protocol definitions for ELEC4740 sensor network
 *
 * @author	Ryan Hicks
 * @affiliation	School of Engineering (Electrical and Computer Engineering),
 *		University of Newcastle
 *
 * @project	ELEC4740
 * @date	2026-02-03
 * @version	1.0
 *
 * @license	Academic use only
 *
 * @details
 * This header defines the fixed Bluetooth Low Energy (BLE) wire protocol
 * shared between Sensor Node 1 (SN1), Sensor Node 2 (SN2) and the Control
 * Node (CN). It specifies UUIDs, packet layouts, scaling rules and helper
 * functions required to ensure byte-for-byte compatibility.
 *
 * The protocol is designed to support independent development of sensor
 * node firmware, followed by system-level integration with minimal refactoring.
 * All packet structures are packed, versioned and validated at compile time to
 * prevent silent interface changes.
 *
 * All values transmitted over BLE use fixed-width integer representations
 * with explicit scaling (e.g. centi-degrees Celsius, deci-lux, per-mille
 * duty cycle) to avoid floating-point transmission and ensure deterministic
 * behaviour.
 *
 * @note
 * - Target platform: Particle Photon 2
 * - Device OS requirement: >= 6.3.4
 * - Endianness: little-endian on the wire
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

/**
 * @brief BLE protocol UUID definitions.
 * @note These UUIDs are a fixed contract shared by SN1, SN2 and CN.
 */
struct ble_uuid_t final
{
	static constexpr const char *service =
	    "8f9d2a10-6a7b-4c7e-9f7b-2c6a0e1d8a40";
	static constexpr const char *telemetry =
	    "8f9d2a11-6a7b-4c7e-9f7b-2c6a0e1d8a40";
	static constexpr const char *event =
	    "8f9d2a12-6a7b-4c7e-9f7b-2c6a0e1d8a40";
	static constexpr const char *control =
	    "8f9d2a13-6a7b-4c7e-9f7b-2c6a0e1d8a40";
};

/**
 * @brief BLE protocol version for payload compatibility checks.
 */
enum class ble_protocol_version_t : std::uint8_t
{
	v1 = 1u
};

/**
 * @brief Logical node identifiers used on the BLE link.
 */
enum class ble_node_id_t : std::uint8_t
{
	control = 0u,
	sn1 = 1u,
	sn2 = 2u
};

/**
 * @brief Telemetry state flags reported by sensor nodes.
 */
enum class ble_telemetry_flag_t : std::uint16_t
{
	help_active = (1u << 0),
	override_active = (1u << 1),
	sensor_fault = (1u << 2)
};

/**
 * @brief Event types reported asynchronously by sensor nodes.
 */
enum class ble_event_type_t : std::uint8_t
{
	help_toggled = 1u,
	motion_detected = 2u,
	sound_detected = 3u,
	sensor_fault = 4u
};

/**
 * @brief Control command flags written from control node to sensor nodes.
 */
enum class ble_control_flag_t : std::uint16_t
{
	override_enable = (1u << 0),
	clear_help_request = (1u << 1)
};

/**
 * @brief Scaling and timing constants defining protocol behaviour.
 * @note Duty is in per-mille (0..1000), where 1000 corresponds to 100%.
 */
struct ble_protocol_constants_t final
{
	static constexpr std::int32_t temperature_centi_per_c = 100;
	static constexpr std::int32_t lux_deci_per_lux = 10;
	static constexpr std::uint16_t duty_per_mille_max = 1000u;

	static constexpr std::uint32_t telemetry_period_ms = 1000u;
	static constexpr std::uint32_t event_lockout_ms = 5000u;
};

/*
 * Force 1-byte packing for all protocol structs below.
 *
 * This guarantees:
 * - A deterministic, byte-for-byte wire format
 * - Struct layout exactly matches BLE characteristic payloads
 *
 * Only the protocol structs are packed; packing is restored
 * immediately afterwards to avoid affecting the rest of the codebase.
 */
#pragma pack(push, 1)

/**
 * @brief Periodic telemetry packet sent from sensor node to control node.
 * @note This struct is transmitted verbatim over BLE as a byte array.
 *
 * Field meaning:
 * - SN1:
 *	primary_value		= lux_deci (lux * 10)
 *	secondary_value bit0	= motion_state (0 or 1)
 * - SN2:
 *	primary_value		= temp_centi (degC * 100)
 *	secondary_value bit0	= sound_state (0 or 1)
 */
struct telemetry_packet_t final
{
	std::uint8_t protocol_version;
	std::uint8_t node_id;
	std::uint16_t flags;
	std::int16_t primary_value;
	std::int16_t secondary_value;
	std::uint16_t potentiometer_raw;
	std::uint16_t duty_commanded;
	std::uint16_t reserved;
};

/**
 * @brief Event packet sent immediately on edge-triggered events.
 */
struct event_packet_t final
{
	std::uint8_t protocol_version;
	std::uint8_t node_id;
	std::uint8_t event_type;
	std::int16_t event_value;
	std::uint16_t timestamp_ms_mod;
};

/**
 * @brief Control packet written by control node to a sensor node.
 */
struct control_packet_t final
{
	std::uint8_t protocol_version;
	std::uint8_t target_node_id;
	std::uint16_t command_flags;
	std::uint16_t duty_override;
	std::uint16_t reserved;
};

/* Restore packing rules. */
#pragma pack(pop)

/*
 * Compile-time size checks.
 *
 * These ensure the BLE wire protocol cannot silently change due to:
 * - compiler differences
 * - edits to struct members
 * - changes to packing rules
 */
static_assert(sizeof(telemetry_packet_t) == 14u,
	      "telemetry_packet_t size changed");
static_assert(sizeof(event_packet_t) == 7u,
	      "event_packet_t size changed");
static_assert(sizeof(control_packet_t) == 8u,
	      "control_packet_t size changed");

/**
 * @brief Convert a telemetry flag to its underlying bit mask.
 * @param flag Flag to convert.
 * @return Bit mask for the flag.
 */
static inline constexpr std::uint16_t ble_telemetry_flag_mask(
    ble_telemetry_flag_t flag)
{
	return static_cast<std::uint16_t>(flag);
}

/**
 * @brief Convert a control flag to its underlying bit mask.
 * @param flag Flag to convert.
 * @return Bit mask for the flag.
 */
static inline constexpr std::uint16_t ble_control_flag_mask(
    ble_control_flag_t flag)
{
	return static_cast<std::uint16_t>(flag);
}

/**
 * @brief Test whether a telemetry flags field contains a specific flag.
 * @param flags Raw telemetry flags field.
 * @param flag Flag to test.
 * @return true if present, otherwise false.
 */
static inline constexpr bool ble_telemetry_flag_is_set(
    std::uint16_t flags,
    ble_telemetry_flag_t flag)
{
	return (flags & ble_telemetry_flag_mask(flag)) != 0u;
}

/**
 * @brief Set or clear a telemetry flag bit.
 * @param flags Raw telemetry flags field.
 * @param flag Flag to modify.
 * @param set true to set, false to clear.
 * @return Updated flags field.
 */
static inline constexpr std::uint16_t ble_telemetry_flag_update(
    std::uint16_t flags,
    ble_telemetry_flag_t flag,
    bool set)
{
	std::uint16_t updated = flags;

	if (set)
	{
		updated = static_cast<std::uint16_t>(updated |
						     ble_telemetry_flag_mask(flag));
	}
	else
	{
		updated = static_cast<std::uint16_t>(updated &
						     static_cast<std::uint16_t>(
							 ~ble_telemetry_flag_mask(flag)));
	}

	return updated;
}

/**
 * @brief Test whether a control flags field contains a specific flag.
 * @param flags Raw control flags field.
 * @param flag Flag to test.
 * @return true if present, otherwise false.
 */
static inline constexpr bool ble_control_flag_is_set(
    std::uint16_t flags,
    ble_control_flag_t flag)
{
	return (flags & ble_control_flag_mask(flag)) != 0u;
}

/**
 * @brief Clamp a duty value to 0..1000 per-mille.
 * @param duty_per_mille Duty request.
 * @return Clamped duty request.
 */
static inline constexpr std::uint16_t ble_clamp_duty_per_mille(
    std::uint16_t duty_per_mille)
{
	std::uint16_t clamped = duty_per_mille;

	if (clamped > ble_protocol_constants_t::duty_per_mille_max)
	{
		clamped = ble_protocol_constants_t::duty_per_mille_max;
	}

	return clamped;
}

/**
 * @brief Validate protocol version on a received packet buffer.
 * @param expected Expected protocol version.
 * @param buffer Source buffer.
 * @param buffer_size Buffer size in bytes.
 * @return true if valid, otherwise false.
 */
static inline bool ble_validate_protocol_version(
    ble_protocol_version_t expected,
    const std::uint8_t *buffer,
    std::size_t buffer_size)
{
	bool valid = true;

	if (buffer == nullptr)
	{
		valid = false;
	}
	else if (buffer_size < 1u)
	{
		valid = false;
	}
	else if (buffer[0] != static_cast<std::uint8_t>(expected))
	{
		valid = false;
	}
	else
	{
		valid = true;
	}

	return valid;
}

/**
 * @brief Serialise a telemetry packet into a byte buffer.
 * @param dst Destination buffer.
 * @param dst_size Destination buffer size in bytes.
 * @param src Packet to serialise.
 * @return true if written, otherwise false.
 */
static inline bool ble_pack_telemetry(
    std::uint8_t *dst,
    std::size_t dst_size,
    const telemetry_packet_t &src)
{
	bool ok = true;

	if (dst == nullptr)
	{
		ok = false;
	}
	else if (dst_size < sizeof(telemetry_packet_t))
	{
		ok = false;
	}
	else if (src.protocol_version !=
		 static_cast<std::uint8_t>(ble_protocol_version_t::v1))
	{
		ok = false;
	}
	else
	{
		std::memcpy(dst, &src, sizeof(telemetry_packet_t));
		ok = true;
	}

	return ok;
}

/**
 * @brief Serialise an event packet into a byte buffer.
 * @param dst Destination buffer.
 * @param dst_size Destination buffer size in bytes.
 * @param src Packet to serialise.
 * @return true if written, otherwise false.
 */
static inline bool ble_pack_event(
    std::uint8_t *dst,
    std::size_t dst_size,
    const event_packet_t &src)
{
	bool ok = true;

	if (dst == nullptr)
	{
		ok = false;
	}
	else if (dst_size < sizeof(event_packet_t))
	{
		ok = false;
	}
	else if (src.protocol_version !=
		 static_cast<std::uint8_t>(ble_protocol_version_t::v1))
	{
		ok = false;
	}
	else
	{
		std::memcpy(dst, &src, sizeof(event_packet_t));
		ok = true;
	}

	return ok;
}

/**
 * @brief Deserialise a control packet from a byte buffer.
 * @param dst Destination packet.
 * @param src Source buffer.
 * @param src_size Source buffer size in bytes.
 * @return true if parsed, otherwise false.
 */
static inline bool ble_unpack_control(
    control_packet_t &dst,
    const std::uint8_t *src,
    std::size_t src_size)
{
	bool ok = true;

	if (!ble_validate_protocol_version(ble_protocol_version_t::v1,
					   src, src_size))
	{
		ok = false;
	}
	else if (src_size < sizeof(control_packet_t))
	{
		ok = false;
	}
	else
	{
		std::memcpy(&dst, src, sizeof(control_packet_t));
		ok = true;
	}

	return ok;
}

/**
 * @brief Build a telemetry packet with required defaults.
 * @param node_id Node ID to embed.
 * @return Initialised packet.
 */
static inline telemetry_packet_t ble_make_telemetry(
    ble_node_id_t node_id)
{
	telemetry_packet_t pkt{};

	pkt.protocol_version =
	    static_cast<std::uint8_t>(ble_protocol_version_t::v1);
	pkt.node_id = static_cast<std::uint8_t>(node_id);
	pkt.flags = 0u;
	pkt.primary_value = 0;
	pkt.secondary_value = 0;
	pkt.potentiometer_raw = 0u;
	pkt.duty_commanded = 0u;
	pkt.reserved = 0u;

	return pkt;
}

/**
 * @brief Build an event packet with required defaults.
 * @param node_id Node ID to embed.
 * @param type Event type.
 * @param value Event value.
 * @param timestamp_ms_mod Timestamp modulo 65536.
 * @return Initialised packet.
 */
static inline event_packet_t ble_make_event(
    ble_node_id_t node_id,
    ble_event_type_t type,
    std::int16_t value,
    std::uint16_t timestamp_ms_mod)
{
	event_packet_t pkt{};

	pkt.protocol_version =
	    static_cast<std::uint8_t>(ble_protocol_version_t::v1);
	pkt.node_id = static_cast<std::uint8_t>(node_id);
	pkt.event_type = static_cast<std::uint8_t>(type);
	pkt.event_value = value;
	pkt.timestamp_ms_mod = timestamp_ms_mod;

	return pkt;
}

/**
 * @brief Build a control packet with required defaults.
 * @param target Target node ID.
 * @param flags Command flags.
 * @param duty_override Duty in per-mille 0..1000.
 * @return Initialised packet.
 */
static inline control_packet_t ble_make_control(
    ble_node_id_t target,
    std::uint16_t flags,
    std::uint16_t duty_override)
{
	control_packet_t pkt{};

	pkt.protocol_version =
	    static_cast<std::uint8_t>(ble_protocol_version_t::v1);
	pkt.target_node_id = static_cast<std::uint8_t>(target);
	pkt.command_flags = flags;
	pkt.duty_override = ble_clamp_duty_per_mille(duty_override);
	pkt.reserved = 0u;

	return pkt;
}

/*
 * Test vectors (little-endian).
 *
 * TELEM_1: SN2 telemetry
 * - protocol_version = 1
 * - node_id = 2
 * - flags = help_active
 * - primary_value = 2250 (22.50°C)
 * - secondary_value bit0 = 1 (sound detected)
 * - pot_raw = 2048
 * - duty_commanded = 500 (50%)
 */
static constexpr std::uint8_t BLE_TEST_TELEM_1[14] =
    {
	0x01u, 0x02u,
	0x01u, 0x00u,
	0xCAu, 0x08u,
	0x01u, 0x00u,
	0x00u, 0x08u,
	0xF4u, 0x01u,
	0x00u, 0x00u};

/*
 * EVENT_1: SN1 motion detected
 * - protocol_version = 1
 * - node_id = 1
 * - event_type = motion_detected
 * - event_value = 1
 * - timestamp = 0x1234
 */
static constexpr std::uint8_t BLE_TEST_EVENT_1[7] =
    {
	0x01u, 0x01u, 0x02u,
	0x01u, 0x00u,
	0x34u, 0x12u};

/*
 * CTRL_1: CN override enable for SN2, duty_override=750 (75%)
 * - protocol_version = 1
 * - target_node_id = 2
 * - command_flags = override_enable
 * - duty_override = 750
 */
static constexpr std::uint8_t BLE_TEST_CTRL_1[8] =
    {
	0x01u, 0x02u,
	0x01u, 0x00u,
	0xEEu, 0x02u,
	0x00u, 0x00u};
