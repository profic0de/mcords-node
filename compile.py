import subprocess
import sys
import os

# --- CONFIG ---
BLACKLIST = [
    "test.c",
    "auth.c",
    "mem.c",
    "old_main.c",
    "temp_test.c"
]
FULL_BINARY = "./proxy.o"
SINGLE_BINARY = "./_temp_run.o"
# --------------

def is_blacklisted(path: str) -> bool:
    """Check if a file or folder is in the blacklist (relative path)."""
    rel_path = os.path.relpath(path, ".")
    for item in BLACKLIST:
        if item.endswith("/"):  # Folder
            if rel_path.startswith(item):
                return True
        else:  # Exact file
            if rel_path == item:
                return True
    return False


def find_c_sources():
    """Return all .c sources that are not blacklisted."""
    return sorted(
        os.path.join(root, file)
        for root, _, files in os.walk(".")
        for file in files
        if file.endswith(".c") and not is_blacklisted(os.path.join(root, file))
    )


def compile_sources(sources, output):
    """Compile multiple source files into a single binary."""
    print("📦 Compiling full project...")
    cmd = [
        "gcc", "-g", "-fsanitize=address",
        "-I.", "-Ih", "-Wall", "-Wno-deprecated-declarations",
        "-o", output,
        "-L/usr/lib", "-lcrypto", "-lssl", "-lresolv", "-lcurl"
    ] + sources

    print("⚙️  " + " ".join(cmd))
    result = subprocess.run(cmd)
    return result.returncode == 0


def compile_single(source, output):
    """Compile and run only the given file (e.g., test or standalone)."""
    print(f"🧩 Compiling single file: {source}")
    cmd = [
        "gcc", "-g", "-fsanitize=address",
        "-I.", "-Ih", "-Wall", "-Wno-deprecated-declarations",
        "-o", output, source,
        "-L/usr/lib", "-lcrypto", "-lssl", "-lresolv", "-lcurl"
    ]
    print("⚙️  " + " ".join(cmd))
    result = subprocess.run(cmd)
    return result.returncode == 0


def main():
    # Detect file argument
    file_arg = sys.argv[1] if len(sys.argv) > 1 else None
    if file_arg:
        file_arg = os.path.abspath(file_arg)
        print(f"📄 Requested file: {file_arg}")
    else:
        print("ℹ️ No file argument provided, performing full build.")
    
    # --- Case 1: Full build ---
    if not file_arg or (not is_blacklisted(file_arg) and os.path.exists(file_arg)):
        sources = find_c_sources()
        if not sources:
            print("❌ No .c source files found (or all are blacklisted).")
            return
        print(f"🔍 Found sources: {', '.join(sources)}")
        if not compile_sources(sources, FULL_BINARY):
            print("❌ Compilation failed.")
            sys.exit(1)
        print("✅ Compilation successful.")
        print(f"🚀 Running: {FULL_BINARY}")
        os.execvp(FULL_BINARY, [FULL_BINARY])
        return

    # --- Case 2: Compile and run single file ---
    if not os.path.exists(file_arg):
        print(f"❌ File not found: {file_arg}")
        sys.exit(1)

    if not compile_single(file_arg, SINGLE_BINARY):
        print("❌ Single file compilation failed.")
        sys.exit(1)

    print("✅ Single file compilation successful.")
    print(f"🚀 Running: {SINGLE_BINARY}")
    os.execvp(SINGLE_BINARY, [SINGLE_BINARY])


if __name__ == "__main__":
    main()
