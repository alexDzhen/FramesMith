/*
   hexdump.c

   The hex dump and the one-line header summary, see hexdump.h.
 */

#include "../include/hexdump.h"

#include "../include/hexparse.h"
#include "../include/mac_fwd.h"

#include <string.h>

void hexdump(FILE *out, const uint8_t *data, size_t length) {
    // Sixteen bytes per line, in the classic offset / hex / ASCII layout.
    for (size_t offset = 0; offset < length; offset += 16) {
        fprintf(out, "%04zx  ", offset);

        // The hex columns. A gap after the eighth byte makes the line
        // easier to read; missing bytes on the last line become spaces so
        // the ASCII column still lines up.
        for (size_t i = 0; i < 16; ++i) {
            if (offset + i < length) {
                fprintf(out, "%02x ", data[offset + i]);
            } else {
                fputs("   ", out);
            }
            if (i == 7) {
                fputc(' ', out);
            }
        }

        // The ASCII column: printable bytes as themselves, the rest as '.'.
        fputc(' ', out);
        for (size_t i = 0; i < 16 && offset + i < length; ++i) {
            uint8_t byte = data[offset + i];
            fputc(byte >= 0x20 && byte < 0x7F ? (int)byte : '.', out);
        }
        fputc('\n', out);
    }
}

/* The name for a well-known EtherType, or NULL. The value is also the
   payload length when it is 1500 or less (the original Ethernet meaning),
   which the caller notes separately. */
static const char *ethertype_name(uint16_t type) {
    switch (type) {
    case 0x0800:
        return "IPv4";
    case 0x0806:
        return "ARP";
    case 0x86DD:
        return "IPv6";
    case 0x8100:
        return "802.1Q VLAN";
    case 0x88CC:
        return "LLDP";
    case 0x0842:
        return "Wake-on-LAN";
    case 0x8892:
        return "PROFINET";
    default:
        return NULL;
    }
}

void frame_describe(FILE *out, const Frame *frame) {
    char dst[18];
    char src[18];
    mac_to_text(&frame->bytes[0], dst);
    mac_to_text(&frame->bytes[6], src);
    fprintf(out, "%s -> %s", src, dst);

    // The EtherType sits at offset 12, or 16 when a VLAN tag is present
    // (its tag protocol id 0x8100 is what marks that).
    size_t type_offset = 12;
    bool has_vlan = frame->length >= 16 && frame->bytes[12] == 0x81 && frame->bytes[13] == 0x00;
    if (has_vlan) {
        uint16_t tci = (uint16_t)(frame->bytes[14] << 8 | frame->bytes[15]);
        fprintf(out, "  VLAN id=%u pcp=%u", tci & 0x0FFF, (tci >> 13) & 0x7);
        type_offset = 16;
    }

    uint16_t type = (uint16_t)(frame->bytes[type_offset] << 8 | frame->bytes[type_offset + 1]);
    const char *name = ethertype_name(type);
    fprintf(out, "  type=0x%04x", type);
    if (name != NULL) {
        fprintf(out, " (%s)", name);
    } else if (type <= 1500) {
        // Values up to 1500 are a length in the older IEEE 802.3 framing.
        fprintf(out, " (length %u)", type);
    }

    size_t header = type_offset + 2;
    fprintf(out, "  payload=%zu bytes  total=%zu bytes\n", frame->length - header, frame->length);
}
