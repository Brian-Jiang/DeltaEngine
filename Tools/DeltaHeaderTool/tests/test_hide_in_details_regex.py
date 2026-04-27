import re


def test_hide_in_details_token_in_dproperty_args():
    assert re.search(r"\bHideInDetails\b", "HideInDetails")
    assert re.search(r"\bHideInDetails\b", "EditorOnly, HideInDetails")
    assert re.search(r"\bHideInDetails\b", "HideInDetails, meta=(UIType=\"Color\")")
    assert not re.search(r"\bHideInDetails\b", "EditorOnly")
