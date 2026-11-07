#!/usr/bin/env bash
# Flash a brand-new lamp over the USB-serial adapter – wipes it and installs the current firmware.
#
#   ./tools/flash-new.sh                   # name "Candle", address candle.local
#   ./tools/flash-new.sh "Porch pumpkin"   # name "Porch pumpkin", address porch-pumpkin.local
#   PORT=/dev/cu.usbserial-1420 ./tools/flash-new.sh   # pick the adapter yourself
set -euo pipefail
cd "$(dirname "$0")/.."

PIO=$(command -v pio || echo "$HOME/.platformio/penv/bin/pio")
PY="$HOME/.platformio/penv/bin/python"
ESPTOOL="$HOME/.platformio/packages/tool-esptoolpy/esptool.py"
bold() { printf '\033[1m%s\033[0m\n' "$*"; }
die() { printf '\033[31m✗ %s\033[0m\n' "$*" >&2; exit 1; }

[ -x "$PIO" ] || die "PlatformIO not found – install it first: https://platformio.org/install/cli"

name="${1:-Candle}"
host=$(LAMP="$name" python3 -c '
import os, re, unicodedata
s = unicodedata.normalize("NFKD", os.environ["LAMP"]).encode("ascii", "ignore").decode().lower()
print(re.sub(r"[^a-z0-9]+", "-", s).strip("-")[:24].strip("-"))')
[ -n "$host" ] || die "Can't make a network address out of \"$name\" – use some letters or digits."

port="${PORT:-$(ls /dev/cu.usbserial* /dev/cu.wchusbserial* /dev/ttyUSB* 2>/dev/null | head -1 || true)}"
[ -n "$port" ] || die "No USB-serial adapter found – plug it in (or set PORT=…)."

bold "🎃 New lamp: \"$name\" → http://$host.local   (adapter $port)"
echo "   1. Jumper on header pins 4–5 (GPIO0 → GND)"
echo "   2. Adapter TX/RX/GND connected, board powered from 5 V"
if [ -t 0 ]; then read -rp "   Ready? Press Enter… "; fi

bold "🔨 Building…"
LAMP_NAME="$name" LAMP_HOST="$host" "$PIO" run -e serial >/dev/null || die "Build failed – run: pio run -e serial"

bold "⚡ Wiping and flashing (about a minute)…"
if ! "$PY" "$ESPTOOL" --chip esp8266 --port "$port" --baud 115200 write_flash --erase-all 0x0 .pio/build/serial/firmware.bin; then
  die "Flashing failed. Check: jumper on 4–5, 5 V power (not the 3.3 V pin of an Uno!), TX↔RX crossed. Then power-cycle the board and try again."
fi

bold "✅ Done!"
echo "   1. Remove the jumper and power-cycle the board"
echo "   2. On your phone join the WiFi \"$host-setup\" → Advanced → WiFi → pick your network"
echo "   3. Back on your WiFi open http://$host.local"
