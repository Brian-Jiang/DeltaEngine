import json
import sys
from pathlib import Path

import pytest

_TESTS_DIR = Path(__file__).resolve().parent
_TOOL_ROOT = _TESTS_DIR.parent

if str(_TOOL_ROOT) not in sys.path:
    sys.path.insert(0, str(_TOOL_ROOT))


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
def snapshot_update(request):
    return request.config.getoption("--snapshot-update")


@pytest.fixture(scope="session")
def schemas():
    schemas_dir = _TOOL_ROOT / "Schemas"
    systems = {}
    for path in sorted(schemas_dir.glob("*.json")):
        data = json.loads(path.read_text(encoding="utf-8"))
        if "system" in data:
            systems[data["system"]] = data
    return systems
