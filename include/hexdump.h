/*
   hexdump.h

   Showing a frame to the operator: a classic hex-plus-ASCII dump and a
   one-line decoded summary of the header fields. Both write to a FILE* so
   the tests can capture and compare the output.
 */

#ifndef FRAMESMITH_HEXDUMP_H
#define FRAMESMITH_HEXDUMP_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "frame.h"

/* Prints `length` bytes as a hex dump: an offset, sixteen hex byte values
   and the printable ASCII for each line, e.g.

     0000  ff ff ff ff ff ff 02 fc 00 00 00 01 08 00 48 69  ..............Hi

   Non-printable bytes show as '.' in the ASCII column. */
void hexdump(FILE *out, const uint8_t *data, size_t length);

/* Prints a one-line human reading of the frame's header: the destination
   and source MACs, the VLAN tag if present, the EtherType with its name,
   and the payload length. */
void frame_describe(FILE *out, const Frame *frame);

#endif /* FRAMESMITH_HEXDUMP_H */
