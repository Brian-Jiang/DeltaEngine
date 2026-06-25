import sys
from pathlib import Path

_TESTS_DIR = Path(__file__).resolve().parent
_DELTA_CMD_ROOT = _TESTS_DIR.parent

if str(_DELTA_CMD_ROOT) not in sys.path:
    sys.path.insert(0, str(_DELTA_CMD_ROOT))
