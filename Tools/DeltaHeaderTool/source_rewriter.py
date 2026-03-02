from __future__ import annotations

import re
from dataclasses import dataclass, field
from pathlib import Path

from parser import ClassInfo


@dataclass
class RewriteResult:
    changed: bool = False
    extra_cpp_definitions: list[str] = field(default_factory=list)
    warnings: list[str] = field(default_factory=list)


def rewrite_header(header_path: Path, classes: list[ClassInfo]) -> RewriteResult:
    result = RewriteResult()
    if not classes:
        return result

    text = header_path.read_text(encoding="utf-8", errors="replace")
    stripped_template_text = re.sub(
        r"(?m)^(?P<indent>\s*)DFUNCTION\s*\([^)]*\)\s*\n(?=\s*template\s*<)",
        "",
        text,
    )
    if stripped_template_text != text:
        text = stripped_template_text
        result.changed = True
    lines = text.splitlines(keepends=True)
    class_map = {c.name: c for c in classes}

    # 1) Remove DFUNCTION macro on templated methods.
    template_macro_lines = sorted(
        {
            macro_line
            for c in classes
            for macro_line in c.template_dfunction_macro_lines
            if macro_line > 0
        },
        reverse=True,
    )
    for one_based_line in template_macro_lines:
        idx = one_based_line - 1
        if 0 <= idx < len(lines) and "DFUNCTION" in lines[idx]:
            lines[idx] = ""
            result.changed = True

    # 2) Move inline DFUNCTION one-line definitions out-of-line.
    for c in classes:
        for fn in sorted(
            (f for f in c.functions if f.is_inline and f.line > 0),
            key=lambda x: x.line,
            reverse=True,
        ):
            idx = fn.line - 1
            if idx < 0 or idx >= len(lines):
                continue

            target_idx = idx
            line = lines[target_idx].rstrip("\r\n")
            if "{" not in line or "}" not in line:
                for probe in range(idx, min(idx + 6, len(lines))):
                    probe_line = lines[probe].rstrip("\r\n")
                    if fn.name in probe_line and "{" in probe_line and "}" in probe_line:
                        target_idx = probe
                        line = probe_line
                        break

            if "{" not in line or "}" not in line:
                result.warnings.append(
                    (
                        f"WARNING: [{header_path.as_posix()}:{fn.line}] "
                        f"DFUNCTION on '{c.name}::{fn.name}' is inline but has a "
                        "multi-line body; rewrite skipped."
                    )
                )
                continue

            left = line[: line.index("{")].rstrip()
            body = line[line.index("{") + 1 : line.rindex("}")].strip()
            decl_no_inline = re.sub(r"\binline\s+", "", left)
            header_decl = decl_no_inline.rstrip() + ";"
            indent = re.match(r"^\s*", lines[target_idx]).group(0)
            lines[target_idx] = f"{indent}{header_decl.lstrip()}\n"
            result.changed = True

            moved = _build_out_of_line_definition(c.name, decl_no_inline, body)
            if moved:
                result.extra_cpp_definitions.append(moved)
            else:
                result.warnings.append(
                    (
                        f"WARNING: [{header_path.as_posix()}:{fn.line}] "
                        f"Failed to move inline body for '{c.name}::{fn.name}'."
                    )
                )

    updated_text = "".join(lines)

    # 3) Non-default ctor -> Initialize + delete declarations.
    for c in classes:
        if c.is_struct:
            continue
        updated_text, changed, class_cpp_defs, class_warnings = _rewrite_class_constructors(
            updated_text,
            c,
            header_path,
        )
        if changed:
            result.changed = True
        result.extra_cpp_definitions.extend(class_cpp_defs)
        result.warnings.extend(class_warnings)

    if result.changed:
        header_path.write_text(updated_text, encoding="utf-8", newline="\n")

    return result


