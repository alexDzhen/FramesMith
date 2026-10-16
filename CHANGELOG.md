# Changelog

All notable changes to this project are documented in this file.
The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/)
and the project uses [Semantic Versioning](https://semver.org/).

## [1.0.0] - 2026-10-16

### Added

- Building of Ethernet II frames from operator-chosen fields: destination and
  source MAC, EtherType (by number or name), an optional 802.1Q VLAN tag and a
  payload given as hex, as text or from a file.
- Interactive mode that asks for each field, and a full set of command-line
  options for scripting.
- A hex-plus-ASCII dump and a decoded one-line summary of every frame.
- Writing frames to a `.pcap` file that opens in Wireshark and tcpdump.
- Sending frames on a chosen interface on Linux (raw AF_PACKET socket), with a
  burst count and interval; a clear explanation on Windows, where it is not
  supported.
- The Ethernet CRC-32 for display.
- Unit tests, command-line tests and a Wireshark-compatible pcap check.
- CMake build for Linux and Windows, CI and release workflows.
