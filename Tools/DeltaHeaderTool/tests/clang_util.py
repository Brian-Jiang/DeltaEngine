import pytest

from parser import libclang_library_path


def require_libclang() -> None:
    p = libclang_library_path()
    if not p.is_file():
        pytest.skip(f"libclang not found at {p}")
