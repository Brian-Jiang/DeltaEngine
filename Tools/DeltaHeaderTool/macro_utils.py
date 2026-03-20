"""Shared token-based macro detection for DeltaHeaderTool."""

import clang.cindex as ci


def get_tokens_before_cursor(tu, cursor, lookback_lines=5):
    start_line = max(1, cursor.location.line - lookback_lines)
    extent = tu.get_extent(
        cursor.location.file.name,
        ((start_line, 1), (cursor.location.line, cursor.location.column)),
    )
    return list(tu.get_tokens(extent=extent))


def is_annotated(tu, cursor, macro_name: str) -> bool:
    tokens = get_tokens_before_cursor(tu, cursor)
    for tok in reversed(tokens):
        if tok.spelling == macro_name:
            return True
        if tok.spelling == ";":
            return False
    return False
