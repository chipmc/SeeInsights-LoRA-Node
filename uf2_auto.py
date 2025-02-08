
Import("env")

import os
import shutil
import time

print("\n[DEBUG] uf2_auto.py is running...")

# Define paths
PROJECT_DIR = env.subst("$PROJECT_DIR")
BUILD_DIR = env.subst("$BUILD_DIR")
BIN_FILE = os.path.join(BUILD_DIR, "firmware.bin")
UF2_FILE = os.path.join(BUILD_DIR, "firmware.uf2")
UF2_TOOL = os.path.join(PROJECT_DIR, "uf2conv.py")

def create_uf2(source, target, env):
    print("[DEBUG] Starting UF2 conversion...")

    if not os.path.exists(BIN_FILE):
        print(f"[ERROR] BIN file not found: {BIN_FILE}")
        return

    print(f"[DEBUG] Running UF2 conversion: {UF2_TOOL}")
    cmd = f"python3 {UF2_TOOL} -c -b 0x2000 -o {UF2_FILE} {BIN_FILE}"
    result = env.Execute(cmd)

    if result == 0 and os.path.exists(UF2_FILE):
        print(f"[SUCCESS] UF2 file created: {UF2_FILE}")
        upload_uf2(UF2_FILE)
    else:
        print("[ERROR] UF2 conversion failed. Check if `uf2conv.py` works manually.")

def upload_uf2(uf2_path):
    print("\n[DEBUG] Searching for Feather M0 in BOOT mode...")

    volumes_path = "/Volumes"
    featherboot_path = None

    if os.path.exists(volumes_path):
        for item in os.listdir(volumes_path):
            print(f"[DEBUG] Found volume: {item}")
            if "FEATHERBOOT" in item.upper():
                featherboot_path = os.path.join(volumes_path, item)
                break

    if featherboot_path:
        print(f"[INFO] Found FEATHERBOOT at {featherboot_path}. Uploading UF2...")
        try:
            time.sleep(2)  # Ensure mount is stable
            shutil.copy(uf2_path, os.path.join(featherboot_path, "NEW.UF2"))
            print("[SUCCESS] UF2 file uploaded! Board should reboot.")
        except Exception as e:
            print(f"[ERROR] Failed to copy UF2 file: {e}")
    else:
        print("[WARNING] FEATHERBOOT not found. Double-tap reset and try again.")

# Attach UF2 conversion to PlatformIO's build process
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", create_uf2)