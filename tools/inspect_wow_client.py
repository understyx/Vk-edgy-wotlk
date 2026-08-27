#!/usr/bin/env python3
"""Inspect a WoW 3.3.5a PE image and optional Windows minidumps.

This tool deliberately uses only the Python standard library.  It fingerprints
the exact executable, reports the PE layout used by absolute-address profiles,
and attempts to read selected globals from crash dumps when those pages were
captured.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


KNOWN_GLOBALS = {
    "map_id": 0x00AB63BC,
    "combat_log_manager": 0x00ADB974,
    "game_state": 0x00B6A9E0,
    "world_loading": 0x00B6AA38,
    "world_loaded": 0x00BEBA40,
    "player_in_game": 0x00BD0792,
    "zone_text_ptr": 0x00BD0788,
    "subzone_text_ptr": 0x00BD0784,
    "zone_id": 0x00BD080C,
    "mouse_over_guid": 0x00BD07A0,
    "last_target_guid": 0x00BD07B8,
    "party_array": 0x00BD1948,
    "raid_array": 0x00BEB568,
    "raid_count": 0x00BEB608,
    "name_cache_hash_table": 0x00C5D940,
    "realm_name": 0x00C79B9E,
    "client_connection": 0x00C79CE0,
    "player_name": 0x00C79D18,
    "local_player_guid": 0x00CA1238,
    "player_base": 0x00CD87A8,
    "internal_map_name_buffer": 0x00CE06D0,
}


def u16(data: bytes, offset: int) -> int:
    return struct.unpack_from("<H", data, offset)[0]


def u32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def u64(data: bytes, offset: int) -> int:
    return struct.unpack_from("<Q", data, offset)[0]


@dataclass(frozen=True)
class PeSection:
    name: str
    virtual_address: int
    virtual_size: int
    raw_offset: int
    raw_size: int

    def contains_va(self, image_base: int, address: int) -> bool:
        rva = address - image_base
        return self.virtual_address <= rva < self.virtual_address + max(self.virtual_size, self.raw_size)


@dataclass(frozen=True)
class PeProfile:
    sha256: str
    machine: int
    timestamp: int
    image_base: int
    image_size: int
    entry_point_rva: int
    characteristics: int
    sections: tuple[PeSection, ...]


def parse_pe(path: Path) -> PeProfile:
    data = path.read_bytes()
    if data[:2] != b"MZ":
        raise ValueError(f"{path} is not a DOS/PE image")
    pe_offset = u32(data, 0x3C)
    if data[pe_offset : pe_offset + 4] != b"PE\0\0":
        raise ValueError(f"{path} has no PE signature")

    coff = pe_offset + 4
    machine = u16(data, coff)
    section_count = u16(data, coff + 2)
    timestamp = u32(data, coff + 4)
    optional_size = u16(data, coff + 16)
    characteristics = u16(data, coff + 18)
    optional = coff + 20
    if u16(data, optional) != 0x10B:
        raise ValueError("only PE32 clients are supported")

    entry_point_rva = u32(data, optional + 16)
    image_base = u32(data, optional + 28)
    image_size = u32(data, optional + 56)
    section_offset = optional + optional_size
    sections: list[PeSection] = []
    for index in range(section_count):
        header = section_offset + index * 40
        name = data[header : header + 8].split(b"\0", 1)[0].decode("ascii", "replace")
        virtual_size = u32(data, header + 8)
        virtual_address = u32(data, header + 12)
        raw_size = u32(data, header + 16)
        raw_offset = u32(data, header + 20)
        sections.append(PeSection(name, virtual_address, virtual_size, raw_offset, raw_size))

    return PeProfile(
        sha256=hashlib.sha256(data).hexdigest(),
        machine=machine,
        timestamp=timestamp,
        image_base=image_base,
        image_size=image_size,
        entry_point_rva=entry_point_rva,
        characteristics=characteristics,
        sections=tuple(sections),
    )


@dataclass(frozen=True)
class MemoryRange:
    start: int
    data_offset: int
    size: int


class MiniDump:
    def __init__(self, path: Path):
        self.path = path
        self.data = path.read_bytes()
        if self.data[:4] != b"MDMP":
            raise ValueError(f"{path} is not a minidump")
        count = u32(self.data, 8)
        directory_rva = u32(self.data, 12)
        self.streams: dict[int, tuple[int, int]] = {}
        for index in range(count):
            entry = directory_rva + index * 12
            stream_type, size, rva = struct.unpack_from("<III", self.data, entry)
            self.streams[stream_type] = (rva, size)
        self.ranges = tuple(self._parse_memory_ranges())

    def _parse_memory_ranges(self) -> Iterable[MemoryRange]:
        # MINIDUMP_MEMORY_LIST
        if 5 in self.streams:
            rva, _ = self.streams[5]
            count = u32(self.data, rva)
            for index in range(count):
                entry = rva + 4 + index * 16
                start = u64(self.data, entry)
                size = u32(self.data, entry + 8)
                data_rva = u32(self.data, entry + 12)
                yield MemoryRange(start, data_rva, size)

        # MINIDUMP_MEMORY64_LIST
        if 9 in self.streams:
            rva, _ = self.streams[9]
            count = u64(self.data, rva)
            data_rva = u64(self.data, rva + 8)
            for index in range(count):
                entry = rva + 16 + index * 16
                start = u64(self.data, entry)
                size = u64(self.data, entry + 8)
                yield MemoryRange(start, data_rva, size)
                data_rva += size

    def read(self, address: int, size: int) -> bytes | None:
        for memory in self.ranges:
            if memory.start <= address and address + size <= memory.start + memory.size:
                offset = memory.data_offset + address - memory.start
                return self.data[offset : offset + size]
        return None

    def read_cstring(self, address: int, limit: int = 96) -> str | None:
        raw = self.read(address, limit)
        if raw is None:
            return None
        return raw.split(b"\0", 1)[0].decode("utf-8", "replace")


def format_profile(path: Path, profile: PeProfile) -> dict[str, object]:
    addresses: dict[str, object] = {}
    for name, address in KNOWN_GLOBALS.items():
        section = next((item.name for item in profile.sections if item.contains_va(profile.image_base, address)), None)
        addresses[name] = {"address": f"0x{address:08X}", "section": section or "unmapped"}
    return {
        "path": str(path),
        "sha256": profile.sha256,
        "machine": f"0x{profile.machine:04X}",
        "timestamp": f"0x{profile.timestamp:08X}",
        "image_base": f"0x{profile.image_base:08X}",
        "image_size": f"0x{profile.image_size:08X}",
        "entry_point_rva": f"0x{profile.entry_point_rva:08X}",
        "relocations_stripped": bool(profile.characteristics & 0x0001),
        "large_address_aware": bool(profile.characteristics & 0x0020),
        "sections": [
            {
                "name": item.name,
                "rva": f"0x{item.virtual_address:08X}",
                "virtual_size": f"0x{item.virtual_size:08X}",
            }
            for item in profile.sections
        ],
        "known_globals": addresses,
    }


def inspect_dump(path: Path) -> dict[str, object]:
    dump = MiniDump(path)
    values: dict[str, object] = {}
    for name, address in KNOWN_GLOBALS.items():
        raw = dump.read(address, 8)
        if raw is None:
            continue
        values[name] = {
            "address": f"0x{address:08X}",
            "u8": raw[0],
            "u32": f"0x{u32(raw, 0):08X}",
            "u64": f"0x{u64(raw, 0):016X}",
            "inline": dump.read_cstring(address),
        }
        pointer = u32(raw, 0)
        pointed = dump.read_cstring(pointer) if pointer else None
        if pointed is not None:
            values[name]["pointer"] = f"0x{pointer:08X}"
            values[name]["pointed_string"] = pointed
    return {
        "path": str(path),
        "stream_types": sorted(dump.streams),
        "memory_range_count": len(dump.ranges),
        "values": values,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("exe", type=Path, help="path to Wow.exe")
    parser.add_argument("dumps", type=Path, nargs="*", help="optional Crash.dmp files")
    args = parser.parse_args()

    result = {"client": format_profile(args.exe, parse_pe(args.exe))}
    if args.dumps:
        result["dumps"] = [inspect_dump(path) for path in args.dumps]
    print(json.dumps(result, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
