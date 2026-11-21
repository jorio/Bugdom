#!/usr/bin/env python3
"""Verify shared runtime files match one or more sibling game checkouts."""
import argparse
from pathlib import Path
import sys


def contents(folder):
    return {str(p.relative_to(folder)): p.read_bytes() for p in folder.rglob('*')
            if p.is_file() and '__pycache__' not in p.parts}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('repositories', nargs='+', type=Path)
    args = parser.parse_args()
    expected = contents(Path(__file__).resolve().parent)
    failed = False
    for repo in args.repositories:
        actual = contents(repo / 'web/common')
        differences = sorted(name for name in expected.keys() | actual.keys()
                             if expected.get(name) != actual.get(name))
        if differences:
            failed = True
            print(f'{repo.name}: DIFFERENT: {", ".join(differences)}')
        else:
            print(f'{repo.name}: {len(expected)} shared files identical')
    return int(failed)


if __name__ == '__main__':
    sys.exit(main())
