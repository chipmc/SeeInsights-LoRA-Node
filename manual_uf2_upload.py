import os
import shutil
import time
import platform

# Detect OS
IS_WINDOWS = platform.system() == "Windows"

# Define paths
PROJECT_DIR = os.path.dirname(os.path.abspath(__file__))
BUILD_DIR = os.path.join(PROJECT_DIR, ".pio", "build", "adafruit_feather_m0")
UF2_FILE = os.path.join(BUILD_DIR, "firmware.uf2")

def find_featherboot():
    """Find the Feather M0 UF2 bootloader mount point."""
    if IS_WINDOWS:
        # Windows: Find a removable drive labeled "FEATHERBOOT"
        import win32api
        drives = win32api.GetLogicalDriveStrings().split('\000')[:-1]
        for drive in drives:
            if os.path.exists(os.path.join(drive, "INFO_UF2.TXT")):
                return drive  # Return drive letter (e.g., "D:\")
    else:
        # macOS/Linux: Find "/Volumes/FEATHERBOOT"
        volumes_path = "/Volumes"
        if os.path.exists(volumes_path):
            for item in os.listdir(volumes_path):
                if "FEATHERBOOT" in item.upper():
                    return os.path.join(volumes_path, item)
    return None

def upload_uf2():
    """Upload the UF2 file to the Feather M0 if in bootloader mode."""
    print("\n[DEBUG] Looking for firmware.uf2...")

    if not os.path.exists(UF2_FILE):
        print(f"[ERROR] UF2 file not found: {UF2_FILE}")
        print("[HINT] Run `pio run` to generate the UF2 file.")
        return
    
    featherboot_path = find_featherboot()

    if featherboot_path:
        print(f"[INFO] Found FEATHERBOOT at {featherboot_path}. Uploading UF2...")
        try:
            time.sleep(2)  # Ensure USB drive is mounted
            dest_file = os.path.join(featherboot_path, "NEW.UF2")
            shutil.copy(UF2_FILE, dest_file)
            print("[SUCCESS] UF2 file uploaded! Board should reboot.")
        except Exception as e:
            print(f"[ERROR] Failed to copy UF2 file: {e}")
    else:
        print("[WARNING] FEATHERBOOT not found. Double-tap reset and try again.")

if __name__ == "__main__":
    upload_uf2()