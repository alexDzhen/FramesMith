/*
   frame.c

   Assembling an Ethernet frame from a FrameSpec, and the CRC-32 the
   hardware uses as the frame check sequence. See frame.h for the layout.
 */

#include "../include/frame.h"

#include <string.h>

const char *frame_error_text(FrameError error) {
    switch (error) {
    case FRAME_OK:
        return "ok";
    case FRAME_ERR_PAYLOAD_TOO_LONG:
        return "the payload is longer than the maximum frame allows";
    case FRAME_ERR_VLAN_ID:
        return "the VLAN id must be between 0 and 4095";
    case FRAME_ERR_VLAN_PRIORITY:
        return "the VLAN priority must be between 0 and 7";
    }
    return "unknown error";
}

/* Writes a 16-bit value big-endian (network byte order), the order every
   field on the wire uses. */
static void put_be16(uint8_t *out, uint16_t value) {
    out[0] = (uint8_t)(value >> 8);
    out[1] = (uint8_t)(value & 0xFF);
}

FrameError build_frame(const FrameSpec *spec, Frame *out) {
    // Validate the few fields that can be out of range. A MAC address
    // cannot be invalid (any six bytes are one), so only these are checked.
    if (spec->payload_length > FRAME_MAX_PAYLOAD) {
        return FRAME_ERR_PAYLOAD_TOO_LONG;
    }
    if (spec->has_vlan && spec->vlan_id > 4095) {
        return FRAME_ERR_VLAN_ID;
    }
    if (spec->has_vlan && spec->vlan_priority > 7) {
        return FRAME_ERR_VLAN_PRIORITY;
    }

    size_t pos = 0;

    // Destination and source MAC: 6 bytes each, copied verbatim.
    memcpy(&out->bytes[pos], spec->destination, 6);
    pos += 6;
    memcpy(&out->bytes[pos], spec->source, 6);
    pos += 6;

    // An 802.1Q VLAN tag, when asked for, goes here: the tag protocol id
    // 0x8100, then two bytes packing priority (3 bits), a drop-eligible
    // bit (0 here) and the 12-bit VLAN id.
    if (spec->has_vlan) {
        put_be16(&out->bytes[pos], 0x8100);
        pos += 2;
        uint16_t tci = (uint16_t)((spec->vlan_priority & 0x7) << 13 | (spec->vlan_id & 0x0FFF));
        put_be16(&out->bytes[pos], tci);
        pos += 2;
    }

    // The EtherType: what the payload is (0x0800 = IPv4, 0x0806 = ARP, ...).
    put_be16(&out->bytes[pos], spec->ethertype);
    pos += 2;

    // The payload itself.
    memcpy(&out->bytes[pos], spec->payload, spec->payload_length);
    pos += spec->payload_length;

    // Pad with zeros up to the 60-byte minimum, as the hardware would, so
    // the frame is not an illegal "runt". The operator can switch this off.
    if (spec->pad_to_minimum && pos < FRAME_MIN_SIZE) {
        memset(&out->bytes[pos], 0, FRAME_MIN_SIZE - pos);
        pos = FRAME_MIN_SIZE;
    }

    out->length = pos;
    return FRAME_OK;
}

/* The CRC-32 table, built once on first use. Computing the table instead of
   writing out 256 constants keeps the polynomial visible and the code short.
   The polynomial 0xEDB88320 is the bit-reversed form of the standard
   Ethernet CRC polynomial. */
static uint32_t g_crc_table[256];
static bool g_crc_table_ready = false;

static void build_crc_table(void) {
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t crc = i;
        for (int bit = 0; bit < 8; ++bit) {
            // If the low bit is set, shift and XOR in the polynomial;
            // otherwise just shift. This is long division in GF(2).
            crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320u : crc >> 1;
        }
        g_crc_table[i] = crc;
    }
    g_crc_table_ready = true;
}

uint32_t frame_crc32(const uint8_t *data, size_t length) {
    if (!g_crc_table_ready) {
        build_crc_table();
    }
    // The CRC starts all-ones, each byte indexes the table, and the result
    // is inverted at the end -- the standard Ethernet/zlib convention.
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < length; ++i) {
        crc = g_crc_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFu;
}
