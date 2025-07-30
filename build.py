import subprocess
import sys
import os

def find_c_sources():
    return sorted(
        os.path.join(root, file)
        for root, _, files in os.walk('.')
        for file in files
        if file.endswith('.c')
    )

def compile_sources(sources, output):
    print("📦 Compiling sources...")
    cmd = ["gcc", "-I.", "-Wall", "-Wno-deprecated-declarations", "-O2", "-o", output, "-L/usr/lib", "-lcrypto", "-lssl", "-lresolv"] + sources
    result = subprocess.run(cmd)
    return result.returncode == 0

def main():
    binary = "./proxy"
    sources = find_c_sources()

    if not sources:
        print("❌ No .c source files found.")
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
