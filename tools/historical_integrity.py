"""Verify frozen archive bytes, including the exact LF form already stored by Git.

No file is rewritten. Optional local evidence always requires its original bytes.
"""
from __future__ import annotations

import hashlib
from pathlib import Path


def archive_form(data: bytes, entry: dict, *, allow_git_lf: bool = True) -> bytes:
    """Return authenticated archive bytes or fail; accept no other normalization."""
    def matches(candidate):
        return (len(candidate) == entry['bytes']
                and hashlib.sha256(candidate).hexdigest() == entry['sha256'])

    if matches(data):
        return data
    if (allow_git_lf and entry['tracked'] and Path(entry['path']).suffix in ('.txt', '.cpp')
            and b'\r' not in data):
        restored = data.replace(b'\n', b'\r\n')
        if matches(restored):
            return restored
    raise ValueError(f"historical size/SHA-256 mismatch: {entry['path']}")
