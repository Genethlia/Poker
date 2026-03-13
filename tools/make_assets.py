from pathlib import Path
import re

PROJECT_ROOT = Path(__file__).resolve().parent.parent
ASSETS_DIR = PROJECT_ROOT / "assets"
OUTPUT_FILE = PROJECT_ROOT  / "assets.h"

def sanitize_name(filename: str) -> str:
    name = filename.lower()
    name = re.sub(r'[^a-z0-9]+', '_', name)
    name = re.sub(r'_+', '_', name).strip('_')
    return name

def file_to_cpp_array(path: Path, var_name: str) -> str:
    data = path.read_bytes()

    lines = []
    for i in range(0, len(data), 12):
        chunk = data[i:i+12]
        line = ", ".join(f"0x{b:02x}" for b in chunk)
        lines.append("    " + line)

    array_data = ",\n".join(lines)

    return f"""static const unsigned char {var_name}[] = {{
{array_data}
}};

static const unsigned int {var_name}_len = {len(data)};
"""

def main():
    files = sorted([p for p in ASSETS_DIR.rglob("*") if p.is_file()])

    print("ASSETS_DIR =", ASSETS_DIR)
    print("FOUND FILES =", [p.name for p in files])
    
    header_parts = []
    header_parts.append("#pragma once\n")
    header_parts.append("// Auto-generated file. Do not edit manually.\n\n")

    for file in files:
        var_name = sanitize_name(file.name)
        header_parts.append(f"// {file.name}\n")
        header_parts.append(file_to_cpp_array(file, var_name))
        header_parts.append("\n")

    OUTPUT_FILE.write_text("".join(header_parts), encoding="utf-8")
    print(f"Generated: {OUTPUT_FILE}")

if __name__ == "__main__":
    main()