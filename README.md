# FramesMith

A console program in C that builds Ethernet frames from fields the operator
chooses. You decide the destination and source MAC, the EtherType, an optional
VLAN tag and the payload; framesmith assembles the exact bytes, shows them as a
hex dump, and can save the frame to a `.pcap` file for Wireshark or send it on a
network interface.

*Български: [README.bg.md](README.bg.md)*

```console
$ framesmith --dst 11:22:33:44:55:66 --src 02:00:00:00:00:01 --type ipv4 --ascii "Hi"
02:00:00:00:00:01 -> 11:22:33:44:55:66  type=0x0800 (IPv4)  payload=46 bytes  total=60 bytes
0000  11 22 33 44 55 66 02 00  00 00 00 01 08 00 48 69  ."3DUf........Hi
0010  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  ................
0020  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  ................
0030  00 00 00 00 00 00 00 00  00 00 00 00              ............
```

## Features

- Every field of the frame is the operator's to set: destination and source MAC,
  EtherType (as a number like `0x0800` or a name like `ipv4`, `arp`, `ipv6`), an
  optional IEEE 802.1Q VLAN tag, and the payload.
- Payload given as hex bytes (`"de ad be ef"`), as literal text, or from a file.
- **Interactive mode**: run it with no field options and it asks for each field.
- A hex-plus-ASCII dump and a decoded summary of every frame.
- Writes a `.pcap` file you can open in Wireshark or tcpdump.
- Sends the frame on a chosen interface on Linux, optionally as a timed burst.
- No dependencies beyond the operating system; plain C11.

## Quick start

You need CMake 3.20 or newer and a C compiler.

**Linux**

```sh
git clone https://github.com/alexDzhen/FramesMith.git
cd framesmith
./scripts/build.sh
./build/framesmith --dst broadcast --type arp --payload "00 01 08 00"
```

To send frames without `sudo` every time, install it and grant the capability once:

```sh
./scripts/install.sh
sudo framesmith --send eth0 ...   # or, after install.sh, without sudo
```

**Windows**

Install Visual Studio 2022 with the "Desktop development with C++" workload and run
the commands in "Developer PowerShell for VS 2022", where `cmake` is already available.

```powershell
git clone https://github.com/alexDzhen/FramesMith.git
cd framesmith
powershell -ExecutionPolicy Bypass -File scripts\build.ps1
.\build\Release\framesmith.exe --dst broadcast --type arp --payload "00 01 08 00"
```

