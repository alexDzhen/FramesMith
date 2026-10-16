/*
   sender.h

   Putting a finished frame on a real network, the one part that needs the
   operating system and special privileges. The interface is tiny and the
   same on both platforms; the implementation differs (a raw AF_PACKET
   socket on Linux; on Windows there is no built-in way, so it reports that
   clearly and points to the .pcap route instead).
 */

#ifndef FRAMESMITH_SENDER_H
#define FRAMESMITH_SENDER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* The result of a send attempt. */
typedef struct SendResult {
    bool ok;
    char message[256]; /* on failure, what went wrong and often what to do */
} SendResult;

/* Sends `length` raw bytes out of the interface named `interface` (e.g.
   "eth0"). The bytes are the complete frame starting at the destination
   MAC; the hardware adds the CRC. `count` copies are sent, `interval_ms`
   apart, so the operator can emit a short burst. Returns ok=false with a
   message when the interface is unknown, permission is denied, or sending
   is not supported on this platform. */
SendResult frame_send(const char *interface, const uint8_t *bytes, size_t length, unsigned count,
                      unsigned interval_ms);

/* True if this build can send at all (Linux yes, Windows no). Lets the
   command-line layer give a clear message before trying. */
bool frame_send_supported(void);

#endif /* FRAMESMITH_SENDER_H */
