/*
   pcap.h

   Writing frames to a .pcap file, the format tcpdump and Wireshark read.
   This lets the operator build a frame and then open it in Wireshark to
   see it fully decoded, without having to send anything on a network.

   The format is small: a 24-byte global header, then for each frame a
   16-byte record header (a timestamp and the length twice) followed by the
   frame bytes. All values are written in the host's byte order and the
   magic number tells the reader which order that was.
 */

#ifndef FRAMESMITH_PCAP_H
#define FRAMESMITH_PCAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* A writer owns an open .pcap file. */
typedef struct PcapWriter {
    FILE *file;
    bool owns_file; /* true when the writer opened it and must close it */
} PcapWriter;

/* Opens `path` for writing and writes the global header. The link type is
   LINKTYPE_ETHERNET (1), telling readers that each record is an Ethernet
   frame. Returns false if the file cannot be opened. */
bool pcap_open(PcapWriter *writer, const char *path);

/* Writes the global header to an already-open file (used by the tests with
   a temporary file). The writer does not take ownership of `file`. */
bool pcap_begin(PcapWriter *writer, FILE *file);

/* Appends one frame as a pcap record. Returns false on a write error. */
bool pcap_write_frame(PcapWriter *writer, const uint8_t *data, size_t length);

/* Flushes and, if the writer opened the file, closes it. */
void pcap_close(PcapWriter *writer);

#endif /* FRAMESMITH_PCAP_H */