def rewrite_source_cpp(header_path: Path, classes: list[ClassInfo]) -> RewriteResult:
    """Remove non-default reflected constructor definitions from companion .cpp files.

    Once headers declare non-default constructors as deleted, keeping .cpp constructor
    bodies causes ODR errors. This pass strips those out-of-line definitions.
    """
    result = RewriteResult()
    source_cpp = header_path.with_suffix(".cpp")
    if not source_cpp.exists():
        return result

    text = source_cpp.read_text(encoding="utf-8", errors="replace")
    updated = text

    for cls in classes:
        if cls.is_struct:
            continue
        marker = f"{cls.name}::{cls.name}("
        cursor = 0
        while True:
            pos = updated.find(marker, cursor)
            if pos == -1:
                break

            paren_open = updated.find("(", pos)
            paren_close = _find_matching(updated, paren_open, "(", ")")
            if paren_close == -1:
                cursor = pos + len(marker)
                continue

            params = updated[paren_open + 1 : paren_close].strip()
            if not params:
                cursor = paren_close + 1
                continue

            body_open = updated.find("{", paren_close)
            if body_open == -1:
                cursor = paren_close + 1
                continue
            body_close = _find_matching(updated, body_open, "{", "}")
            if body_close == -1:
                cursor = body_open + 1
                continue

            line_start = updated.rfind("\n", 0, pos)
            line_start = 0 if line_start == -1 else line_start + 1
            remove_end = body_close + 1
            while remove_end < len(updated) and updated[remove_end] in ("\r", "\n"):
                remove_end += 1

            updated = updated[:line_start] + updated[remove_end:]
            result.changed = True
            cursor = line_start

    if result.changed and updated != text:
        source_cpp.write_text(updated, encoding="utf-8", newline="\n")

    return result


def _rewrite_class_constructors(
    text: str,
    cls: ClassInfo,
    header_path: Path,
) -> tuple[str, bool, list[str], list[str]]:
    # Clean previously misplaced Initialize declarations between DCLASS() and class keyword.
    class_decl_pos = text.find(f"class {cls.name}")
    if class_decl_pos != -1:
        pre_start = max(0, class_decl_pos - 500)
        pre = text[pre_start:class_decl_pos]
        cleaned_pre = re.sub(
            r"(?m)^\s*DELTAENGINE_API\s+void\s+Initialize\([^)]*\);\s*\n",
            "",
            pre,
        )
        if cleaned_pre != pre:
            text = text[:pre_start] + cleaned_pre + text[class_decl_pos:]

    class_bounds = _find_class_block(text, cls.name)
    if class_bounds is None:
        return text, False, [], []

    class_start, class_end = class_bounds
    class_block = text[class_start:class_end]
    class_body = class_block[class_block.index("{") + 1 : class_block.rfind("}")]
    member_names = _collect_member_names(class_body)

    ctor_decl_re = re.compile(
        rf"(?m)^(?P<indent>\s*)(?:(?P<api>DELTAENGINE_API)\s+)?{re.escape(cls.name)}\s*\((?P<params>[^)]*)\)\s*(?:=\s*delete)?\s*;\s*$"
    )
    ctor_inline_re = re.compile(
        rf"(?m)^(?P<indent>\s*)(?:(?P<api>DELTAENGINE_API)\s+)?{re.escape(cls.name)}\s*\((?P<params>[^)]*)\)\s*(?::[^\{{\n\r]*)?\{{[^\{{\}}]*\}}\s*$"
    )
    ctor_matches = list(ctor_decl_re.finditer(class_block))
    inline_ctor_matches = list(ctor_inline_re.finditer(class_block))
    if not ctor_matches and not inline_ctor_matches:
        return text, False, [], []

    changed = False
    extra_cpp_defs: list[str] = []
    warnings: list[str] = []
    replacements: list[tuple[int, int, str]] = []
    existing_initialize_decl = {
        m.group("params").strip()
        for m in re.finditer(
            r"(?m)^\s*(?:DELTAENGINE_API\s+)?void\s+Initialize\s*\((?P<params>[^)]*)\)\s*;\s*$",
            class_block,
        )
    }

    all_matches = sorted(ctor_matches + inline_ctor_matches, key=lambda m: m.start())
    seen_spans: set[tuple[int, int]] = set()
    for m in all_matches:
        span = (m.start(), m.end())
        if span in seen_spans:
            continue
        seen_spans.add(span)
        params = (m.group("params") or "").strip()
        if _is_default_ctor_param_list(params):
            continue

        indent = m.group("indent") or ""
        delete_decl = f"{indent}{cls.name}({params}) = delete;\n"
        replacement = delete_decl
        changed = True

        if params not in existing_initialize_decl:
            replacement += f"{indent}DELTAENGINE_API void Initialize({params});\n"
            existing_initialize_decl.add(params)

        replacements.append((m.start(), m.end(), replacement))

        init_body, unmatched = _build_initialize_body(
            cls.name,
            params,
            member_names,
        )
        extra_cpp_defs.append(init_body)
        for param_name in unmatched:
            warnings.append(
                (
                    f"WARNING: [{header_path.as_posix()}:{cls.line}] "
                    f"Initialize generation for '{cls.name}' could not map parameter "
                    f"'{param_name}' to a member field."
                )
            )

    if not changed:
        return text, False, [], []

    replacements.sort(key=lambda x: x[0], reverse=True)
    rebuilt = class_block
    for start, end, rep in replacements:
        rebuilt = rebuilt[:start] + rep + rebuilt[end:]

    new_text = text[:class_start] + rebuilt + text[class_end:]
    return new_text, True, extra_cpp_defs, warnings


