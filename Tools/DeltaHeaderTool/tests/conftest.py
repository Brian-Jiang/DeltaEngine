import sys
from pathlib import Path

import pytest

_TESTS_DIR = Path(__file__).resolve().parent
_TOOL_ROOT = _TESTS_DIR.parent

if str(_TOOL_ROOT) not in sys.path:
    sys.path.insert(0, str(_TOOL_ROOT))


@pytest.fixture(scope="session", autouse=True)
def _configure_libclang():
    from parser import _ensure_configured

    _ensure_configured()


def pytest_addoption(parser):
    parser.addoption(
        "--snapshot-update",
        action="store_true",
        default=False,
        help="Write snapshot files instead of comparing",
    )


@pytest.fixture
def snapshot_dir():
    return _TESTS_DIR / "snapshots"


@pytest.fixture
def fixtures_dir():
    return _TESTS_DIR / "fixtures"


@pytest.fixture
def snapshot_update(request):
    return request.config.getoption("--snapshot-update")
