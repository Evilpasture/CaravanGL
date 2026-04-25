import re
import os

# --- Path Logic ---
SCRIPT_DIR: str = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR: str = os.path.dirname(SCRIPT_DIR)

HEADER_PATH: str = os.path.join(ROOT_DIR, "include", "caravangl_core.h")
INC_OUTPUT: str = os.path.join(ROOT_DIR, "src", "caravangl", "gl_constants.inc")
PYI_OUTPUT: str = os.path.join(ROOT_DIR, "src", "caravangl", "constants.pyi")
INIT_PYI_PATH: str = os.path.join(ROOT_DIR, "src", "caravangl", "__init__.pyi")

# --- Regex Patterns ---
# ENUM_BLOCK_RE: Handles "typedef enum Name : type {"
ENUM_BLOCK_RE: re.Pattern[str] = re.compile(
    r"typedef enum (\w+)\s*(?::\s*\w+)?\s*\{([^}]+)\}", 
    re.MULTILINE
)

# ENUM_MEMBER_RE: Captures name and optional value.
# Now that comments are stripped, this is safe.
ENUM_MEMBER_RE: re.Pattern[str] = re.compile(r"(\w+)\s*(?:=\s*([^,}\s/]+))?")

# CONSTEXPR_RE: Captures standalone constants
CONSTEXPR_RE: re.Pattern[str] = re.compile(r"static constexpr \w+ (\w+)\s*=\s*([^;]+);")

GroupData = list[dict[str, str | list[tuple[str, str]]]]

def parse_header() -> GroupData:
    if not os.path.exists(HEADER_PATH):
        print(f"Error: Could not find header at {HEADER_PATH}")
        exit(1)

    with open(HEADER_PATH, "r", encoding="utf-8") as f:
        raw_content: str = f.read()

    # --- STEP 1: STRIP COMMENTS ---
    # Remove // comments
    content: str = re.sub(r"//.*", "", raw_content)
    # Remove /* */ comments
    content = re.sub(r"/\*.*?\*/", "", content, flags=re.DOTALL)

    groups: GroupData = []
    seen_py_names: set[str] = set()

    # 2. Parse Core Constants
    core_entries: list[tuple[str, str]] = []
    for match in CONSTEXPR_RE.finditer(content):
        c_name: str = match.group(1)
        py_name: str = c_name[3:] if c_name.startswith("GL_") else c_name
        if py_name not in seen_py_names:
            core_entries.append((py_name, c_name))
            seen_py_names.add(py_name)
    
    if core_entries:
        groups.append({"group": "Core Constants", "entries": core_entries})

    # 3. Parse Enum Blocks
    for block_match in ENUM_BLOCK_RE.finditer(content):
        group_name: str = block_match.group(1)
        body: str = block_match.group(2)
        
        entries: list[tuple[str, str]] = []
        for m in ENUM_MEMBER_RE.finditer(body):
            c_name: str = m.group(1)
            
            # Skip if it's a numeric literal that the regex caught as a "word"
            if c_name[0].isdigit():
                continue
                
            if c_name.endswith(("_COUNT", "_TUPLE_SIZE")):
                continue
            
            py_name: str = c_name[3:] if c_name.startswith("GL_") else c_name
            
            if py_name and py_name not in seen_py_names:
                entries.append((py_name, c_name))
                seen_py_names.add(py_name)
        
        if entries:
            groups.append({"group": group_name, "entries": entries})

    return groups

def write_inc(groups: GroupData) -> None:
    os.makedirs(os.path.dirname(INC_OUTPUT), exist_ok=True)
    with open(INC_OUTPUT, "w", encoding="utf-8", newline="\n") as f:
        f.write("// Automatically generated. DO NOT EDIT.\n\n")
        for g in groups:
            f.write(f"// --- {g['group']} ---\n")
            for py, c in g['entries']: # type: ignore
                f.write(f"X({py}, {c})\n")
            f.write("\n")
    print(f"Created: {INC_OUTPUT}")

def write_pyi(groups: GroupData) -> None:
    os.makedirs(os.path.dirname(PYI_OUTPUT), exist_ok=True)
    all_const_names: list[str] = []
    for g in groups:
        for py, _ in g['entries']: # type: ignore
            all_const_names.append(py)

    with open(PYI_OUTPUT, "w", encoding="utf-8", newline="\n") as f:
        f.write("# Automatically generated. DO NOT EDIT.\n")
        f.write("from typing import Final\n\n")
        for g in groups:
            f.write(f"# --- {g['group']} ---\n")
            for py, _ in g['entries']: # type: ignore
                f.write(f"{py}: Final[int]\n")
            f.write("\n")
        f.write("__all__ = [\n")
        for name in all_const_names:
            f.write(f'    "{name}",\n')
        f.write("]\n")
    print(f"Created: {PYI_OUTPUT}")

def update_init_pyi(groups: GroupData) -> None:
    if not os.path.exists(INIT_PYI_PATH):
        print(f"Warning: {INIT_PYI_PATH} not found.")
        return

    py_names: list[str] = []
    for g in groups:
        for py, _ in g['entries']: # type: ignore
            py_names.append(f'    "{py}",')
    
    with open(INIT_PYI_PATH, "r", encoding="utf-8") as f:
        content: str = f.read()

    pattern: re.Pattern[str] = re.compile(
        r"([ \t]*# @GENERATED_CONSTANTS_START@).*?([ \t]*# @GENERATED_CONSTANTS_END@)", 
        re.DOTALL
    )

    if not pattern.search(content):
        print(f"!! Error: Markers not found in {INIT_PYI_PATH} !!")
        return

    const_block: str = "\n".join(py_names)
    replacement: str = f"\\1\n{const_block}\n\\2"
    new_content: str = pattern.sub(replacement, content)

    with open(INIT_PYI_PATH, "w", encoding="utf-8", newline="\n") as f:
        f.write(new_content)
    print(f"Updated __all__ list in: {INIT_PYI_PATH}")

if __name__ == "__main__":
    data: GroupData = parse_header()
    write_inc(data)
    write_pyi(data)
    update_init_pyi(data)