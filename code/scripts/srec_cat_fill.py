import argparse

def parse_intel_hex(file_path):
    data = {}
    extended_addr = 0
    with open(file_path, 'r') as f:
        for line in f:
            line = line.strip()
            if not line or line[0] != ':':
                continue
            # Parse record fields
            byte_count = int(line[1:3], 16)
            offset = int(line[3:7], 16)
            record_type = int(line[7:9], 16)
            data_field = line[9:9 + byte_count*2]
            # checksum = int(line[9+byte_count*2:9+byte_count*2+2], 16)  # Not validating here

            if record_type == 0x04:
                # Extended linear address record
                if byte_count != 2:
                    raise ValueError("Invalid extended linear address record length")
                extended_addr = int(data_field, 16)
            elif record_type == 0x00:
                # Data record
                base_addr = (extended_addr << 16) + offset
                for i in range(byte_count):
                    byte_val = int(data_field[i*2:i*2+2], 16)
                    data[base_addr + i] = byte_val
            elif record_type == 0x01:
                # End Of File record, stop processing this file.
                break
            else:
                # Other record types are skipped.
                continue
    return data

def compute_checksum(byte_count, offset, record_type, data_bytes):
    total = byte_count + ((offset >> 8) & 0xFF) + (offset & 0xFF) + record_type
    total += sum(data_bytes)
    checksum = ((~total + 1) & 0xFF)
    return checksum

def write_data_as_hex(merged_data, output_file):
    if not merged_data:
        raise ValueError("No data to write")

    min_addr = min(merged_data.keys())
    max_addr = max(merged_data.keys())
    current = min_addr
    last_ext_addr = None
    records = []

    # Function to generate an extended linear address record.
    def gen_ext_record(ext_addr):
        byte_count = 2
        offset = 0
        record_type = 0x04
        data_bytes = [(ext_addr >> 8) & 0xFF, ext_addr & 0xFF]
        chksum = compute_checksum(byte_count, offset, record_type, data_bytes)
        record = ":{:02X}{:04X}{:02X}{}{:02X}".format(
            byte_count, offset, record_type, ''.join(f"{b:02X}" for b in data_bytes), chksum)
        return record

    # Write data records using 16 bytes per record.
    while current <= max_addr:
        ext_addr = current >> 16
        # If extended address changed, output extended linear address record.
        if ext_addr != last_ext_addr:
            records.append(gen_ext_record(ext_addr))
            last_ext_addr = ext_addr

        offset = current & 0xFFFF
        # Determine chunk length until next 16-byte boundary or next extended change.
        chunk_len = min(16 - (offset % 16), max_addr - current + 1)
        data_bytes = []
        for i in range(chunk_len):
            addr = current + i
            # If gap, fill with zero.
            data_bytes.append(merged_data.get(addr, 0))
        chksum = compute_checksum(chunk_len, offset, 0x00, data_bytes)
        record = ":{:02X}{:04X}{:02X}{}{:02X}".format(
            chunk_len, offset, 0x00, ''.join(f"{b:02X}" for b in data_bytes), chksum)
        records.append(record)
        current += chunk_len

    # End Of File record
    records.append(":00000001FF")
    with open(output_file, 'w') as f:
        for rec in records:
            f.write(rec + "\n")

def main():
    parser = argparse.ArgumentParser(
        description='Merge multiple HEX files and fill gaps with zeros.')
    parser.add_argument('files', nargs='+', help='Input HEX files')
    parser.add_argument('-o', '--output', default='out.hex', help='Output HEX file')
    args = parser.parse_args()

    merged_data = {}

    # Merge data from each HEX file.
    for file in args.files:
        print(f"Processing file: {file}")
        file_data = parse_intel_hex(file)
        # Later files overwrite earlier ones on address conflicts.
        merged_data.update(file_data)

    if not merged_data:
        print("No data found in input files.")
        return

    print(f"Merging address range: 0x{min(merged_data.keys()):X} to 0x{max(merged_data.keys()):X}")
    write_data_as_hex(merged_data, args.output)
    print(f"Output written to {args.output}")

if __name__ == '__main__':
    main()