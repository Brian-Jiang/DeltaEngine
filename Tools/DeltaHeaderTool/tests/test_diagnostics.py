from diagnostics import Diagnostic, DiagnosticCollector


def test_warnings_accumulate():
    d = DiagnosticCollector()
    d.warn("a.h", 1, "first")
    d.warn("b.h", 2, "second")
    assert len(d.warnings) == 2
    assert d.warnings[0].message == "first"
    assert d.warnings[1].file == "b.h"


def test_duplicate_warnings_not_deduplicated():
    d = DiagnosticCollector()
    d.warn("f.h", 10, "same")
    d.warn("f.h", 10, "same")
    assert len(d.warnings) == 2
    assert all(w.message == "same" for w in d.warnings)


def test_clear_warnings_via_list():
    d = DiagnosticCollector()
    d.warn("x.h", 3, "w")
    d.warnings.clear()
    assert d.warnings == []


def test_emit_all_format(capsys):
    d = DiagnosticCollector()
    d.warn("src/Foo.h", 42, "bad thing")
    d.emit_all()
    err = capsys.readouterr().err
    assert "[DeltaHeaderTool WARNING]" in err
    assert "src/Foo.h:42:" in err
    assert "bad thing" in err


def test_diagnostic_format():
    diag = Diagnostic(file="g.h", line=7, message="msg")
    assert diag.format() == "[DeltaHeaderTool WARNING] g.h:7: msg"
