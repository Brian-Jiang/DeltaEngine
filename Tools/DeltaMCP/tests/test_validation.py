from validation import validate_operation


def test_valid_query(schemas):
    op = {"type": "query", "system": "scene", "query": "game_objects", "params": {}}
    assert validate_operation(op, schemas) is None


def test_valid_command(schemas):
    op = {
        "type": "command",
        "system": "scene",
        "command": "CreateGameObject",
        "params": {"name": "Sun"},
    }
    assert validate_operation(op, schemas) is None


def test_unknown_system(schemas):
    op = {"type": "query", "system": "nope", "query": "whatever", "params": {}}
    err = validate_operation(op, schemas)
    assert err["stage"] == "system"
    assert "scene" in err["available_systems"]


def test_unknown_command(schemas):
    op = {
        "type": "command",
        "system": "scene",
        "command": "CreateGameObjct",
        "params": {},
    }
    err = validate_operation(op, schemas)
    assert err["stage"] == "operation"
    assert "CreateGameObject" in err["available"]
    assert "CreateGameObject" in err["did_you_mean"]


def test_missing_required_param(schemas):
    op = {
        "type": "command",
        "system": "scene",
        "command": "SetPosition",
        "params": {"objectId": "abc"},
    }
    err = validate_operation(op, schemas)
    assert err["stage"] == "params"
    problems = {(e["param"], e["problem"]) for e in err["param_errors"]}
    assert ("value", "missing_required") in problems


def test_type_mismatch(schemas):
    op = {
        "type": "command",
        "system": "scene",
        "command": "DeleteGameObject",
        "params": {"objectId": 123},
    }
    err = validate_operation(op, schemas)
    assert err["stage"] == "params"
    problems = {(e["param"], e["problem"]) for e in err["param_errors"]}
    assert ("objectId", "type_mismatch") in problems


def test_unknown_param(schemas):
    op = {
        "type": "command",
        "system": "scene",
        "command": "DeleteGameObject",
        "params": {"objectId": "abc", "bogus": 1},
    }
    err = validate_operation(op, schemas)
    assert err["stage"] == "params"
    problems = {(e["param"], e["problem"]) for e in err["param_errors"]}
    assert ("bogus", "unknown") in problems


def test_invalid_option(schemas):
    op = {
        "type": "command",
        "system": "scene",
        "command": "SetPosition",
        "params": {"objectId": "abc", "value": [0, 0, 0], "space": "galactic"},
    }
    err = validate_operation(op, schemas)
    assert err["stage"] == "params"
    problems = {(e["param"], e["problem"]) for e in err["param_errors"]}
    assert ("space", "invalid_option") in problems


def test_optional_param_omitted(schemas):
    op = {
        "type": "command",
        "system": "scene",
        "command": "CreateGameObject",
        "params": {"name": "Sun"},
    }
    assert validate_operation(op, schemas) is None


def test_missing_system_field(schemas):
    op = {"type": "query", "query": "game_objects", "params": {}}
    err = validate_operation(op, schemas)
    assert err["stage"] == "envelope"


def test_invalid_type_field(schemas):
    op = {"type": "mutate", "system": "scene", "command": "CreateGameObject"}
    err = validate_operation(op, schemas)
    assert err["stage"] == "envelope"