On Windows framesmith builds and saves frames exactly as on Linux; only `--send`
is unavailable (see [Sending frames](#sending-frames)).

Ready-made binaries for both systems are attached to every
[release](https://github.com/alexDzhen/framesmith/releases).

## Usage

```text
framesmith [frame options] [output options]
framesmith --interactive

Frame options:
  --dst MAC         Destination MAC, or 'broadcast' (default: broadcast)
  --src MAC         Source MAC (default: 02:00:00:00:00:01)
  --type TYPE       EtherType: 0x0800, or a name like ipv4, arp, ipv6
  --vlan ID[:PCP]   Add an 802.1Q VLAN tag (id 0-4095, priority 0-7)
  --payload HEX     Payload as hex bytes, e.g. "de ad be ef"
  --ascii TEXT      Payload as literal text
  --file PATH       Payload read from a file
  --no-pad          Do not pad a short frame to the 60-byte minimum

Output options:
  --pcap FILE       Write the frame to FILE (open it in Wireshark)
  --send IFACE      Send the frame out of interface IFACE (Linux, needs root)
  --count N         Send N copies (default 1)
  --interval MS     Wait MS milliseconds between copies
  -q, --quiet       Do not print the hex dump
  -i, --interactive Ask for each field instead of using options
  -h, --help        Show help and exit
  -V, --version     Show the version and exit
```

### Interactive mode

With no frame options, framesmith asks for each field. Press Enter to accept the
`[default]` shown in brackets.

```console
$ framesmith
framesmith interactive mode. Press Enter to accept the [default].
Destination MAC [broadcast]: 11:22:33:44:55:66
Source MAC [02:00:00:00:00:01]:
EtherType [ipv4]: arp
Payload (hex bytes, e.g. "de ad be ef") []: 00 01 08 00
...the frame is built and dumped...
```

### A VLAN-tagged ARP frame, hidden dump, straight to pcap

```console
$ framesmith --dst broadcast --type arp --vlan 100:3 --payload "00 01 08 00 06 04 00 01" \
             --pcap arp.pcap --quiet
Wrote arp.pcap (open it in Wireshark).
```

| Exit code | Meaning                                                 |
|-----------|---------------------------------------------------------|
| 0         | The frame was built (and sent, if `--send` was given)   |
| 1         | Sending failed                                          |
| 2         | A usage error or bad input (a malformed MAC, bad hex)   |

Bad input is pointed at precisely:

```console
$ framesmith --dst 11:22:33:GG:55:66
framesmith: bad destination: a MAC address is six groups of two hex digits
    11:22:33:GG:55:66
             ^
```

## What is in a frame

framesmith builds an **Ethernet II** frame, the kind almost all traffic uses:

```text
+-------------------+-------------------+-----------+-----------------+
| Destination MAC 6 | Source MAC      6 | Type    2 | Payload 46-1500 |
+-------------------+-------------------+-----------+-----------------+
```

- **Destination / source MAC** — 6 bytes each. The destination may be a specific
  address, or `broadcast` (`ff:ff:ff:ff:ff:ff`) for every station on the segment.
- **EtherType** — 2 bytes saying what the payload is: `0x0800` IPv4, `0x0806` ARP,
  `0x86DD` IPv6, and so on. framesmith accepts the number or a short name.
- **Payload** — your bytes. A frame's body must be at least 60 bytes, so a short
  payload is padded with zeros (switch that off with `--no-pad` to build a runt
  on purpose).

With `--vlan` an IEEE 802.1Q tag is inserted after the source MAC, which carries
the VLAN id and a priority:

```text
... | Source MAC 6 | 0x8100 | PCP/VID 2 | Type 2 | Payload ... |
```

The 4-byte CRC (the frame check sequence) that trails the frame on the wire is
added and checked by the network card, not by software, so framesmith does not
put it in the bytes it hands over; it can show the CRC value, but the hardware
is what actually appends it.

## Sending frames

`--send IFACE` puts the frame on a real network.

- **Linux** opens a raw packet socket (`AF_PACKET`), which hands the bytes
  straight to the driver. This needs the `CAP_NET_RAW` privilege, so run with
  `sudo`, or grant it once with `sudo setcap cap_net_raw+ep <path>` (which
  `scripts/install.sh` does for you). `--count` and `--interval` send a short
  burst.
- **Windows** has no built-in way to send raw frames, so `--send` reports this
  and suggests `--pcap` instead: write the frame to a file and open it in
  Wireshark, or replay it with a tool that ships a capture driver such as Npcap.

Either way, `--pcap` works on both systems and is the safe way to inspect a frame
without touching a network.

## How it works

The program is split so that only one small file touches the network:

```text
framesmith/
├── app/main.c                  Entry point: fill the spec, build, dump/pcap/send
├── include/                    Public headers
├── src/                        Implementation
├── tests/                      Unit tests (greatest) and command-line tests
├── third_party/greatest/       Test framework (single header, ISC licence)
├── scripts/                    Build and install scripts
└── .github/workflows/          CI and release pipelines
```

Everything except the two `sender_*` files is pure byte manipulation with no
system calls, which is why it compiles unchanged on both systems and is fully
unit-tested, down to the exact bytes of a VLAN tag and the standard CRC-32 check
value.

## Tests

The tests use [greatest](https://github.com/silentbicycle/greatest), a single
header framework included in `third_party/`.

- Frame assembly (header layout, padding, VLAN tag, size limits), the CRC-32
  against its standard check value, the text parsers (every notation and every
  error, with the reported position), the hex dump and summary, and the pcap
  bytes against the documented format.
- Command-line tests that run the real binary, and a check that the `.pcap`
  output is well formed.

```sh
ctest --test-dir build -C Release --output-on-failure
```

## Install and package

```sh
./scripts/install.sh                                                     # Linux
cmake --install build --config Release --prefix C:\Tools\framesmith     # Windows
cpack --config build/CPackConfig.cmake -C Release -B dist                # .tar.gz / .zip
```

Pushing a tag such as `v1.0.0` runs the release workflow, which builds, tests and
attaches both archives to a GitHub release.

## A note on responsible use

framesmith sends exactly the bytes you ask it to, which is the point of a frame
generator: testing switches, learning the protocol, reproducing a bug. Send
frames only on networks you are allowed to test.

## License

[MIT](LICENSE). The bundled test framework is under the ISC licence, see
`third_party/greatest/LICENSE`.
