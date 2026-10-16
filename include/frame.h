/*
   frame.h

   The Ethernet frame model and the code that turns the operator's choices
   into the exact bytes that go on the wire.

   An Ethernet II frame looks like this (lengths in bytes):

     +-------------------+-------------------+-----------+--------------+
     | Destination MAC 6 | Source MAC      6 | Type    2 | Payload 46.. |
     +-------------------+-------------------+-----------+--------------+

   The 4-byte CRC (FCS) that follows the payload on the wire is added and
   checked by the network card, not by software, so it is not part of the
   bytes a program hands to the operating system; framesmith can compute it
   for display but never puts it in the sent frame.

   An optional IEEE 802.1Q VLAN tag inserts four bytes after the source MAC:

     ... | Source MAC 6 | 0x8100 | PCP/DEI/VID 2 | Type 2 | Payload ... |

   Everything in this file is pure byte assembly with no system calls, so it
   compiles everywhere and is fully unit-tested. Sending the finished bytes
   is a separate, platform-specific step (see sender.h).
 */

#ifndef FRAMESMITH_FRAME_H
#define FRAMESMITH_FRAME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* A standard Ethernet frame carries 46..1500 payload bytes. The header is
   14 bytes (18 with a VLAN tag), so the largest frame body we build is
   1518 bytes; a generous buffer leaves room for oversized ("jumbo") frames
   an operator may ask for on purpose. The 4-byte CRC is not stored here. */
#define FRAME_MAX_PAYLOAD 9000
#define FRAME_HEADER_SIZE 14
#define FRAME_VLAN_TAG_SIZE 4
#define FRAME_MAX_SIZE (FRAME_HEADER_SIZE + FRAME_VLAN_TAG_SIZE + FRAME_MAX_PAYLOAD)
/* The smallest valid Ethernet frame body is 60 bytes (before the CRC);
   shorter frames are padded to this length by the hardware. */
#define FRAME_MIN_SIZE 60

/* The operator's description of a frame, before it is turned into bytes.
   Every field here corresponds to something the operator sets. */
typedef struct FrameSpec {
    uint8_t destination[6]; /* who the frame is addressed to */
    uint8_t source[6];      /* who it claims to be from */
    uint16_t ethertype;     /* 0x0800 = IPv4, 0x0806 = ARP, 0x86DD = IPv6, ... */

    bool has_vlan;         /* insert an 802.1Q tag? */
    uint16_t vlan_id;      /* VLAN id, 0..4095 */
    uint8_t vlan_priority; /* priority code point, 0..7 */

    uint8_t payload[FRAME_MAX_PAYLOAD];
    size_t payload_length;

    /* When true, build_frame pads a short frame with zero bytes up to the
       60-byte minimum, the way the hardware would. When false the frame is
       left exactly as described, which is useful for seeing the real length
       or for deliberately building an undersized ("runt") frame. */
    bool pad_to_minimum;
} FrameSpec;

/* The assembled bytes, ready to display or send. */
typedef struct Frame {
    uint8_t bytes[FRAME_MAX_SIZE];
    size_t length;
} Frame;

/* Why a spec could not be turned into a frame. */
typedef enum FrameError {
    FRAME_OK,
    FRAME_ERR_PAYLOAD_TOO_LONG, /* payload_length exceeds FRAME_MAX_PAYLOAD */
    FRAME_ERR_VLAN_ID,          /* vlan_id > 4095 */
    FRAME_ERR_VLAN_PRIORITY     /* vlan_priority > 7 */
} FrameError;

/* A human-readable message for a FrameError. */
const char *frame_error_text(FrameError error);

/* Assembles the frame described by `spec` into `out`. Returns FRAME_OK on
 * success, or the reason the spec was rejected. The source and destination
 * MACs are never range-checked (every 6-byte value is a valid address);
 * only the VLAN fields and the payload length can be out of range. */
FrameError build_frame(const FrameSpec *spec, Frame *out);

/* The IEEE 802.3 frame check sequence (CRC-32) of `length` bytes, the value
 * the hardware appends as the FCS. Provided so the operator can see it;
 * build_frame never includes it. */
uint32_t frame_crc32(const uint8_t *data, size_t length);

#endif /* FRAMESMITH_FRAME_H */
