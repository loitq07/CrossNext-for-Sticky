import os
import sys
import time
import json
import urllib.request
import urllib.parse

IP = sys.argv[1] if len(sys.argv) > 1 else "192.168.0.174"
PAIR_CODE = sys.argv[2] if len(sys.argv) > 2 else "966886"
FW_PATH = sys.argv[3] if len(sys.argv) > 3 else r"D:\Projects\Sticky\output\firmware-sticky-web-mobile.bin"

# 1. Status
print(f"Connecting to http://{IP}/api/status ...", flush=True)
try:
    req = urllib.request.Request(f"http://{IP}/api/status")
    with urllib.request.urlopen(req, timeout=5) as resp:
        status = json.loads(resp.read().decode())
        print(f"Connected: {status.get('device')} v{status.get('version')} (RSSI {status.get('rssi')} dBm)", flush=True)
except Exception as e:
    print(f"Error connecting: {e}", flush=True)
    sys.exit(1)

# 2. Pair
print(f"Pairing with code {PAIR_CODE} ...", flush=True)
data = urllib.parse.urlencode({"code": PAIR_CODE}).encode()
req = urllib.request.Request(f"http://{IP}/api/dev/pair", data=data, method="POST")
try:
    with urllib.request.urlopen(req, timeout=10) as resp:
        res = json.loads(resp.read().decode())
        token = res.get("token")
        print(f"Paired! Token: {token}", flush=True)
except Exception as e:
    print(f"Error pairing: {e}", flush=True)
    sys.exit(1)

# 3. Upload firmware
file_size = os.path.getsize(FW_PATH)
print(f"Uploading {file_size} bytes to device ...", flush=True)

class ProgressStream:
    def __init__(self, filename):
        self.f = open(filename, "rb")
        self.total = os.path.getsize(filename)
        self.read_bytes = 0
        self.start_time = time.time()
        self.last_print = 0

    def read(self, size=-1):
        chunk = self.f.read(size if size > 0 else 65536)
        if chunk:
            self.read_bytes += len(chunk)
            now = time.time()
            if now - self.last_print >= 1.5 or self.read_bytes == self.total:
                pct = self.read_bytes * 100.0 / self.total
                elapsed = now - self.start_time
                speed = (self.read_bytes / 1024.0) / elapsed if elapsed > 0 else 0
                print(f"  Progress: {pct:.1f}% ({self.read_bytes}/{self.total} bytes) at {speed:.1f} KB/s", flush=True)
                self.last_print = now
        return chunk

stream = ProgressStream(FW_PATH)
req = urllib.request.Request(
    f"http://{IP}/api/dev/upload",
    data=stream,
    headers={
        "X-Dev-Token": token,
        "Content-Type": "application/octet-stream",
        "Content-Length": str(file_size)
    },
    method="PUT"
)

try:
    with urllib.request.urlopen(req, timeout=300) as resp:
        body = resp.read().decode()
        print(f"Upload completed: {body.strip()}", flush=True)
except Exception as e:
    print(f"Upload failed: {e}", flush=True)
    sys.exit(1)

# 4. Flash
print("Flashing firmware to OTA partition (takes ~1 minute)...", flush=True)
req = urllib.request.Request(
    f"http://{IP}/api/dev/flash",
    headers={"X-Dev-Token": token},
    data=b"",
    method="POST"
)

try:
    with urllib.request.urlopen(req, timeout=300) as resp:
        result = resp.read().decode()
        print(f"Flash response: {result.strip()}", flush=True)
except Exception as e:
    print(f"Flash request note: {e}", flush=True)

print("Waiting for device reboot...", flush=True)
time.sleep(10)

for attempt in range(25):
    try:
        req = urllib.request.Request(f"http://{IP}/api/status")
        with urllib.request.urlopen(req, timeout=3) as resp:
            status = json.loads(resp.read().decode())
            print(f"Device restarted successfully! {status.get('device')} v{status.get('version')} (uptime: {status.get('uptime')}s)", flush=True)
            sys.exit(0)
    except Exception:
        time.sleep(2)

print("Device reboot timed out or Wi-Fi reconnecting.", flush=True)
