"""Extract IP.BIN and files from the high-density track of a Redump GD-ROM dump.

Usage: python tools/extract_gdrom.py <track3.bin> <out_dir> [--all]
Without --all, only IP.BIN and 1ST_READ.BIN are written; the full file list is printed.
"""
import struct
import sys
from pathlib import Path

SECTOR = 2352
USER_OFS = 16  # MODE1/2352: 12 sync + 4 header bytes
HD_START_LBA = 45000  # high-density area starts here; ISO9660 LBAs are absolute


def read_sectors(f, lba, count):
    out = bytearray()
    for i in range(count):
        f.seek((lba - HD_START_LBA + i) * SECTOR + USER_OFS)
        out += f.read(2048)
    return bytes(out)


def walk(f, lba, size, prefix=""):
    data = read_sectors(f, lba, (size + 2047) // 2048)
    pos = 0
    while pos < len(data):
        rec_len = data[pos]
        if rec_len == 0:  # records never span sectors; skip to next one
            pos = (pos // 2048 + 1) * 2048
            continue
        rec = data[pos:pos + rec_len]
        ext_lba = struct.unpack_from("<I", rec, 2)[0]
        ext_size = struct.unpack_from("<I", rec, 10)[0]
        flags = rec[25]
        name = rec[33:33 + rec[32]]
        pos += rec_len
        if name in (b"\x00", b"\x01"):
            continue
        name = name.decode("ascii").split(";")[0]
        path = prefix + name
        if flags & 2:
            yield from walk(f, ext_lba, ext_size, path + "/")
        else:
            yield path, ext_lba, ext_size


def main():
    track3, out_dir = Path(sys.argv[1]), Path(sys.argv[2])
    extract_all = "--all" in sys.argv[3:]
    out_dir.mkdir(parents=True, exist_ok=True)
    with open(track3, "rb") as f:
        (out_dir / "IP.BIN").write_bytes(read_sectors(f, HD_START_LBA, 16))
        pvd = read_sectors(f, HD_START_LBA + 16, 1)
        assert pvd[1:6] == b"CD001", "no ISO9660 PVD in track 3"
        root = pvd[156:156 + 34]
        root_lba, root_size = struct.unpack_from("<I", root, 2)[0], struct.unpack_from("<I", root, 10)[0]
        for path, lba, size in walk(f, root_lba, root_size):
            print(f"{lba:7d} {size:10d} {path}")
            if extract_all or path == "1ST_READ.BIN":
                dest = out_dir / path
                dest.parent.mkdir(parents=True, exist_ok=True)
                f.seek((lba - HD_START_LBA) * SECTOR)
                with open(dest, "wb") as o:
                    for i in range((size + 2047) // 2048):
                        o.write(read_sectors(f, lba + i, 1))
                    o.truncate(size)


if __name__ == "__main__":
    main()
