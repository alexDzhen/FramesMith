/*
   test_hexdump.c -- tests for the hex dump and the header summary
   (src/hexdump.c).
 */
#include "greatest.h"

#include <string.h>

#include "capture.h"
#include "../include/frame.h"
#include "../include/hexdump.h"

/* The dump has the offset / hex / ASCII layout with the mid-line gap and
   non-printable bytes shown as dots. */
TEST dump_has_offset_hex_and_ascii(void) {
    uint8_t data[18];
    for (size_t i = 0; i < 16; ++i) {
        data[i] = (uint8_t)i;
    }
    data[16] = 'H';
    data[17] = 'i';

    FILE *out = capture_tmpfile();
    hexdump(out, data, sizeof(data));
    char *text = capture_contents(out);
    fclose(out);

    ASSERT_STR_EQ("0000  00 01 02 03 04 05 06 07  08 09 0a 0b 0c 0d 0e 0f  ................\n"
                  "0010  48 69                                             Hi\n",
                  text);
    free(text);
    PASS();
}

/* A printable payload shows up in the ASCII column. */
TEST dump_shows_printable_ascii(void) {
    const uint8_t data[] = "ABC~ ";
    FILE *out = capture_tmpfile();
    hexdump(out, data, 5);
    char *text = capture_contents(out);
    fclose(out);
    ASSERT(strstr(text, "ABC~ ") != NULL);
    free(text);
    PASS();
}

/* The summary names the MACs, the EtherType and the payload length. */
TEST describes_a_plain_frame(void) {
    FrameSpec spec;
    memset(&spec, 0, sizeof(spec));
    memset(spec.destination, 0xFF, 6);
    spec.source[0] = 0x02;
    spec.source[5] = 0x01;
    spec.ethertype = 0x0806;
    spec.payload_length = 4;
    spec.pad_to_minimum = false;

    Frame frame;
    build_frame(&spec, &frame);
    FILE *out = capture_tmpfile();
    frame_describe(out, &frame);
    char *text = capture_contents(out);
    fclose(out);

    ASSERT_STR_EQ("02:00:00:00:00:01 -> ff:ff:ff:ff:ff:ff  type=0x0806 (ARP)  payload=4 bytes  "
                  "total=18 bytes\n",
                  text);
    free(text);
    PASS();
}

/* With a VLAN tag the summary reports the id and priority and still finds
   the EtherType after the tag. */
TEST describes_a_vlan_frame(void) {
    FrameSpec spec;
    memset(&spec, 0, sizeof(spec));
    memset(spec.destination, 0xFF, 6);
    spec.ethertype = 0x0800;
    spec.has_vlan = true;
    spec.vlan_id = 100;
    spec.vlan_priority = 3;
    spec.payload_length = 4;
    spec.pad_to_minimum = false;

    Frame frame;
    build_frame(&spec, &frame);
    FILE *out = capture_tmpfile();
    frame_describe(out, &frame);
    char *text = capture_contents(out);
    fclose(out);

    ASSERT(strstr(text, "VLAN id=100 pcp=3") != NULL);
    ASSERT(strstr(text, "type=0x0800 (IPv4)") != NULL);
    free(text);
    PASS();
}

/* An EtherType of 1500 or less is shown as a length (802.3 framing). */
TEST describes_length_field(void) {
    FrameSpec spec;
    memset(&spec, 0, sizeof(spec));
    spec.ethertype = 46;
    spec.payload_length = 46;
    spec.pad_to_minimum = false;
    Frame frame;
    build_frame(&spec, &frame);

    FILE *out = capture_tmpfile();
    frame_describe(out, &frame);
    char *text = capture_contents(out);
    fclose(out);
    ASSERT(strstr(text, "(length 46)") != NULL);
    free(text);
    PASS();
}

SUITE(hexdump_suite) {
    RUN_TEST(dump_has_offset_hex_and_ascii);
    RUN_TEST(dump_shows_printable_ascii);
    RUN_TEST(describes_a_plain_frame);
    RUN_TEST(describes_a_vlan_frame);
    RUN_TEST(describes_length_field);
}
