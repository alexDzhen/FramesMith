/*
   hexparse.c

   Parsing operator text into bytes, see hexparse.h. Every function returns
   a ParseStatus so the caller can show exactly what was wrong and where.
 */

#include "../include/hexparse.h"

#include <stdio.h>
#include <string.h>

/* Builds a successful status. */
static ParseStatus ok_status(void) {
    ParseStatus status;
    status.ok = true;
    status.message[0] = '\0';
    status.position = -1;
    return status;
}

/* Builds a failure status with a formatted message and a position. */
static ParseStatus fail_status(long position, const char *message) {
    ParseStatus status;
    status.ok = false;
    status.position = position;
    snprintf(status.message, sizeof(status.message), "%s", message);
    return status;
}

/* Value of one hex digit, or -1 if the character is not one. */
static int hex_digit(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

/* True for the characters allowed between hex bytes. */
static bool is_separator(char c) {
    return c == ' ' || c == '\t' || c == ':' || c == '-' || c == '\n' || c == '\r';
}

ParseStatus parse_hex_bytes(const char *text, uint8_t *out, size_t capacity, size_t *out_length) {
    size_t count = 0;
    const char *p = text;

    while (*p != '\0') {
        // Skip any run of separators between bytes.
        if (is_separator(*p)) {
            ++p;
            continue;
        }
        // A byte is exactly two hex digits.
        int high = hex_digit(p[0]);
        if (high < 0) {
            return fail_status((long)(p - text), "expected a hexadecimal digit");
        }
        int low = hex_digit(p[1]);
        if (low < 0) {
            return fail_status((long)((p + 1) - text), "each byte needs two hexadecimal digits");
        }
        if (count >= capacity) {
            return fail_status((long)(p - text), "too many bytes for the payload");
        }
        out[count++] = (uint8_t)(high * 16 + low);
        p += 2;
        // The next character is either the end of the text, a separator, or
        // the first digit of the next byte -- all fine. An odd digit count
        // (as in "aaa bb") is caught on the next pass, when the lone digit's
        // partner turns out to be a separator.
    }

    *out_length = count;
    return ok_status();
}

ParseStatus parse_ascii_bytes(const char *text, uint8_t *out, size_t capacity, size_t *out_length) {
    size_t length = strlen(text);
    if (length > capacity) {
        return fail_status((long)capacity, "the text is longer than the payload allows");
    }
    memcpy(out, text, length);
    *out_length = length;
    return ok_status();
}

ParseStatus parse_mac(const char *text, uint8_t out[6]) {
    // Two convenient names.
    if (strcmp(text, "broadcast") == 0 || strcmp(text, "bcast") == 0) {
        memset(out, 0xFF, 6);
        return ok_status();
    }
    if (strcmp(text, "zero") == 0) {
        memset(out, 0, 6);
        return ok_status();
    }

    // Otherwise exactly six two-digit groups separated by ':' or '-'.
    uint8_t bytes[6];
    const char *p = text;
    for (int i = 0; i < 6; ++i) {
        int high = hex_digit(p[0]);
        int low = high < 0 ? -1 : hex_digit(p[1]);
        if (low < 0) {
            return fail_status((long)(p - text), "a MAC address is six groups of two hex digits");
        }
        bytes[i] = (uint8_t)(high * 16 + low);
        p += 2;
        if (i < 5) {
            if (*p != ':' && *p != '-') {
                return fail_status((long)(p - text), "separate the groups with ':' or '-'");
            }
            ++p;
        }
    }
    if (*p != '\0') {
        return fail_status((long)(p - text), "a MAC address has exactly six groups");
    }
    memcpy(out, bytes, 6);
    return ok_status();
}

/* A table of EtherType names the operator may use instead of the number. */
static const struct {
    const char *name;
    uint16_t value;
} kEthertypeNames[] = {
    {"ipv4", 0x0800}, {"ip", 0x0800},   {"arp", 0x0806}, {"ipv6", 0x86DD},
    {"vlan", 0x8100}, {"lldp", 0x88CC}, {"wol", 0x0842}, {"profinet", 0x8892},
};

ParseStatus parse_ethertype(const char *text, uint16_t *out) {
    for (size_t i = 0; i < sizeof(kEthertypeNames) / sizeof(kEthertypeNames[0]); ++i) {
        if (strcmp(text, kEthertypeNames[i].name) == 0) {
            *out = kEthertypeNames[i].value;
            return ok_status();
        }
    }

    // A hex number, with or without a "0x" prefix.
    const char *p = text;
    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
        p += 2;
    }
    if (*p == '\0') {
        return fail_status(0, "expected an EtherType, e.g. 0x0800 or 'ipv4'");
    }
    unsigned long value = 0;
    int digits = 0;
    for (; *p != '\0'; ++p) {
        int digit = hex_digit(*p);
        if (digit < 0) {
            return fail_status((long)(p - text), "expected a hexadecimal digit or a known name");
        }
        value = value * 16 + (unsigned long)digit;
        if (++digits > 4) {
            return fail_status((long)(p - text),
                               "an EtherType is at most four hex digits (16 bits)");
        }
    }
    *out = (uint16_t)value;
    return ok_status();
}

ParseStatus parse_uint(const char *text, unsigned long min, unsigned long max, unsigned long *out) {
    if (text[0] == '\0') {
        return fail_status(0, "expected a number");
    }
    unsigned long value = 0;
    for (const char *p = text; *p != '\0'; ++p) {
        if (*p < '0' || *p > '9') {
            return fail_status((long)(p - text), "expected a decimal digit");
        }
        value = value * 10 + (unsigned long)(*p - '0');
        if (value > max) {
            char message[64];
            snprintf(message, sizeof(message), "the value must be at most %lu", max);
            return fail_status(0, message);
        }
    }
    if (value < min) {
        char message[64];
        snprintf(message, sizeof(message), "the value must be at least %lu", min);
        return fail_status(0, message);
    }
    *out = value;
    return ok_status();
}
