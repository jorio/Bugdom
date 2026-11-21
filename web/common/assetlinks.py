#!/usr/bin/env python3
"""Generate Digital Asset Links for a signed Android TWA wrapper."""
import argparse
import json
from pathlib import Path
import re


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--package', required=True, help='Android application ID')
    p.add_argument('--sha256', required=True, action='append', help='Signing certificate fingerprint; repeat for multiple certificates')
    p.add_argument('--output', required=True, type=Path)
    args = p.parse_args()
    if not re.fullmatch(r'[a-zA-Z][\w]*(?:\.[a-zA-Z][\w]*)+', args.package):
        p.error('Invalid Android package name')
    fingerprints = []
    for value in args.sha256:
        raw = value.replace(':', '').upper()
        if not re.fullmatch('[0-9A-F]{64}', raw): p.error('Each SHA-256 fingerprint must contain 64 hexadecimal digits')
        fingerprints.append(':'.join(raw[i:i+2] for i in range(0, 64, 2)))
    # Preserve associations for other apps sharing this origin.
    statements = json.loads(args.output.read_text()) if args.output.exists() else []
    statements = [s for s in statements if s.get('target', {}).get('package_name') != args.package]
    statements.append(dict(relation=['delegate_permission/common.handle_all_urls'],
        target=dict(namespace='android_app', package_name=args.package, sha256_cert_fingerprints=fingerprints)))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(statements, indent=2)+'\n')


if __name__ == '__main__': main()
