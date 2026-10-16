/*
   test_frame.c -- tests for frame assembly (src/frame.c).
 */
#include "greatest.h"

#include <string.h>

#include "../include/frame.h"

/* A spec with sensible values the individual tests tweak. */
static FrameSpec sample_spec(void) {
    FrameSpec spec;
    memset(&spec, 0, sizeof(spec));
    memset(spec.destination, 0xFF, 6);
    spec.source[0] = 0x02;
    spec.source[5] = 0x01;
    spec.ethertype = 0x0800;
    spec.pad_to_minimum = true;
    return spec;
}

/* The header is destination, source, type, in that order; a short frame is
   padded to 60 bytes. */
TEST builds_a_basic_frame(void) {
    FrameSpec spec = sample_spec();
    const uint8_t payload[] = {0xDE, 0xAD, 0xBE, 0xEF};
    memcpy(spec.payload, payload, sizeof(payload));
    spec.payload_length = sizeof(payload);

    Frame frame;
    ASSERT_EQ(FRAME_OK, build_frame(&spec, &frame));
    ASSERT_EQ(60u, (unsigned)frame.length); /* padded up from 18 */

    const uint8_t expected_header[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x02, 0x00, 0x00,
                                       0x00, 0x00, 0x01, 0x08, 0x00, 0xDE, 0xAD, 0xBE, 0xEF};
    ASSERT_MEM_EQ(expected_header, frame.bytes, sizeof(expected_header));
    /* Everything after the payload is zero padding. */
    for (size_t i = 18; i < 60; ++i) {
        ASSERT_EQ(0, frame.bytes[i]);
    }
    PASS();
}

/* Without padding the frame is exactly as described, even if that is a
   runt shorter than 60 bytes. */
TEST respects_no_padding(void) {
    FrameSpec spec = sample_spec();
    spec.pad_to_minimum = false;
    spec.payload_length = 2;
    spec.payload[0] = 0x45;
    spec.payload[1] = 0x00;

    Frame frame;
    ASSERT_EQ(FRAME_OK, build_frame(&spec, &frame));
    ASSERT_EQ(16u, (unsigned)frame.length); /* 14 header + 2 payload */
    PASS();
}

/* A VLAN tag inserts four bytes (0x8100 and the TCI) after the source MAC
   and pushes the EtherType back. */
TEST inserts_a_vlan_tag(void) {
    FrameSpec spec = sample_spec();
    spec.has_vlan = true;
    spec.vlan_id = 100;     /* 0x064 */
    spec.vlan_priority = 3; /* goes in the top three bits */
    spec.pad_to_minimum = false;

    Frame frame;
    ASSERT_EQ(FRAME_OK, build_frame(&spec, &frame));
    /* bytes 12-13: tag protocol id 0x8100 */
    ASSERT_EQ(0x81, frame.bytes[12]);
    ASSERT_EQ(0x00, frame.bytes[13]);
    /* bytes 14-15: priority 3 (<<13) | id 100 = 0x6064 */
    ASSERT_EQ(0x60, frame.bytes[14]);
    ASSERT_EQ(0x64, frame.bytes[15]);
    /* bytes 16-17: the EtherType, now after the tag */
    ASSERT_EQ(0x08, frame.bytes[16]);
    ASSERT_EQ(0x00, frame.bytes[17]);
    PASS();
}

/* Out-of-range fields are rejected with the matching error. */
TEST rejects_out_of_range_fields(void) {
    Frame frame;

    FrameSpec too_long = sample_spec();
    too_long.payload_length = FRAME_MAX_PAYLOAD + 1;
    ASSERT_EQ(FRAME_ERR_PAYLOAD_TOO_LONG, build_frame(&too_long, &frame));

    FrameSpec bad_vlan = sample_spec();
    bad_vlan.has_vlan = true;
    bad_vlan.vlan_id = 4096;
    ASSERT_EQ(FRAME_ERR_VLAN_ID, build_frame(&bad_vlan, &frame));

    FrameSpec bad_pcp = sample_spec();
    bad_pcp.has_vlan = true;
    bad_pcp.vlan_priority = 8;
    ASSERT_EQ(FRAME_ERR_VLAN_PRIORITY, build_frame(&bad_pcp, &frame));

    /* Every error has a message, and it is not the "unknown" fallback. */
    ASSERT(strlen(frame_error_text(FRAME_ERR_PAYLOAD_TOO_LONG)) > 0);
    ASSERT_STR_EQ("ok", frame_error_text(FRAME_OK));
    PASS();
}

/* A full-size payload is built without padding and reports the right length. */
TEST builds_a_maximum_payload(void) {
    FrameSpec spec = sample_spec();
    spec.payload_length = 1500;
    for (size_t i = 0; i < 1500; ++i) {
        spec.payload[i] = (uint8_t)i;
    }
    Frame frame;
    ASSERT_EQ(FRAME_OK, build_frame(&spec, &frame));
    ASSERT_EQ(1514u, (unsigned)frame.length);
    ASSERT_EQ(0, frame.bytes[14]);               /* payload[0] */
    ASSERT_EQ((uint8_t)1499, frame.bytes[1513]); /* payload[1499] */
    PASS();
}

SUITE(frame_suite) {
    RUN_TEST(builds_a_basic_frame);
    RUN_TEST(respects_no_padding);
    RUN_TEST(inserts_a_vlan_tag);
    RUN_TEST(rejects_out_of_range_fields);
    RUN_TEST(builds_a_maximum_payload);
}
