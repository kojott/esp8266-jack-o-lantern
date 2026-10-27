# 🎃 WiFi Jack-o'-Lantern

**A flickering "candle" for your carved pumpkin that you control from your phone.**
One ESP8266, four NeoPixels, an evening of soldering. No soot, no burnt lid, no fire brigade.

<p align="center">
  <img src="docs/images/pumpkin.gif" alt="Carved pumpkin glowing with a flickering LED candle" width="420">
</p>

<p align="center">
  <a href="https://platformio.org"><img src="https://img.shields.io/badge/PlatformIO-ready-orange?logo=platformio" alt="PlatformIO"></a>
  <img src="https://img.shields.io/badge/ESP8266-ESP--12F-blue" alt="ESP8266">
  <img src="https://img.shields.io/badge/LEDs-WS2812B-ff6e14" alt="WS2812B">
  <img src="https://img.shields.io/badge/spookiness-100%25-black" alt="Spookiness 100%">
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-MIT-green" alt="MIT license"></a>
</p>

Real candles in pumpkins are a bit of a scam: they cook the pumpkin from the inside, blacken the lid and die
right when the trick-or-treaters show up. This one flickers like a real flame, runs all night from a USB power
bank, and you can switch it from *cosy candle* to *full-on bonfire* from the sofa. 🛋️🔥

▶️ [Watch the full video](docs/images/pumpkin.mp4)

---

## ✨ What it does

- 🕯️ **Realistic candle flicker** – every LED has its own little flame, plus the occasional "draught" that makes
  them all dip together. When the flame dims it also gets redder, just like a real one.
- 🔥 **Three moods** – *Candle*, *Fire (wilder)* and *Steady light*.
- 👻 **Spooky mode** – one button and *every* pumpkin on your WiFi goes blood red, heart-beating, flickering and
  staring at the very same moment. Or let it happen on its own: "every 60 s, for 20 s", then back to cosy.
- 📱 **Web UI on your phone** – brightness, flicker, speed, colour (incl. warm white & spooky green/purple).
- ⏲️ **Sleep timer** – 15 min, 30 min, 1 h or any custom value. Goes out by itself with a smooth fade.
- ⏰ **Set-and-forget schedule** – "on at 17:00, off at 22:00", with internet time and proper time zones/DST.
  Put it on the porch or in the kids' room and forget about it.
- 🔌 **Works like a lamp** – plugging it in always lights it with the last settings, even with no internet.
  Once the clock syncs it simply follows the next on/off step.
- 📶 **No hard-coded WiFi** – first boot opens a hotspot, you pick your network on your phone. You can switch
  networks later from the web UI.
