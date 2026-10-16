/*
   sender_windows.c

   The Windows build cannot send raw Ethernet frames, see sender.h. Only
   compiled on Windows.

   Windows removed raw-socket frame sending years ago, and the kernel driver
   that tools like Npcap use to do it is a separate install, not something a
   self-contained program can rely on. Rather than pretend, framesmith says
   so plainly and points the operator at the --pcap output, which they can
   open in Wireshark or replay with a tool that does ship such a driver.
 */

#include "framesmith/sender.h"

#include <stdio.h>
#include <string.h>

bool frame_send_supported(void) {
    return false;
}

SendResult frame_send(const char *interface, const uint8_t *bytes, size_t length, unsigned count,
                      unsigned interval_ms) {
    (void)interface;
    (void)bytes;
    (void)length;
    (void)count;
    (void)interval_ms;

    SendResult result;
    result.ok = false;
    snprintf(result.message, sizeof(result.message),
             "sending raw frames is not supported on Windows.\n"
             "  Write the frame to a .pcap file with --pcap and open it in Wireshark,\n"
             "  or replay it with a tool that installs a capture driver (e.g. Npcap).");
    return result;
}
