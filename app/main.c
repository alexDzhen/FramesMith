//
// main.c
//
// Entry point of framesmith. It fills a FrameSpec either from the command
// line or by asking the operator, builds the frame, and then does whatever
// the output options ask: dump it, write a .pcap, send it.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "../include/cli.h"
#include "../include/frame.h"
#include "../include/hexdump.h"
#include "../include/hexparse.h"
#include "../include/pcap.h"
#include "../include/sender.h"

/* Prints a parse failure with a caret under the offending character, the
   way a compiler points at an error:

     framesmith: bad destination: a MAC address is six groups of two hex digits
         11:22:33:GG
                   ^
 */
static void report_parse_error(const char *field, const char *input, const ParseStatus *status) {
    fprintf(stderr, "framesmith: bad %s: %s\n", field, status->message);
    if (status->position >= 0) {
        fprintf(stderr, "    %s\n    %*s^\n", input, (int)status->position, "");
    }
}

/* ---- filling the spec from the command line -------------------------- */

/* Loads a file's bytes into the payload. Returns false (with a message) on
   any error. */
static bool load_payload_file(const char *path, FrameSpec *spec, char *error, size_t error_size) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        snprintf(error, error_size, "cannot open payload file '%s'", path);
        return false;
    }
    size_t count = fread(spec->payload, 1, sizeof(spec->payload), file);
    // If the file still has bytes left, it is larger than a frame can hold.
    int leftover = fgetc(file);
    fclose(file);
    if (leftover != EOF) {
        snprintf(error, error_size, "payload file '%s' is larger than %d bytes", path,
                 FRAME_MAX_PAYLOAD);
        return false;
    }
    spec->payload_length = count;
    return true;
}

/* Splits "ID" or "ID:PCP" into the VLAN fields. */
static bool parse_vlan_option(const char *text, FrameSpec *spec, char *error, size_t error_size) {
    const char *colon = strchr(text, ':');
    char id_text[16];
    if (colon == NULL) {
        snprintf(id_text, sizeof(id_text), "%s", text);
    } else {
        size_t id_len = (size_t)(colon - text);
        if (id_len >= sizeof(id_text)) {
            id_len = sizeof(id_text) - 1;
        }
        memcpy(id_text, text, id_len);
        id_text[id_len] = '\0';
    }

    unsigned long id;
    ParseStatus status = parse_uint(id_text, 0, 4095, &id);
    if (!status.ok) {
        snprintf(error, error_size, "VLAN id: %s", status.message);
        return false;
    }
    unsigned long pcp = 0;
    if (colon != NULL) {
        status = parse_uint(colon + 1, 0, 7, &pcp);
        if (!status.ok) {
            snprintf(error, error_size, "VLAN priority: %s", status.message);
            return false;
        }
    }
    spec->has_vlan = true;
    spec->vlan_id = (uint16_t)id;
    spec->vlan_priority = (uint8_t)pcp;
    return true;
}

/* Fills the spec from the parsed options. Fields the operator did not give
   take a default. Returns false after printing its own error. */
static bool spec_from_options(const CliOptions *options, FrameSpec *spec) {
    memset(spec, 0, sizeof(*spec));
    spec->pad_to_minimum = !options->no_pad;

    // Defaults: broadcast destination, a locally-administered source, and
    // EtherType 0x0800 (IPv4), all overridable.
    memset(spec->destination, 0xFF, 6);
    spec->source[0] = 0x02;
    spec->source[5] = 0x01;
    spec->ethertype = 0x0800;

    ParseStatus status;
    if (options->destination != NULL) {
        status = parse_mac(options->destination, spec->destination);
        if (!status.ok) {
            report_parse_error("destination", options->destination, &status);
            return false;
        }
    }
    if (options->source != NULL) {
        status = parse_mac(options->source, spec->source);
        if (!status.ok) {
            report_parse_error("source", options->source, &status);
            return false;
        }
    }
    if (options->ethertype != NULL) {
        status = parse_ethertype(options->ethertype, &spec->ethertype);
        if (!status.ok) {
            report_parse_error("ethertype", options->ethertype, &status);
            return false;
        }
    }

    char error[256];
    if (options->vlan != NULL && !parse_vlan_option(options->vlan, spec, error, sizeof(error))) {
        fprintf(stderr, "framesmith: %s\n", error);
        return false;
    }

    if (options->payload != NULL) {
        if (options->payload_encoding == PAYLOAD_FILE) {
            if (!load_payload_file(options->payload, spec, error, sizeof(error))) {
                fprintf(stderr, "framesmith: %s\n", error);
                return false;
            }
        } else {
            status = options->payload_encoding == PAYLOAD_ASCII
                         ? parse_ascii_bytes(options->payload, spec->payload, sizeof(spec->payload),
                                             &spec->payload_length)
                         : parse_hex_bytes(options->payload, spec->payload, sizeof(spec->payload),
                                           &spec->payload_length);
            if (!status.ok) {
                report_parse_error("payload", options->payload, &status);
                return false;
            }
        }
    }
    return true;
}

/* ---- interactive mode ------------------------------------------------ */

/* Reads one line into `buffer`, trimming the newline. Returns false at end
   of input. */
static bool read_line(char *buffer, size_t size) {
    if (fgets(buffer, (int)size, stdin) == NULL) {
        return false;
    }
    buffer[strcspn(buffer, "\r\n")] = '\0';
    return true;
}

