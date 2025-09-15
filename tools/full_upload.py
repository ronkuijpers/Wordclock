Import("env")
from SCons.Script import Alias, AlwaysBuild
import socket
import time

# Ensure uploadfs runs after upload
env.Depends("uploadfs", "upload")

def wait_for_device_http(*args, **kwargs):
    """When using OTA, give the device time to reboot.
    Probe HTTP port 80 as a readiness signal (webserver comes up after WiFi).
    """
    proto = (env.GetProjectOption("upload_protocol") or "").lower()
    if proto != "espota":
        return
    host = env.GetProjectOption("upload_port") or "192.168.1.101"
    port = 80
    # Small initial delay to allow reboot
    time.sleep(2)
    deadline = time.time() + 60
    print(f"[upload_all] Waiting for device HTTP {host}:{port} ...")
    while time.time() < deadline:
        try:
            with socket.create_connection((host, port), timeout=2):
                print("[upload_all] Device HTTP is up")
                return
        except OSError:
            time.sleep(1)
    print("[upload_all] Warning: device HTTP not up after 60s; continuing anyway")

# Before uploadfs, wait for device to reboot and webserver to be ready when using espota
env.AddPreAction("uploadfs", wait_for_device_http)

# Define an alias that depends on both PIO targets: 'upload' and 'uploadfs'.
# Usage: pio run -t upload_all
upload_all = Alias("upload_all", ["upload", "uploadfs"])
AlwaysBuild(upload_all)
