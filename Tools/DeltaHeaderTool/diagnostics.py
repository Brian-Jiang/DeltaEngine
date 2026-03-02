"""Structured diagnostic collection for DeltaHeaderTool.

Warnings are collected during parsing and emitted after all headers are processed.
Each worker accumulates diagnostics into a list that is returned alongside generated code
so the main process can merge and print them in a single pass.
"""

from __future__ import annotations

import sys
from dataclasses import dataclass, field


@dataclass
class Diagnostic:
    file: str
    line: int
    message: str

    def format(self) -> str:
        return f"[DeltaHeaderTool WARNING] {self.file}:{self.line}: {self.message}"


@dataclass
class DiagnosticCollector:
    warnings: list[Diagnostic] = field(default_factory=list)

    def warn(self, file: str, line: int, message: str) -> None:
        self.warnings.append(Diagnostic(file=file, line=line, message=message))

    def emit_all(self) -> None:
        for diag in self.warnings:
            print(diag.format(), file=sys.stderr)
