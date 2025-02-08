
Import("env")

import os
import shutil
import time
import platform

print("\n[DEBUG] uf2_auto.py is running...")

# Detect OS
IS_WINDOWS = platform.system() == "Windows"

# Define paths
PROJECT_DIR = env.subst("$PROJECT_DIR")
BUILD_DIR = env.subst("$BUILD_DIR")
BIN_FILE = os.path.join(BUILD_DIR, "firmware.bin")
UF2_FILE = os.path.join(BUILD_DIR, "firmware.uf2")
UF2_TOOL = os.path.join(PROJECT_DIR, "uf2conv.py")

def find_featherboot():
    """Find the Feather M0 UF2 bootloader mount point."""
    if IS_WINDOWS:
        # Windows: Look for a removable drive with "INFO_UF2.TXT"
        try:
            import win32api
            drives = win32api.GetLogicalDriveStrings().split('\000')[:-1]
            for drive in drives:
                if os.path.exists(os.path.join(drive, "INFO_UF2.TXT")):
                    return drive  # Return drive letter (e.g., "D:\")
        except ImportError:
            print("[ERROR] Missing 'pywin32' module. Install it with 'pip install pywin32'.")
            return None
    else:
        # macOS/Linux: Look for "/Volumes/FEATHERBOOT"
        volumes_path = "/Volumes"
        if os.path.exists(volumes_path):
            for item in os.listdir(volumes_path):
                if "FEATHERBOOT" in item.upper():
                    return os.path.join(volumes_path, item)
    return None

def create_uf2(source, target, env):
    print("[DEBUG] Converting BIN to UF2...")

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

    featherboot_path = find_featherboot()

    if featherboot_path:
        print(f"[INFO] Found FEATHERBOOT at {featherboot_path}. Uploading UF2...")
        try:
            time.sleep(2)  # Ensure USB drive is mounted
            dest_file = os.path.join(featherboot_path, "NEW.UF2")
            shutil.copy(uf2_path, dest_file)
            print("[SUCCESS] UF2 file uploaded! Board should reboot.")
        except Exception as e:
            print(f"[ERROR] Failed to copy UF2 file: {e}")
    else:
        print("[WARNING] FEATHERBOOT not found. Double-tap reset and try again.")

# Attach UF2 conversion to PlatformIO's build process
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", create_uf2)