/* Asks for one field, re-prompting until the parse succeeds or the operator
   presses Enter to accept the default. `parse` is one of the hexparse
   functions wrapped in a small lambda-like helper by the caller. Returns
   false only at end of input. */
typedef ParseStatus (*FieldParser)(const char *text, void *out);

static bool ask_field(const char *prompt, const char *default_text, FieldParser parse, void *out) {
    char line[2048];
    for (;;) {
        printf("%s", prompt);
        if (default_text != NULL) {
            printf(" [%s]", default_text);
        }
        printf(": ");
        fflush(stdout);

        if (!read_line(line, sizeof(line))) {
            return false;
        }
        // An empty line means "use the default", if there is one.
        const char *text = line[0] == '\0' && default_text != NULL ? default_text : line;
        ParseStatus status = parse(text, out);
        if (status.ok) {
            return true;
        }
        printf("  %s\n", status.message);
    }
}

// Small adapters so the field parsers share one function-pointer type.
static ParseStatus parse_mac_field(const char *text, void *out) {
    return parse_mac(text, (uint8_t *)out);
}
static ParseStatus parse_ethertype_field(const char *text, void *out) {
    return parse_ethertype(text, (uint16_t *)out);
}

/* Fills the spec by asking the operator for each field. Returns false if
   the operator ends input early. */
static bool spec_from_prompts(FrameSpec *spec) {
    memset(spec, 0, sizeof(*spec));
    spec->pad_to_minimum = true;

    puts("framesmith interactive mode. Press Enter to accept the [default].");

    memset(spec->destination, 0xFF, 6);
    if (!ask_field("Destination MAC", "broadcast", parse_mac_field, spec->destination)) {
        return false;
    }
    spec->source[0] = 0x02;
    spec->source[5] = 0x01;
    if (!ask_field("Source MAC", "02:00:00:00:00:01", parse_mac_field, spec->source)) {
        return false;
    }
    spec->ethertype = 0x0800;
    if (!ask_field("EtherType", "ipv4", parse_ethertype_field, &spec->ethertype)) {
        return false;
    }

    // The payload is asked for as hex, with an empty default (no payload).
    char line[4096];
    printf("Payload (hex bytes, e.g. \"de ad be ef\") []: ");
    fflush(stdout);
    if (!read_line(line, sizeof(line))) {
        return false;
    }
    for (;;) {
        ParseStatus status =
            parse_hex_bytes(line, spec->payload, sizeof(spec->payload), &spec->payload_length);
        if (status.ok) {
            break;
        }
        printf("  %s\n  Payload (hex) []: ", status.message);
        fflush(stdout);
        if (!read_line(line, sizeof(line))) {
            return false;
        }
    }
    return true;
}

/* ---- outputs --------------------------------------------------------- */

static int write_pcap(const char *path, const Frame *frame) {
    PcapWriter writer;
    if (!pcap_open(&writer, path)) {
        fprintf(stderr, "framesmith: cannot write pcap file '%s'\n", path);
        return 2;
    }
    bool ok = pcap_write_frame(&writer, frame->bytes, frame->length);
    pcap_close(&writer);
    if (!ok) {
        fprintf(stderr, "framesmith: error writing pcap file '%s'\n", path);
        return 2;
    }
    printf("Wrote %s (open it in Wireshark).\n", path);
    return 0;
}

static int send_frame(const CliOptions *options, const Frame *frame) {
    SendResult result = frame_send(options->send_iface, frame->bytes, frame->length,
                                   options->send_count, options->send_interval);
    if (!result.ok) {
        fprintf(stderr, "framesmith: %s\n", result.message);
        return 1;
    }
    printf("%s\n", result.message);
    return 0;
}

int main(int argc, char **argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    CliOptions options;
    char error[256];
    if (!cli_parse(argc, argv, &options, error, sizeof(error))) {
        fprintf(stderr, "framesmith: %s\nTry 'framesmith --help' for more information.\n", error);
        return 2;
    }
    if (options.show_help) {
        fputs(cli_usage_text(), stdout);
        return 0;
    }
    if (options.show_version) {
        printf("framesmith %s\n", FRAMESMITH_VERSION);
        return 0;
    }

    // Describe the frame, from prompts or from options.
    FrameSpec spec;
    if (options.interactive) {
        if (!spec_from_prompts(&spec)) {
            fprintf(stderr, "framesmith: input ended before the frame was complete\n");
            return 2;
        }
    } else if (!spec_from_options(&options, &spec)) {
        return 2; // the helper already printed the error
    }

    // Build it.
    Frame frame;
    FrameError build_error = build_frame(&spec, &frame);
    if (build_error != FRAME_OK) {
        fprintf(stderr, "framesmith: %s\n", frame_error_text(build_error));
        return 2;
    }

    // Always show what was built, unless silenced.
    if (!options.quiet) {
        frame_describe(stdout, &frame);
        hexdump(stdout, frame.bytes, frame.length);
    }

    // Optional outputs. A pcap write failure is fatal; a send failure sets
    // the exit code but still lets the pcap have been written.
    int status = 0;
    if (options.pcap_path != NULL) {
        int pcap_status = write_pcap(options.pcap_path, &frame);
        if (pcap_status != 0) {
            return pcap_status;
        }
    }
    if (options.send_iface != NULL) {
        status = send_frame(&options, &frame);
    }
    return status;
}
