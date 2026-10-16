/*
   test_hexparse.c -- tests for the text parsers (src/hexparse.c).
 */
#include "greatest.h"

#include <string.h>

#include "../include/hexparse.h"

/* Hex bytes accept spaces, colons, dashes or nothing between bytes. */
TEST parses_hex_in_several_notations(void) {
    uint8_t out[8];
    size_t length;

    ASSERT(parse_hex_bytes("deadbeef", out, sizeof(out), &length).ok);
    ASSERT_EQ(4u, (unsigned)length);
    ASSERT_EQ(0xDE, out[0]);
    ASSERT_EQ(0xEF, out[3]);

    ASSERT(parse_hex_bytes("de ad be ef", out, sizeof(out), &length).ok);
    ASSERT_EQ(4u, (unsigned)length);
    ASSERT(parse_hex_bytes("DE:AD:BE:EF", out, sizeof(out), &length).ok);
    ASSERT_EQ(4u, (unsigned)length);
    ASSERT(parse_hex_bytes("", out, sizeof(out), &length).ok);
    ASSERT_EQ(0u, (unsigned)length);
    PASS();
}

/* Bad hex is rejected and the position points at the problem. */
TEST reports_bad_hex_position(void) {
    uint8_t out[8];
    size_t length;

    ParseStatus odd = parse_hex_bytes("dead b", out, sizeof(out), &length);
    ASSERT_FALSE(odd.ok);
    ASSERT_EQ(6, odd.position); /* the missing second digit of "b" */

    ParseStatus bad = parse_hex_bytes("dexd", out, sizeof(out), &length);
    ASSERT_FALSE(bad.ok);
    ASSERT_EQ(2, bad.position); /* the 'x' at index 2 is not a hex digit */

    /* An odd number of digits (a lone digit before a separator) is caught. */
    ParseStatus triple = parse_hex_bytes("aaa bb", out, sizeof(out), &length);
    ASSERT_FALSE(triple.ok);

    /* Overflowing the buffer is an error, not a silent truncation. */
    ParseStatus overflow = parse_hex_bytes("00112233", out, 3, &length);
    ASSERT_FALSE(overflow.ok);
    PASS();
}

/* ASCII copies the bytes verbatim. */
TEST parses_ascii(void) {
    uint8_t out[16];
    size_t length;
    ASSERT(parse_ascii_bytes("Hi!", out, sizeof(out), &length).ok);
    ASSERT_EQ(3u, (unsigned)length);
    ASSERT_EQ('H', out[0]);
    ASSERT_EQ('!', out[2]);
    ASSERT_FALSE(parse_ascii_bytes("way too long", out, 4, &length).ok);
    PASS();
}

/* MAC addresses: both separators, the two names, and the error cases. */
TEST parses_mac_addresses(void) {
    uint8_t mac[6];

    ASSERT(parse_mac("02:fc:00:0a:bc:ff", mac).ok);
    ASSERT_EQ(0x02, mac[0]);
    ASSERT_EQ(0xFF, mac[5]);
    ASSERT(parse_mac("02-FC-00-0A-BC-FF", mac).ok);
    ASSERT_EQ(0xBC, mac[4]);

    ASSERT(parse_mac("broadcast", mac).ok);
    for (int i = 0; i < 6; ++i) {
        ASSERT_EQ(0xFF, mac[i]);
    }
    ASSERT(parse_mac("zero", mac).ok);
    ASSERT_EQ(0, mac[0]);

    ASSERT_FALSE(parse_mac("02:fc:00:0a:bc", mac).ok);       /* five groups */
    ASSERT_FALSE(parse_mac("02:fc:00:0a:bc:ff:11", mac).ok); /* seven groups */
    ASSERT_FALSE(parse_mac("gg:fc:00:0a:bc:ff", mac).ok);    /* not hex */
    PASS();
}

/* EtherTypes: names, hex with and without 0x, and errors. */
TEST parses_ethertypes(void) {
    uint16_t type;
    ASSERT(parse_ethertype("ipv4", &type).ok);
    ASSERT_EQ(0x0800, type);
    ASSERT(parse_ethertype("arp", &type).ok);
    ASSERT_EQ(0x0806, type);
    ASSERT(parse_ethertype("ipv6", &type).ok);
    ASSERT_EQ(0x86DD, type);
    ASSERT(parse_ethertype("0x88cc", &type).ok);
    ASSERT_EQ(0x88CC, type);
    ASSERT(parse_ethertype("800", &type).ok);
    ASSERT_EQ(0x0800, type);
    ASSERT_FALSE(parse_ethertype("12345", &type).ok); /* more than 16 bits */
    ASSERT_FALSE(parse_ethertype("nope", &type).ok);
    ASSERT_FALSE(parse_ethertype("", &type).ok);
    PASS();
}

/* Bounded decimal parsing. */
TEST parses_bounded_numbers(void) {
    unsigned long value;
    ASSERT(parse_uint("0", 0, 4095, &value).ok);
    ASSERT_EQ(0u, value);
    ASSERT(parse_uint("4095", 0, 4095, &value).ok);
    ASSERT_EQ(4095u, value);
    ASSERT_FALSE(parse_uint("4096", 0, 4095, &value).ok);
    ASSERT_FALSE(parse_uint("", 0, 4095, &value).ok);
    ASSERT_FALSE(parse_uint("12x", 0, 4095, &value).ok);
    ASSERT_FALSE(parse_uint("3", 5, 7, &value).ok); /* below the minimum */
    PASS();
}

SUITE(hexparse_suite) {
    RUN_TEST(parses_hex_in_several_notations);
    RUN_TEST(reports_bad_hex_position);
    RUN_TEST(parses_ascii);
    RUN_TEST(parses_mac_addresses);
    RUN_TEST(parses_ethertypes);
    RUN_TEST(parses_bounded_numbers);
}
