/*
   pcap.c

   Writing frames to a .pcap file, see pcap.h. The structures are written
   field by field with fwrite so the on-disk layout does not depend on how
   the compiler would pad a struct.
 */

#include "../include/pcap.h"

#include <string.h>

/* The classic pcap magic number. Writing it in the host's byte order lets a
   reader detect that order: it sees 0xa1b2c3d4 if it uses the same order,
   or the byte-swapped 0xd4c3b2a1 if not. Microsecond timestamps.
   These are #defines rather than an enum because the magic number does not
   fit in an int, and a plain enum is limited to int range in standard C. */
#define PCAP_MAGIC 0xA1B2C3D4u
#define PCAP_VERSION_MAJOR 2u
#define PCAP_VERSION_MINOR 4u
#define PCAP_SNAPLEN 262144u      /* maximum stored frame length */
#define PCAP_LINKTYPE_ETHERNET 1u /* each record is an Ethernet frame */

/* fwrite one 32-bit and one 16-bit value in host byte order. */
static bool write_u32(FILE *file, uint32_t value) {
    return fwrite(&value, sizeof(value), 1, file) == 1;
}
static bool write_u16(FILE *file, uint16_t value) {
    return fwrite(&value, sizeof(value), 1, file) == 1;
}

/* Writes the 24-byte global header that starts every pcap file. */
static bool write_global_header(FILE *file) {
    bool ok = write_u32(file, PCAP_MAGIC);
    ok = ok && write_u16(file, PCAP_VERSION_MAJOR);
    ok = ok && write_u16(file, PCAP_VERSION_MINOR);
    ok = ok && write_u32(file, 0); /* timezone offset, always 0 */
    ok = ok && write_u32(file, 0); /* timestamp accuracy, always 0 */
    ok = ok && write_u32(file, PCAP_SNAPLEN);
    ok = ok && write_u32(file, PCAP_LINKTYPE_ETHERNET);
    return ok;
}

bool pcap_open(PcapWriter *writer, const char *path) {
    // "wb": binary mode matters on Windows, where text mode would turn
    // every 0x0A byte into 0x0D 0x0A and corrupt the file.
    writer->file = fopen(path, "wb");
    if (writer->file == NULL) {
        return false;
    }
    writer->owns_file = true;
    if (!write_global_header(writer->file)) {
        pcap_close(writer);
        return false;
    }
    return true;
}

bool pcap_begin(PcapWriter *writer, FILE *file) {
    writer->file = file;
    writer->owns_file = false;
    return write_global_header(file);
}

bool pcap_write_frame(PcapWriter *writer, const uint8_t *data, size_t length) {
    // Each frame gets a 16-byte record header: a timestamp (left at zero
    // here, since the frame was generated, not captured at a real moment)
    // and the length stored twice -- the captured length and the original
    // length, which are equal because nothing is truncated.
    FILE *file = writer->file;
    bool ok = write_u32(file, 0);  /* timestamp seconds */
    ok = ok && write_u32(file, 0); /* timestamp microseconds */
    ok = ok && write_u32(file, (uint32_t)length);
    ok = ok && write_u32(file, (uint32_t)length);
    ok = ok && fwrite(data, 1, length, file) == length;
    return ok;
}

void pcap_close(PcapWriter *writer) {
    if (writer->file == NULL) {
        return;
    }
    fflush(writer->file);
    if (writer->owns_file) {
        fclose(writer->file);
    }
    writer->file = NULL;
}
