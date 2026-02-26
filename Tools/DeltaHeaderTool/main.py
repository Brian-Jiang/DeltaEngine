import clang.cindex
import pathlib 

if __name__ == "__main__":
    TOOLS_DIR = pathlib.Path(__file__).parent.parent
    lib_path = TOOLS_DIR / "Clang"
    clang.cindex.Config.set_library_path(str(lib_path))
    print(clang.cindex)

    # Create a simple in-memory C++ snippet and parse it
    index = clang.cindex.Index.create()
    
    test_code = """
    class Foo {
    public:
        int bar(float x, bool flag);
        float m_value;
    };
    """
    
    # Parse from an unsaved file (in-memory)
    tu = index.parse(
        "test.cpp",
        unsaved_files=[("test.cpp", test_code)],
        args=["-std=c++23"]
    )
    
    # Check for parse errors
    errors = [d for d in tu.diagnostics if d.severity >= clang.cindex.Diagnostic.Error]
    if errors:
        print("Parse errors:")
        for e in errors:
            print(f"  {e.spelling}")
    else:
        print("Parsed successfully, no errors")
    
    # Walk the AST and print what we find
    def walk(cursor, indent=0):
        print(f"{'  ' * indent}{cursor.kind.name} | {cursor.spelling} | {cursor.type.spelling}")
        for child in cursor.get_children():
            walk(child, indent + 1)
    
    walk(tu.cursor)