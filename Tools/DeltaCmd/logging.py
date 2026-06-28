from __future__ import annotations

import re
import subprocess
import sys
import threading
import time
from datetime import datetime
from pathlib import Path

LOG_DIR_NAME = "Intermediate/DeltaCmd"
LOG_FILENAME_RE = re.compile(r"^\d{8}-\d{6}\.log$")
DEFAULT_RETENTION_DAYS = 7


class _TeeStream:
    def __init__(self, original: object, session: SessionLog) -> None:
        self._original = original
        self._session = session

    def write(self, text: str) -> int:
        if not text:
            return 0
        self._original.write(text)
        self._original.flush()
        self._session.append(text)
        return len(text)

    def flush(self) -> None:
        self._original.flush()

    def __getattr__(self, name: str) -> object:
        return getattr(self._original, name)


class SessionLog:
    def __init__(self, project_root: Path, argv: list[str], automatic: bool) -> None:
        self.project_root = project_root.resolve()
        self.argv = list(argv)
        self.automatic = automatic
        self.started_at = datetime.now().astimezone()
        self.cwd = Path.cwd()
        self.log_dir = self.project_root / LOG_DIR_NAME
        self.log_dir.mkdir(parents=True, exist_ok=True)
        timestamp = self.started_at.strftime("%Y%m%d-%H%M%S")
        self.log_path = self.log_dir / f"{timestamp}.log"
        self._log_file = self.log_path.open("a", encoding="utf-8", newline="\n")
        self._original_stdout = sys.stdout
        self._original_stderr = sys.stderr
        self._stream_tee_installed = False

    def append(self, text: str) -> None:
        self._log_file.write(text)
        self._log_file.flush()

    def write_header(self) -> None:
        command = " ".join(self.argv)
        lines = [
            "=== DeltaCmd Session ===",
            f"started_at: {self.started_at.isoformat(timespec='seconds')}",
            f"command: {command}",
            f"automatic: {str(self.automatic).lower()}",
            f"project_root: {self.project_root}",
            f"platform: {sys.platform}",
            f"cwd: {self.cwd}",
            "========================",
            "",
        ]
        self.append("\n".join(lines))

    def write_footer(self, exit_code: int) -> None:
        finished_at = datetime.now().astimezone()
        lines = [
            "",
            "=== DeltaCmd Session End ===",
            f"finished_at: {finished_at.isoformat(timespec='seconds')}",
            f"exit_code: {exit_code}",
            "============================",
            "",
        ]
        self.append("\n".join(lines))
        self._log_file.close()

    def install_stream_tee(self) -> None:
        if self._stream_tee_installed:
            return
        sys.stdout = _TeeStream(self._original_stdout, self)
        sys.stderr = _TeeStream(self._original_stderr, self)
        self._stream_tee_installed = True

    def restore_streams(self) -> None:
        if not self._stream_tee_installed:
            return
        sys.stdout = self._original_stdout
        sys.stderr = self._original_stderr
        self._stream_tee_installed = False

    def log_meta(self, line: str) -> None:
        text = f"{line}\n"
        self._original_stdout.write(text)
        self._original_stdout.flush()
        self.append(text)

    def print_log_path(self) -> None:
        try:
            relative = self.log_path.relative_to(self.project_root)
        except ValueError:
            relative = self.log_path
        message = f"Log saved to: {relative.as_posix()}\n"
        self._original_stdout.write(message)
        self._original_stdout.flush()


def run_subprocess_with_tee(
    argv: list[str] | str,
    *,
    cwd: Path,
    session: SessionLog,
    shell: bool = False,
) -> int:
    process = subprocess.Popen(
        argv,
        cwd=cwd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
        shell=shell,
    )
    assert process.stdout is not None

    def _pump_output() -> None:
        # Write to the original stream (not sys.stdout): sys.stdout is the
        # installed _TeeStream, which would append to the log itself and
        # duplicate every line. Append once here instead.
        for line in process.stdout:
            session._original_stdout.write(line)
            session._original_stdout.flush()
            session.append(line)

    reader = threading.Thread(target=_pump_output, daemon=True)
    reader.start()
    return_code = process.wait()
    reader.join()
    return return_code


def prune_logs(log_dir: Path, *, max_age_days: int = DEFAULT_RETENTION_DAYS) -> None:
    if not log_dir.is_dir():
        return

    cutoff = time.time() - (max_age_days * 86400)
    for path in log_dir.glob("*.log"):
        try:
            if path.stat().st_mtime < cutoff:
                path.unlink()
        except OSError as exc:
            print(f"WARNING: Failed to prune log '{path}': {exc}", file=sys.__stderr__)
