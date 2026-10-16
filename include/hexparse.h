/*
   hexparse.h

   Turning operator text into bytes. The operator describes a frame's
   payload in one of a few convenient notations, and MAC addresses and
   EtherTypes as hex; these helpers parse all of that, reporting the exact
   position and reason when the text is wrong.
 */

#ifndef FRAMESMITH_HEXPARSE_H
#define FRAMESMITH_HEXPARSE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* How to read the operator's payload text. */
typedef enum PayloadEncoding {
    PAYLOAD_HEX,   /* "de ad be ef" or "deadbeef" or "de:ad:be:ef": hex byte pairs */
    PAYLOAD_ASCII, /* the literal characters, so "Hi" becomes 0x48 0x69 */
    PAYLOAD_FILE   /* read the raw bytes of a file (handled by the caller) */
} PayloadEncoding;

/* The outcome of a parse attempt. On failure `message` explains what is
   wrong and `position` is the byte offset in the input where the problem
   is (for pointing at it), or -1 when no single spot applies. */
typedef struct ParseStatus {
    bool ok;
    char message[128];
    long position;
} ParseStatus;

/* Parses hex bytes from `text` into `out` (up to `capacity`). Spaces, tabs,
   ':' and '-' between bytes are ignored, so "aa:bb", "aa bb" and "aabb" are
   the same. Each byte must be exactly two hex digits. `*out_length` gets
   the number of bytes produced. */
ParseStatus parse_hex_bytes(const char *text, uint8_t *out, size_t capacity, size_t *out_length);

/* Copies the literal bytes of `text` into `out` (up to `capacity`). */
ParseStatus parse_ascii_bytes(const char *text, uint8_t *out, size_t capacity, size_t *out_length);

/* Parses a MAC address: six hex byte pairs separated by ':' or '-', or the
   names "broadcast" (ff:ff:ff:ff:ff:ff) and "zero". */
ParseStatus parse_mac(const char *text, uint8_t out[6]);

/* Parses a 16-bit EtherType written as hex ("0x0800", "0800" or "800") or
   as one of the names "ipv4", "ipv6", "arp", "vlan", "lldp", "wol". */
ParseStatus parse_ethertype(const char *text, uint16_t *out);

/* Parses an unsigned decimal number and checks it is within [min, max]. */
ParseStatus parse_uint(const char *text, unsigned long min, unsigned long max, unsigned long *out);

#endif /* FRAMESMITH_HEXPARSE_H */