def _find_class_block(text: str, class_name: str) -> tuple[int, int] | None:
    class_re = re.compile(rf"\bclass\s+{re.escape(class_name)}\b")
    m = class_re.search(text)
    if not m:
        return None

    open_brace = text.find("{", m.end())
    if open_brace == -1:
        return None

    depth = 0
    for i in range(open_brace, len(text)):
        ch = text[i]
        if ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                semi = text.find(";", i)
                if semi == -1:
                    return None
                return m.start(), semi + 1
    return None


def _find_matching(text: str, start_idx: int, open_ch: str, close_ch: str) -> int:
    depth = 0
    for i in range(start_idx, len(text)):
        ch = text[i]
        if ch == open_ch:
            depth += 1
        elif ch == close_ch:
            depth -= 1
            if depth == 0:
                return i
    return -1


def _collect_member_names(class_body: str) -> set[str]:
    members = set()
    for m in re.finditer(r"\bm_[A-Za-z_][A-Za-z0-9_]*\b", class_body):
        members.add(m.group(0))
    return members


def _split_params(params: str) -> list[tuple[str, str]]:
    raw = []
    token = []
    depth = 0
    for ch in params:
        if ch == "<":
            depth += 1
            token.append(ch)
        elif ch == ">":
            depth -= 1
            token.append(ch)
        elif ch == "," and depth == 0:
            raw.append("".join(token).strip())
            token = []
        else:
            token.append(ch)
    if token:
        raw.append("".join(token).strip())

    out: list[tuple[str, str]] = []
    for p in raw:
        if not p:
            continue
        p = re.sub(r"\s*=\s*.*$", "", p).strip()
        m = re.match(r"(.+?)\s+([A-Za-z_][A-Za-z0-9_]*)$", p)
        if not m:
            out.append((p, "arg"))
            continue
        out.append((m.group(1).strip(), m.group(2).strip()))
    return out


def _is_default_ctor_param_list(params: str) -> bool:
    p = params.strip()
    if not p:
        return True
    # Treat all-defaulted params as non-default for safety in this migration.
    return False


def _build_initialize_body(
    class_name: str,
    params_text: str,
    member_names: set[str],
) -> tuple[str, list[str]]:
    params = _split_params(params_text)
    assignments: list[str] = []
    unmatched: list[str] = []

    for _, pname in params:
        target = _map_param_to_member(pname, member_names)
        if target:
            assignments.append(f"    {target} = {pname};")
        else:
            unmatched.append(pname)

    if not assignments:
        assignments.append("    // No direct parameter-to-member mapping found.")

    body = (
        f"void {class_name}::Initialize({params_text})\n"
        "{\n"
        f"{'\n'.join(assignments)}\n"
        "}\n"
    )
    return body, unmatched


def _map_param_to_member(param_name: str, member_names: set[str]) -> str | None:
    direct = f"m_{param_name}"
    if direct in member_names:
        return direct

    # Common naming mismatch used in this codebase.
    if param_name == "filePath" and "m_sourcePath" in member_names:
        return "m_sourcePath"

    if param_name.endswith("Path") and "m_sourcePath" in member_names:
        return "m_sourcePath"

    lowered = param_name[:1].lower() + param_name[1:]
    maybe = f"m_{lowered}"
    if maybe in member_names:
        return maybe

    return None


def _build_out_of_line_definition(class_name: str, decl_no_inline: str, body: str) -> str | None:
    decl = decl_no_inline.strip()
    decl = re.sub(r"\bDELTAENGINE_API\b", "", decl).strip()
    m = re.match(
        r"(?P<ret>.+?)\s+(?P<name>[A-Za-z_][A-Za-z0-9_]*)\s*\((?P<params>.*)\)\s*(?P<cv>const)?\s*$",
        decl,
    )
    if not m:
        return None

    ret = m.group("ret").strip()
    name = m.group("name").strip()
    params = m.group("params").strip()
    cv = f" {m.group('cv').strip()}" if m.group("cv") else ""
    return (
        f"{ret} {class_name}::{name}({params}){cv}\n"
        "{\n"
        f"    {body}\n"
        "}\n"
    )
