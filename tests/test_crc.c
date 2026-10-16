/*
   test_crc.c -- tests for the Ethernet CRC-32 (src/frame.c).

   The expected values are the standard CRC-32 (the one zlib and Ethernet
   use), which can be checked against any independent implementation.
 */
#include "greatest.h"

#include <string.h>

#include "../include/frame.h"

/* The canonical check value: CRC-32 of "123456789" is 0xCBF43926. */
TEST matches_the_standard_check_value(void) {
    const uint8_t data[] = "123456789";
    ASSERT_EQ_FMT(0xCBF43926u, frame_crc32(data, 9), "%08x");
    PASS();
}

/* Known small cases. */
TEST matches_known_small_inputs(void) {
    ASSERT_EQ_FMT(0x00000000u, frame_crc32(NULL, 0), "%08x");
    const uint8_t one_zero[] = {0x00};
    ASSERT_EQ_FMT(0xD202EF8Du, frame_crc32(one_zero, 1), "%08x");
    const uint8_t abc[] = "abc";
    ASSERT_EQ_FMT(0x352441C2u, frame_crc32(abc, 3), "%08x");
    PASS();
}

/* Changing a single bit changes the CRC (its whole point). */
TEST detects_a_one_bit_change(void) {
    uint8_t data[20];
    memset(data, 0x55, sizeof(data));
    uint32_t before = frame_crc32(data, sizeof(data));
    data[10] ^= 0x01;
    uint32_t after = frame_crc32(data, sizeof(data));
    ASSERT(before != after);
    PASS();
}

SUITE(crc_suite) {
    RUN_TEST(matches_the_standard_check_value);
    RUN_TEST(matches_known_small_inputs);
    RUN_TEST(detects_a_one_bit_change);
}
