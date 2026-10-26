# Bakes the firmware version into the build: latest git tag, commit and build time.
import subprocess
from datetime import datetime

Import("env")


def git(*args):
    try:
        return subprocess.check_output(["git", *args], cwd=env["PROJECT_DIR"], stderr=subprocess.DEVNULL).decode().strip()
    except Exception:
        return ""


# e.g. "v1.2.0-3-g1a2b3c4-dirty" = 3 commits after v1.2.0, with uncommitted changes
desc = git("describe", "--tags", "--long", "--dirty", "--always")
parts = desc.split("-")
if len(parts) >= 3 and parts[2].startswith("g"):
    version = parts[0] + ("+" + parts[1] if parts[1] != "0" else "")
    commit = parts[2][1:] + ("-dirty" if parts[-1] == "dirty" else "")
else:  # no tag yet
    version, commit = "dev", desc or "unknown"

env.Append(CPPDEFINES=[
    ("FW_VERSION", env.StringifyMacro(version)),
    ("FW_COMMIT", env.StringifyMacro(commit)),
    ("FW_BUILT", env.StringifyMacro(datetime.now().strftime("%Y-%m-%d %H:%M"))),
])
print("Firmware %s (%s)" % (version, commit))
