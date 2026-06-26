from __future__ import annotations

import os
import re
import time
from io import StringIO
from pathlib import Path
from unittest.mock import MagicMock, patch

import pytest

from env import _delta_cmd_logging

logging_mod = _delta_cmd_logging()
LOG_FILENAME_RE = logging_mod.LOG_FILENAME_RE
SessionLog = logging_mod.SessionLog
prune_logs = logging_mod.prune_logs
run_subprocess_with_tee = logging_mod.run_subprocess_with_tee


def _make_session(tmp_path: Path, argv: list[str] | None = None, automatic: bool = True) -> SessionLog:
    return SessionLog(tmp_path, argv or ["build", "editor"], automatic)


def test_log_filename_format(tmp_path: Path):
    session = _make_session(tmp_path)
    assert LOG_FILENAME_RE.match(session.log_path.name)


def test_header_contains_command(tmp_path: Path):
    session = _make_session(tmp_path, ["build", "editor", "--preset", "x64-debug"], automatic=True)
    session.write_header()
    session._log_file.close()

    content = session.log_path.read_text(encoding="utf-8")
    assert "command: build editor --preset x64-debug" in content
    assert "automatic: true" in content


def test_tee_writes_and_prints(tmp_path: Path, capsys: pytest.CaptureFixture[str]):
    session = _make_session(tmp_path)
    session.write_header()

    mock_process = MagicMock()
    mock_process.stdout = iter(["line one\n", "line two\n"])
    mock_process.wait.return_value = 0

    with patch("delta_cmd_logging.subprocess.Popen", return_value=mock_process):
        exit_code = run_subprocess_with_tee(
            ["echo", "test"],
            cwd=tmp_path,
            session=session,
        )

    assert exit_code == 0
    log_content = session.log_path.read_text(encoding="utf-8")
    assert "line one" in log_content
    assert "line two" in log_content

    captured = capsys.readouterr()
    assert "line one" in captured.out
    assert "line two" in captured.out


def test_footer_exit_code(tmp_path: Path):
    session = _make_session(tmp_path)
    session.write_header()
    session.write_footer(42)

    content = session.log_path.read_text(encoding="utf-8")
    assert "exit_code: 42" in content


def test_prune_deletes_old_logs(tmp_path: Path):
    log_dir = tmp_path / "Intermediate" / "DeltaCmd"
    log_dir.mkdir(parents=True)

    old_log = log_dir / "20000101-000000.log"
    new_log = log_dir / "20990101-000000.log"
    old_log.write_text("old", encoding="utf-8")
    new_log.write_text("new", encoding="utf-8")

    old_time = time.time() - (8 * 86400)
    os.utime(old_log, (old_time, old_time))

    prune_logs(log_dir, max_age_days=7)

    assert not old_log.exists()
    assert new_log.exists()


def test_prune_ignores_non_logs(tmp_path: Path):
    log_dir = tmp_path / "Intermediate" / "DeltaCmd"
    log_dir.mkdir(parents=True)

    other_file = log_dir / "notes.txt"
    other_file.write_text("keep", encoding="utf-8")

    prune_logs(log_dir)

    assert other_file.exists()


def test_stream_tee_captures_stderr(tmp_path: Path):
    session = _make_session(tmp_path)
    session.write_header()
    session.install_stream_tee()

    print("ERROR: something failed", file=__import__("sys").stderr)

    session.restore_streams()
    session.write_footer(1)

    content = session.log_path.read_text(encoding="utf-8")
    assert "ERROR: something failed" in content
