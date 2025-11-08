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
# --------------

def is_blacklisted(path):
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
    return sorted(
        os.path.join(root, file)
        for root, _, files in os.walk('.')
        for file in files
        if file.endswith('.c') and not is_blacklisted(os.path.join(root, file))
    )

def compile_sources(sources, output):
    print("📦 Compiling sources...")
    cmd = ["gcc", "-g","-fsanitize=address", "-I.", "-Ih", "-Wall", "-Wno-deprecated-declarations", "-o", output,
           "-L/usr/lib", "-lcrypto", "-lssl", "-lresolv", "-lcurl"] + sources #, "-Dprintf(...)=my_printf(__VA_ARGS__)"
    result = subprocess.run(cmd)
    return result.returncode == 0

def main():
    binary = "./proxy.o"
    sources = find_c_sources()

    if not sources:
        print("❌ No .c source files found (or all are blacklisted).")
        return

    print(f"🔍 Found sources: {', '.join(sources)}")
    if not compile_sources(sources, binary):
        print("❌ Compilation failed.")
        sys.exit(1)

    print("✅ Compilation successful.")
    print(f"🚀 Running: {binary} {' '.join(sys.argv[1:])}")
    os.execvp(binary, [binary] + sys.argv[1:])

if __name__ == "__main__":
    main()