- 🏕️ **Works without WiFi too** – no network in reach (camping, the garden, a party at grandma's)? It runs its own
  hotspot with the full web UI, and borrows your phone's clock so even the schedule keeps working.
- 🏷️ **Name it** – every pumpkin gets its own name and address (`jack.local`, `porch.local`, …), so a whole
  pumpkin army can live on one network.
- 🚀 **Over-the-air updates** – flash once with a cable, then never again.
- 💾 **Remembers everything** across power cuts (and only writes to flash when something actually changed).

<p align="center">
  <img src="docs/images/web-ui.png" alt="Web UI on a phone" width="320">
</p>

## 🛒 Shopping list (~ $8)

| Part | Notes |
|---|---|
| ESP8266 **ESP-12F** module (or ESP-12E) | the brain + WiFi |
| **WS2812B** LED strip, 4 LEDs | cut 4 pixels off any 5 V NeoPixel strip |
| **AMS1117-3.3** regulator module | 5 V → 3.3 V for the ESP |
| 5 × 7 cm perfboard, some wire | |
| 5-pin header + 1 jumper | for flashing |
| USB-to-serial adapter, **3.3 V** | e.g. the cheap CH340 "ESP-01 adapter" – only needed once |
| 5 V USB power bank or phone charger | a 10 000 mAh bank easily lasts the whole night |

> 💡 **Lazy mode:** got a **Wemos D1 mini** or **NodeMCU**? Skip the regulator, header and USB adapter – they're
> already on the board. Connect LED `+5V` → `5V`, `GND` → `G`, `DI` → `D5` (GPIO14) and flash it over USB.
> Literally a 10-minute build.

## 🔌 Wiring

<p align="center">
  <img src="docs/wiring.svg" alt="Wiring diagram" width="100%">
</p>

| From | To | Why |
|---|---|---|
| 5 V input (+) | AMS1117 `VIN` **and** LED strip `+5V` | LEDs want 5 V |
| 5 V input (−) | AMS1117 `GND`, ESP `GND`, LED `GND` | common ground |
| AMS1117 `VOUT` | ESP `VCC` | 3.3 V for the ESP |
| ESP `EN`, `GPIO2` | 3.3 V | boot straps |
| ESP `GPIO15` | GND | boot strap |
| ESP `GPIO14` | LED strip `DI` | data (green wire) |
| ESP `TX`, `RX`, `GPIO0`, `GND` | 5-pin header | for flashing |

The 5-pin header, top to bottom: **1 TX · 2 RX · 3 GND · 4 GND · 5 GPIO0** – so a single jumper across pins 4–5
puts the ESP into flashing mode. Neat.

| Board front | Board back |
|---|---|
| ![Board front](docs/images/board-front.jpg) | ![Board back](docs/images/board-back.jpg) |

## 🧪 Build log

| The junior engineer at work | First light! | Ready to go into the pumpkin |
|---|---|---|
| ![Junior engineer soldering](docs/images/build-junior-engineer.jpg) | ![First light on the bench](docs/images/first-light.jpg) | ![Board running from a power bank](docs/images/powerbank.jpg) |

## ⚡ Flashing

You need [PlatformIO](https://platformio.org) (VS Code extension or CLI). Everything else downloads itself.

### First time – over a cable

1. Put the **jumper on header pins 4–5** (GPIO0 → GND).
2. Wire the USB adapter: adapter `TX` → header `RX`, adapter `RX` → header `TX`, `GND` → `GND`.
   (On the CH340 ESP-01 adapter the socket is already crossed – there it's `TX`→`TX`, `RX`→`RX`.
   The diagram has a section on finding the right holes.)

   | CH340 adapter, top | …and bottom (white wire = GPIO0 mod) |
   |---|---|
   | <img src="docs/images/usb-adapter-front.jpg" width="200" alt="CH340 USB adapter top"> | <img src="docs/images/usb-adapter-back.jpg" width="200" alt="CH340 USB adapter bottom"> |
3. **Power the board from 5 V** (power bank / charger on the red-black wires) and plug in the adapter.
4. Flash:

   ```bash
   pio run -e serial -t upload
   ```

5. Remove the jumper and power-cycle the board. 🎉

### Every time after that – over WiFi

```bash
pio run -e ota -t upload                              # default address candle.local
pio run -e ota -t upload --upload-port jack.local     # a pumpkin you renamed
```

Not sure what's running on a pumpkin? The very bottom of *Advanced* shows the firmware version (latest git tag),
commit and build time – also in `/api` as `fw`, `commit` and `built`. A `+3` after the version means three
commits past that tag, `-dirty` means it was built with uncommitted changes.

The OTA password is `candle` – change it in `src/main.cpp` **and** `platformio.ini` before you put your pumpkin
on the porch.

## 📶 First start

1. Power it up. With no WiFi saved, the pumpkin opens a hotspot called **`candle-setup`**.
2. Join it from your phone – the candle page pops up. *Advanced → WiFi → Scan networks*, pick yours, type the password.
3. Back on your home WiFi, open **http://candle.local**. Light it up. 🎃
   (Some Android phones don't do `.local` names – use the pumpkin's IP from your router instead.)

Moved house? *Advanced → WiFi → Scan networks* switches networks from the web UI. If the new network doesn't work
out, the pumpkin falls back to the `…-setup` hotspot, so you can never lock yourself out.

**No WiFi at all?** That's fine. About 20 seconds after it loses (or never finds) its network, the pumpkin opens
its hotspot. Join it and you get the very same web UI at **http://192.168.4.1** – it's a pumpkin, not a cloud service.
Opening the page also hands over your phone's clock, so the schedule works offline too. Every two minutes (only
while no phone is on the hotspot) it peeks for the home WiFi, and once it's back the hotspot switches itself off.

## 🤖 HTTP API (a.k.a. Halloween home automation)

Everything the web UI does is a simple GET on `/api` – handy for Home Assistant, Shortcuts or a cron job that
lights the pumpkin at sunset.

```bash
curl "http://candle.local/api"                              # current state as JSON
curl "http://candle.local/api?on=1"                         # light it
curl "http://candle.local/api?toggle=1"
curl "http://candle.local/api?mode=1&flicker=90&speed=80"   # bonfire mode
curl "http://candle.local/api?color=%2340ff80"              # witchy green
curl "http://candle.local/api?timer=120"                    # off in 2 hours
```

| Parameter | Values |
|---|---|
| `on` / `toggle` | `0`/`1` / any |
| `brightness` | 0–255 |
| `flicker`, `speed` | 0–100 |
| `mode` | `0` candle, `1` fire, `2` steady |
| `color` | `#rrggbb` (URL-encoded `%23rrggbb`) |
| `timer` | minutes, `0` cancels |
| `spookyNow` | any – spooks every lamp in the network |
| `spooky`, `spookyEvery`, `spookyLen`, `all` | `0`/`1`, seconds (30–3600), seconds (5–60), `all=1` = send to every lamp |
| `sched`, `schedOn`, `schedOff` | `0`/`1`, `HH:MM`, `HH:MM` |
| `tz` | POSIX time zone, e.g. `CET-1CEST,M3.5.0,M10.5.0/3` |
| `name`, `hostname` | device name / `hostname`.local (changing the hostname restarts it) |
| `ledCount`, `pin` | strip length (1–60) / data GPIO (2, 4, 5, 12, 13, 14) |

## 👻 Spooky mode

A spook is a ~20 s show in deep red, cut into scenes: a **dying cellar tube** to open, then a random mix of a
**heartbeat** (lub-dub…), **pitch darkness** with a single ember, two **eyes** looking around and blinking, and a slow
**inhale** that ends in a flash. Then it fades back to whatever the pumpkin was doing before – nothing is overwritten.

The trick is that every lamp shows *exactly* the same thing:

- The whole show is a pure function of a **seed** and the **time since start** – no frame-timing randomness – so
  two lamps with the same seed and start are identical frame by frame.
- **Regular spooks need no messages at all.** Each lamp has internet time, so they all start at whole multiples of
  the interval (`:00`, `:01:00`, …) and derive the seed from that slot.
- **"👻 Spooky now!"** sends a UDP broadcast (port `6666`) with a seed and a start time ~0.8 s in the future, so
  the packet reaches everyone before the show starts. *Save for all lamps* broadcasts the settings the same way.

Lamps that are off stay off for the regular spooks; *Spooky now!* lights them all (and puts them back to sleep).
Interval 30–3600 s, length 5–60 s. Without internet time a lamp still spooks on its own button, just not in sync.

```bash
curl "http://candle.local/api?spookyNow=1"                                   # boo!
curl "http://candle.local/api?spooky=1&spookyEvery=60&spookyLen=20&all=1"    # every minute, all lamps
```

## 🕯️ How the flicker works

Each LED chases a random brightness target that is *usually* close to full and *sometimes* drops deep
(`1 − intensity · r³`), with exponential smoothing so it never jumps. 60 % of each LED follows a shared
"common flame", 40 % is its own, which makes the light move around inside the pumpkin instead of blinking.
Every now and then a "gust" dims everything at once. Green and blue fall off faster than red as the flame dims
(`g · v^1.6`, `b · v^2.5`), so dips turn deep orange – and a gamma curve keeps it all looking natural to the eye.

All of it lives in `renderFrame()` in [`src/main.cpp`](src/main.cpp), ~40 lines. Tweak away.

## ⏰ The schedule, in plain words

- **Power on** (plugged in) → lights up with the last settings. No clock needed.
- **Clock synced** (NTP a few seconds after joining WiFi, or your phone's clock when you open the page on its
  hotspot) → it waits for the *next* step: at *On at* it lights,
  at *Off at* it goes out. Overnight ranges like 18:00 → 06:00 work too.
- **Restarts over WiFi** (updates, renaming) keep whatever state it was in.
- You can still switch it manually any time – the schedule just takes over again at the next step.

## 🔐 Security note

This is a cosy LAN gadget, not Fort Knox: the web UI has no login, and the OTA password is a compile-time default.
Change `OTA_PASSWORD` (and `upload_flags`) before deploying, keep it on your home network and never port-forward it
to the internet. Guests on your WiFi *will* be able to make your pumpkin go disco. 🪩 The offline hotspot is open,
too – anyone standing next to the pumpkin can join it.

## 👻 Lessons learned the spooky way

- **"Failed to connect to ESP8266"** for an hour? Check the **power** first. The 3.3 V pin of an Arduino Uno can't
  feed an ESP8266 (it needs 300+ mA peaks). Ours sat there completely silent until we fed the board proper 5 V.
  A **flickering power LED on the regulator** is the tell-tale sign of a brown-out.
- To test a USB-serial adapter, short its `TX` and `RX` and send something – if it comes back, the adapter is fine.
- Pumpkins are wet inside. Put the board in a small zip bag or a jam jar and keep the power bank outside.

## 📁 Project layout

```
├── platformio.ini       # build + upload config (serial & OTA)
├── tools/version.py     # stamps git tag, commit and build time into the firmware
├── src/main.cpp         # flame effect, WiFi, web server, OTA
├── src/page.h           # the web UI (single HTML page)
└── docs/                # wiring diagram, photos, GIF
```

## 📜 License

[MIT](LICENSE) – carve it, fork it, light it. If you build one, open an issue with a photo of your pumpkin! 🎃
