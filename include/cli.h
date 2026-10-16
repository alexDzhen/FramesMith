/*
 * cli.h
 *
 * The command-line interface. framesmith has two ways to describe a frame:
 * entirely through options (good for scripts), or interactively, where it
 * asks the operator for each field in turn (good for learning and for the
 * task's "content determined by an operator"). The options also choose what
 * to do with the finished frame: dump it, write a .pcap, or send it.
 */

#ifndef FRAMESMITH_CLI_H
#define FRAMESMITH_CLI_H

#include <stdbool.h>
#include <stddef.h>

#include "hexparse.h"

// What to do once the frame is built. These are not exclusive: the frame is
// always dumped unless --quiet, and a .pcap and/or a send can be added. */
typedef struct CliOptions {
    // Field sources. Each "..._set" flag says the operator gave this field;
    // fields left unset take a sensible default or are asked for in
    // interactive mode. The text is kept unparsed so the parser can report
    // a good error with the original spelling. */
    const char *destination; // --dst
    const char *source;      // --src
    const char *ethertype;   // --type
    const char *vlan;        // --vlan ID[:PRIORITY], NULL for no tag
    const char *payload;     // --payload / --ascii / --file argument
    PayloadEncoding payload_encoding;
    bool no_pad; // --no-pad: do not pad short frames to 60 bytes

    // Outputs.
    bool quiet;             // --quiet: do not print the hex dump
    const char *pcap_path;  // --pcap FILE, or NULL
    const char *send_iface; // --send IFACE, or NULL
    unsigned send_count;    // --count (default 1)
    unsigned send_interval; // --interval milliseconds (default 0)

    bool interactive; // --interactive, or no field options at all
    bool show_help;
    bool show_version;
} CliOptions;

// Parses argv. Returns true on success. On a bad option it returns false
// and writes a message into `error`.
bool cli_parse(int argc, char **argv, CliOptions *options, char *error, size_t error_size);

// The text printed for --help.
const char *cli_usage_text(void);

#endif /* FRAMESMITH_CLI_H */
