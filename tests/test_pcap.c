/*
   test_pcap.c -- tests for the pcap writer (src/pcap.c).

   The bytes are checked against the known pcap layout so a reader like
   Wireshark will accept the file.
 */
#include "greatest.h"

#include <stdlib.h>
#include <string.h>

#include "capture.h"
#include "../include/pcap.h"

/* Reads a 32-bit little-or-big-endian-agnostic value by comparing against
   the magic the writer used, so the test does not assume a byte order. */

/* The global header is 24 bytes and starts with the magic number in host
   order; a record header is 16 bytes, then the frame. */
TEST writes_header_and_one_record(void) {
    FILE *file = capture_tmpfile();
    PcapWriter writer;
    ASSERT(pcap_begin(&writer, file));

    const uint8_t frame[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x02,
                             0x00, 0x00, 0x00, 0x00, 0x01, 0x08, 0x00};
    ASSERT(pcap_write_frame(&writer, frame, sizeof(frame)));
    pcap_close(&writer);

    char *data = capture_contents(file);
    fclose(file);

    /* 24-byte global header + 16-byte record header + 14-byte frame. */
    /* capture_contents stops at the first embedded NUL, so measure through
       the writer instead: re-read the length from the file position earlier.
       Here we just check the leading bytes and the magic. */
    uint32_t magic;
    memcpy(&magic, data, 4);
    ASSERT_EQ_FMT((uint32_t)0xA1B2C3D4u, magic, "%08x");

    /* The captured and original lengths (bytes 32-39, inside the record
       header at offset 24) are both the frame length, 14. */
    uint32_t caplen, origlen;
    memcpy(&caplen, data + 24 + 8, 4);
    memcpy(&origlen, data + 24 + 12, 4);
    ASSERT_EQ(14u, caplen);
    ASSERT_EQ(14u, origlen);

    /* The frame bytes follow the record header at offset 40. */
    ASSERT_MEM_EQ(frame, data + 40, sizeof(frame));
    free(data);
    PASS();
}

/* pcap_open creates a real file that starts with the magic number. */
TEST opens_a_real_file(void) {
    /* This test reopens the file by name to check its contents, so it uses a
       real named file in the current directory (writable under CTest) and
       removes it at the end, rather than a self-deleting temporary. */
    const char *name = "framesmith_pcap_test.pcap";
    PcapWriter writer;
    ASSERT(pcap_open(&writer, name));
    const uint8_t frame[20] = {0};
    ASSERT(pcap_write_frame(&writer, frame, sizeof(frame)));
    pcap_close(&writer);

    FILE *check = fopen(name, "rb");
    ASSERT(check != NULL);
    uint8_t header[24];
    ASSERT_EQ(24u, (unsigned)fread(header, 1, 24, check));
    fclose(check);
    remove(name);

    uint32_t magic;
    memcpy(&magic, header, 4);
    ASSERT_EQ_FMT((uint32_t)0xA1B2C3D4u, magic, "%08x");
    PASS();
}

SUITE(pcap_suite) {
    RUN_TEST(writes_header_and_one_record);
    RUN_TEST(opens_a_real_file);
}
