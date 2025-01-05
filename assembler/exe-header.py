from dataclasses import dataclass
import struct

@dataclass
class DOSExeHeader:
    signature: bytes         # 2 bytes: Magic number ("MZ")
    last_page_size: int      # 2 bytes: Bytes in the last page
    num_pages: int           # 2 bytes: Total number of 512-byte pages
    num_relocations: int     # 2 bytes: Number of relocation entries
    header_paragraphs: int   # 2 bytes: Size of the header in paragraphs (16 bytes each)
    min_memory: int          # 2 bytes: Minimum memory required (in paragraphs)
    max_memory: int          # 2 bytes: Maximum memory required (in paragraphs)
    initial_ss: int          # 2 bytes: Initial stack segment (relative to the load segment)
    initial_sp: int          # 2 bytes: Initial stack pointer
    checksum: int            # 2 bytes: Checksum of the header
    initial_ip: int          # 2 bytes: Initial instruction pointer
    initial_cs: int          # 2 bytes: Initial code segment (relative to the load segment)
    relocation_table_offset: int  # 2 bytes: Offset of the relocation table
    overlay_number: int      # 2 bytes: Overlay number

    @classmethod
    def from_bytes(cls, data: bytes):
        """Create a DOSExeHeader instance from binary data."""
        fields = struct.unpack("<2s13H", data[:28])  # '<' for little-endian, '2s13H' matches the header layout
        return cls(
            signature=fields[0],
            last_page_size=fields[1],
            num_pages=fields[2],
            num_relocations=fields[3],
            header_paragraphs=fields[4],
            min_memory=fields[5],
            max_memory=fields[6],
            initial_ss=fields[7],
            initial_sp=fields[8],
            checksum=fields[9],
            initial_ip=fields[10],
            initial_cs=fields[11],
            relocation_table_offset=fields[12],
            overlay_number=fields[13],
        )

    def to_bytes(self) -> bytes:
        """Convert the DOSExeHeader instance to binary data."""
        return struct.pack(
            "<2s13H",
            self.signature,
            self.last_page_size,
            self.num_pages,
            self.num_relocations,
            self.header_paragraphs,
            self.min_memory,
            self.max_memory,
            self.initial_ss,
            self.initial_sp,
            self.checksum,
            self.initial_ip,
            self.initial_cs,
            self.relocation_table_offset,
            self.overlay_number,
        )

# Example Usage
# Assuming `binary_data` contains the first 28 bytes of a DOS EXE file
binary_data = b'MZ\x90\x00\x03\x00\x00\x00\x04\x00\x00\x00\xFF\xFF\x00\x00\xB8\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
header = DOSExeHeader.from_bytes(binary_data)

# Print the parsed header fields
print(header)

# Convert the header back to bytes
header_bytes = header.to_bytes()
print(header_bytes)

def parse_relocation_table(data, offset, num_entries):
    """Parse relocation entries from binary data."""
    relocation_entries = []
    for i in range(num_entries):
        entry_offset = offset + (i * 4)
        segment_offset, segment = struct.unpack("<HH", data[entry_offset:entry_offset + 4])
        relocation_entries.append((segment_offset, segment))
    return relocation_entries

# Example Usage:
# Assuming `data` contains the binary content of an EXE file:
relocation_offset = 0x1A  # Relocation table offset from the header
relocation_count = 3     # Number of entries from the header
relocation_table = parse_relocation_table(data, relocation_offset, relocation_count)

# Print parsed relocation entries
print(relocation_table)

