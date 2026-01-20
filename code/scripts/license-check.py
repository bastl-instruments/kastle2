#!/usr/bin/env python3

import os
import sys
import re
from typing import Dict, List, Tuple

LICENSE_PATTERNS = {
    'MIT': r'MIT License|Permission is hereby granted, free of charge',
    'LGPL': r'GNU Lesser General Public License|LGPL',
    'GPL': r'GNU General Public License|GPL',
    'Apache': r'Apache License',
    'CC0': r'Creative Commons CC0',
    'BSD': r'BSD License|Redistribution and use in source',
}

def find_source_files(src_dir: str) -> List[str]:
    """Find all source files with specified extensions."""
    source_files = []
    for root, _, files in os.walk(src_dir):
        for file in files:
            if file.endswith(('.h', '.hpp', '.cpp', '.c', '.pio')):
                source_files.append(os.path.join(root, file))
    return source_files

def detect_license(content: str) -> Tuple[bool, str]:
    """Detect license in file content. Returns (has_license, license_type)."""
    # Look only in the first 1000 characters where license headers typically appear
    header = content[:1000].lower()
    
    for license_type, pattern in LICENSE_PATTERNS.items():
        if re.search(pattern.lower(), header):
            return True, license_type
    
    # Check for generic copyright notices
    if re.search(r'copyright|©|\(c\)', header):
        return True, "Copyright"
        
    return False, ""

def check_licenses(src_dir: str, verbose: bool = False) -> Dict:
    """Check licenses in all source files."""
    stats = {
        'total_files': 0,
        'with_license': 0,
        'without_license': 0,
        'license_types': {}
    }
    
    files = find_source_files(src_dir)
    stats['total_files'] = len(files)
    
    for file_path in files:
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
                
            has_license, license_type = detect_license(content)
            rel_path = os.path.relpath(file_path, src_dir)
            
            if has_license:
                stats['with_license'] += 1
                stats['license_types'][license_type] = stats['license_types'].get(license_type, 0) + 1
                if verbose:
                    print(f"{rel_path}: {license_type}")
            else:
                stats['without_license'] += 1
                print(f"{rel_path}: License Missing")
                
        except Exception as e:
            print(f"Error processing {file_path}: {str(e)}", file=sys.stderr)
            
    return stats

def print_stats(stats: Dict):
    """Print license check statistics."""
    print("\nSummary:")
    print(f"Total files checked: {stats['total_files']}")
    print(f"Files with license: {stats['with_license']}")
    print(f"Files without license: {stats['without_license']}")
    
    if stats['license_types']:
        print("\nLicense types found:")
        for license_type, count in stats['license_types'].items():
            print(f"  {license_type}: {count}")

def main():
    src_dir = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'src')
    verbose = '--verbose' in sys.argv or '-v' in sys.argv
    
    if not os.path.exists(src_dir):
        print(f"Error: Source directory not found: {src_dir}", file=sys.stderr)
        sys.exit(1)
    
    stats = check_licenses(src_dir, verbose)
    print_stats(stats)
    
    # Exit with status code 1 if any files are missing licenses
    sys.exit(1 if stats['without_license'] > 0 else 0)

if __name__ == '__main__':
    main()
