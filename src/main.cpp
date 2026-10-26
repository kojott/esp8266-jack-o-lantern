// WiFi Jack-o'-Lantern – flickering candle on ESP-12F + WS2812B, web UI at http://candle.local
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>
#include <DNSServer.h>
#include <Adafruit_NeoPixel.h>
#include <time.h>
#include <sys/time.h>
#include <coredecls.h>
#include "page.h"

static const char *OTA_PASSWORD = "candle";  // change me (and upload_flags in platformio.ini)
static const uint32_t SETTINGS_MAGIC_V1 = 0x53564B31;  // "SVK1" – before name/hostname existed
static const uint32_t SETTINGS_MAGIC_V2 = 0x53564B32;  // "SVK2" – before the schedule existed
static const uint32_t SETTINGS_MAGIC = 0x53564B33;     // "SVK3"
static const uint8_t MAX_LEDS = 60;

enum Mode : uint8_t { MODE_CANDLE = 0, MODE_FIRE = 1, MODE_STEADY = 2 };

struct Settings {
  uint32_t magic;
  uint8_t on;
  uint8_t brightness;  // 0–255
  uint8_t r, g, b;     // flame colour
  uint8_t flicker;     // 0–100 flicker intensity
  uint8_t speed;       // 0–100 flicker speed
  uint8_t mode;
  uint8_t ledCount;
  uint8_t pin;
  char name[48];      // shown in the page header
  char hostname[25];  // reachable as <hostname>.local
  uint8_t schedEnabled;
  uint16_t schedOn, schedOff;  // minutes after midnight, local time
  char tz[48];                 // POSIX TZ string
};

static Settings cfg;
static const Settings DEFAULTS = {SETTINGS_MAGIC, 1, 180, 255, 110, 20, 60, 50, MODE_CANDLE, 4, 14, "Candle", "candle", 0, 17 * 60, 22 * 60, "CET-1CEST,M3.5.0,M10.5.0/3"};

static Adafruit_NeoPixel strip(4, 14, NEO_GRB + NEO_KHZ800);
static ESP8266WebServer server(80);
static DNSServer dns;

static bool netStarted = false;  // mDNS + OTA, started on the first WiFi connection
static bool apActive = false;    // own hotspot "<hostname>-setup" is running
static bool clockFromPhone = false;
static uint32_t lastConnected = 0, lastRetry = 0;
static bool dirty = false;
static uint32_t dirtyAt = 0;
static uint32_t offAt = 0;      // 0 = no sleep timer
static uint32_t restartAt = 0;  // 0 = no restart scheduled
static String newSsid, newPass;
static int lastMinute = -1;  // last local minute seen by the schedule, -1 = clock not synced yet

struct Flame {
  float level = 1.0f;
  float target = 1.0f;
};
static Flame flames[MAX_LEDS];
static Flame common;
static float gust = 0.0f;
static float master = 0.0f;  // smooth fade in/out

static bool validPin(int p) { return p == 2 || p == 4 || p == 5 || p == 12 || p == 13 || p == 14; }

// Hostname may contain only a–z, 0–9 and dashes (not at the start or end).
static bool sanitizeHostname(String in, char *out) {
  in.trim();
  in.toLowerCase();
  String h;
  for (unsigned i = 0; i < in.length(); i++) {
    char c = in[i];
    if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-') h += c;
    else if (c == ' ' || c == '_' || c == '.') h += '-';
  }
  while (h.startsWith("-")) h.remove(0, 1);
  while (h.endsWith("-")) h.remove(h.length() - 1);
  if (h.length() == 0 || h.length() > 24) return false;
  strcpy(out, h.c_str());
  return true;
}

static String jsonEscape(const char *s) {
  String o;
  for (; *s; s++) {
    if (*s == '"' || *s == '\\') o += '\\';
    if ((uint8_t)*s >= 0x20) o += *s;
  }
  return o;
}

static void scheduleRestart() { restartAt = millis() + 1000; }

static bool validTz(const String &tz) {
  if (tz.length() == 0 || tz.length() >= sizeof(Settings::tz)) return false;
  for (unsigned i = 0; i < tz.length(); i++)
    if (!isalnum(tz[i]) && !strchr(",.+-/:<>", tz[i])) return false;
  return true;
}

static void startClock() { configTime(cfg.tz, "pool.ntp.org", "time.google.com"); }

static bool clockSynced() { return time(nullptr) > 1700000000; }

// "HH:MM" -> minutes after midnight, -1 if invalid.
static int parseHhmm(const String &v) {
  int c = v.indexOf(':');
  if (c < 1) return -1;
  int h = v.substring(0, c).toInt(), m = v.substring(c + 1).toInt();
  return (h >= 0 && h < 24 && m >= 0 && m < 60) ? h * 60 + m : -1;
}

static void applyStripConfig() {
  strip.clear();
  strip.show();
  strip.updateLength(cfg.ledCount);
  strip.setPin(cfg.pin);
  strip.begin();
}

static void loadSettings() {
  EEPROM.begin(sizeof(Settings));
  EEPROM.get(0, cfg);
  // Older layouts are a prefix of the current one – keep what exists, add defaults for the rest.
  if (cfg.magic == SETTINGS_MAGIC_V1) {
    memcpy(cfg.name, DEFAULTS.name, sizeof(cfg.name));
    memcpy(cfg.hostname, DEFAULTS.hostname, sizeof(cfg.hostname));
    cfg.magic = SETTINGS_MAGIC_V2;
  }
  if (cfg.magic == SETTINGS_MAGIC_V2) {
    cfg.schedEnabled = DEFAULTS.schedEnabled;
    cfg.schedOn = DEFAULTS.schedOn;
    cfg.schedOff = DEFAULTS.schedOff;
    memcpy(cfg.tz, DEFAULTS.tz, sizeof(cfg.tz));
    cfg.magic = SETTINGS_MAGIC;
  }
  cfg.tz[sizeof(cfg.tz) - 1] = 0;
  cfg.name[sizeof(cfg.name) - 1] = 0;
  cfg.hostname[sizeof(cfg.hostname) - 1] = 0;
  char host[25];
  if (cfg.magic != SETTINGS_MAGIC || !validPin(cfg.pin) || cfg.ledCount == 0 || cfg.ledCount > MAX_LEDS ||
      !sanitizeHostname(cfg.hostname, host) || strcmp(host, cfg.hostname) != 0 || !validTz(cfg.tz) ||
      cfg.schedOn >= 24 * 60 || cfg.schedOff >= 24 * 60) {
    cfg = DEFAULTS;
  }
}

// Flash survives tens of thousands of erase cycles – only write real changes.
static void saveSettings() {
  dirty = false;
  Settings stored;
  EEPROM.get(0, stored);
  if (memcmp(&stored, &cfg, sizeof(Settings)) == 0) return;
  EEPROM.put(0, cfg);
  EEPROM.commit();
  Serial.println("Settings saved");
}

static void markDirty() {
  dirty = true;
  dirtyAt = millis();
}

static float frand() { return random(0, 10000) / 10000.0f; }

// New flame target: mostly bright, occasionally a deeper dip.
static float newTarget(float intensity) {
  float r = frand();
  return 1.0f - intensity * r * r * r;
}

// Edge-triggered: acts only when the clock reaches the on/off minute, never on boot.
static void checkSchedule() {
  static uint32_t lastCheck = 0;
  if (millis() - lastCheck < 1000 || !clockSynced()) return;
  lastCheck = millis();
  time_t now = time(nullptr);
  struct tm t;
  localtime_r(&now, &t);
  int m = t.tm_hour * 60 + t.tm_min;
  if (m == lastMinute) return;
  bool firstSync = lastMinute < 0;
  lastMinute = m;
  if (firstSync || !cfg.schedEnabled) return;
  if (m == cfg.schedOn && !cfg.on) {
    cfg.on = 1;
    offAt = 0;
    markDirty();
    Serial.println("Schedule: on");
  } else if (m == cfg.schedOff && m != cfg.schedOn && cfg.on) {
    cfg.on = 0;
    offAt = 0;
    markDirty();
    Serial.println("Schedule: off");
  }
}

static void renderFrame() {
  const bool fire = cfg.mode == MODE_FIRE;
  const float intensity = cfg.flicker / 100.0f * (fire ? 1.0f : 0.7f);
  const float spd = cfg.speed / 100.0f;
  const float smoothing = 0.03f + spd * (fire ? 0.35f : 0.22f);
  const int changeChance = 15 + (int)(spd * (fire ? 300 : 160));

  float target = cfg.on ? 1.0f : 0.0f;
  if (offAt && (int32_t)(millis() - offAt) >= 0) {
    cfg.on = 0;
    offAt = 0;
    markDirty();
  }
  master += (target - master) * 0.04f;

  if (cfg.mode != MODE_STEADY) {
    if (random(1000) < changeChance) common.target = newTarget(intensity);
    common.level += (common.target - common.level) * smoothing;

    // Occasional "draught" – all LEDs dip together for a moment.
    if (random(10000) < 3 + (int)(spd * 25) && intensity > 0.05f) gust = 0.25f + 0.5f * intensity * frand();
    gust *= 0.90f;
  } else {
    common.level = 1.0f;
    gust = 0.0f;
  }

  const float bri = cfg.brightness / 255.0f * master;
  for (uint8_t i = 0; i < cfg.ledCount; i++) {
    Flame &f = flames[i];
    float v = 1.0f;
    if (cfg.mode != MODE_STEADY) {
      if (random(1000) < changeChance) f.target = newTarget(intensity);
      f.level += (f.target - f.level) * smoothing;
      v = (0.6f * common.level + 0.4f * f.level) * (1.0f - gust);
    }
    if (v < 0.0f) v = 0.0f;
    // A dimming flame turns redder: green and blue fall off faster than red.
    float rr = cfg.r / 255.0f * v;
    float gg = cfg.g / 255.0f * powf(v, 1.6f);
    float bb = cfg.b / 255.0f * powf(v, 2.5f);
    auto out = [&](float c) { return (uint8_t)(powf(c * bri, 2.2f) * 255.0f + 0.5f); };
    strip.setPixelColor(i, out(rr), out(gg), out(bb));
  }
  strip.show();
}

static String stateJson() {
  const bool online = WiFi.status() == WL_CONNECTED;
  char buf[256];
  int32_t left = offAt ? (int32_t)(offAt - millis()) / 1000 : 0;
  snprintf(buf, sizeof(buf),
           "{\"on\":%d,\"brightness\":%d,\"color\":\"#%02x%02x%02x\",\"flicker\":%d,\"speed\":%d,"
           "\"mode\":%d,\"ledCount\":%d,\"pin\":%d,\"timerLeft\":%ld,\"ip\":\"%s\",\"rssi\":%d,",
           cfg.on, cfg.brightness, cfg.r, cfg.g, cfg.b, cfg.flicker, cfg.speed, cfg.mode, cfg.ledCount, cfg.pin,
           (long)(left > 0 ? left : 0), (online ? WiFi.localIP() : WiFi.softAPIP()).toString().c_str(), WiFi.RSSI());
  String j(buf);
  char clock[6] = "";
  if (clockSynced()) {
    time_t now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);
    snprintf(clock, sizeof(clock), "%02d:%02d", t.tm_hour, t.tm_min);
  }
  j += "\"sched\":" + String(cfg.schedEnabled) + ",\"schedOn\":" + cfg.schedOn + ",\"schedOff\":" + cfg.schedOff +
       ",\"tz\":\"" + cfg.tz + "\",\"time\":\"" + clock + "\",\"clockFromPhone\":" + (clockFromPhone ? "true" : "false") +
       ",\"online\":" + (online ? "true" : "false") + ",\"ap\":" + (apActive ? "true" : "false") + ",";
  j += "\"name\":\"" + jsonEscape(cfg.name) + "\",\"hostname\":\"" + cfg.hostname + "\",\"ssid\":\"" +
       jsonEscape(WiFi.SSID().c_str()) + "\",\"restarting\":" + (restartAt ? "true" : "false") + "}";
  return j;
}

static void handleApi() {
  bool stripChanged = false;
  if (server.hasArg("on")) cfg.on = server.arg("on").toInt() ? 1 : 0;
  if (server.hasArg("toggle")) cfg.on = !cfg.on;
  if (server.hasArg("brightness")) cfg.brightness = constrain(server.arg("brightness").toInt(), 0, 255);
  if (server.hasArg("flicker")) cfg.flicker = constrain(server.arg("flicker").toInt(), 0, 100);
  if (server.hasArg("speed")) cfg.speed = constrain(server.arg("speed").toInt(), 0, 100);
  if (server.hasArg("mode")) cfg.mode = constrain(server.arg("mode").toInt(), 0, 2);
  if (server.hasArg("color")) {
    String c = server.arg("color");
    if (c.startsWith("#")) c.remove(0, 1);
    if (c.length() == 6) {
      uint32_t v = strtoul(c.c_str(), nullptr, 16);
      cfg.r = v >> 16;
      cfg.g = v >> 8;
      cfg.b = v;
    }
  }
  if (server.hasArg("ledCount")) {
    int n = constrain(server.arg("ledCount").toInt(), 1, MAX_LEDS);
    if (n != cfg.ledCount) {
      cfg.ledCount = n;
      stripChanged = true;
    }
  }
  if (server.hasArg("pin")) {
    int p = server.arg("pin").toInt();
    if (validPin(p) && p != cfg.pin) {
      cfg.pin = p;
      stripChanged = true;
    }
  }
  if (server.hasArg("name")) {
    String n = server.arg("name");
    n.trim();
    if (n.length() > 0 && n.length() < sizeof(cfg.name)) strcpy(cfg.name, n.c_str());
  }
  if (server.hasArg("hostname")) {
    char host[25];
    if (sanitizeHostname(server.arg("hostname"), host) && strcmp(host, cfg.hostname) != 0) {
      strcpy(cfg.hostname, host);
      saveSettings();
      scheduleRestart();  // mDNS, OTA and DHCP hostname are applied after a restart
    }
  }
  if (server.hasArg("sched")) cfg.schedEnabled = server.arg("sched").toInt() ? 1 : 0;
  if (server.hasArg("schedOn")) {
    int m = parseHhmm(server.arg("schedOn"));
    if (m >= 0) cfg.schedOn = m;
  }
  if (server.hasArg("schedOff")) {
    int m = parseHhmm(server.arg("schedOff"));
    if (m >= 0) cfg.schedOff = m;
  }
  if (server.hasArg("tz")) {
    String tz = server.arg("tz");
    if (validTz(tz) && tz != cfg.tz) {
      strcpy(cfg.tz, tz.c_str());
      startClock();
      lastMinute = -1;
    }
  }
  // Offline (hotspot) the page hands over the phone's clock; internet time always wins.
  if (server.hasArg("epoch") && !clockSynced()) {
    timeval tv = {(time_t)strtoul(server.arg("epoch").c_str(), nullptr, 10), 0};
    if (tv.tv_sec > 1700000000) settimeofday(&tv, nullptr);
  }
  if (server.hasArg("timer")) {
    int minutes = constrain(server.arg("timer").toInt(), 0, 24 * 60);
    offAt = minutes > 0 ? millis() + minutes * 60000UL : 0;
    if (minutes > 0) cfg.on = 1;
  }
  if (stripChanged) applyStripConfig();
  if (server.args() > 0) markDirty();
  server.send(200, "application/json", stateJson());
}

static void startWebServer() {
  server.on("/", HTTP_GET, [] { server.send_P(200, "text/html; charset=utf-8", PAGE_HTML); });
  server.on("/api", handleApi);
  server.on("/wifireset", HTTP_POST, [] {
    server.send(200, "text/plain; charset=utf-8", "WiFi forgotten, restarting…");
    delay(500);
    WiFi.persistent(true);
    WiFi.disconnect(true);  // erases the stored credentials
    ESP.restart();
  });
  // Nearby WiFi networks – the scan runs asynchronously, the page polls until done.
  server.on("/wifi/scan", HTTP_GET, [] {
    int n = WiFi.scanComplete();
    if (n == WIFI_SCAN_FAILED) WiFi.scanNetworks(true);
    if (n < 0) {
      server.send(200, "application/json", "{\"scanning\":true}");
      return;
    }
    String j = "{\"networks\":[";
    bool first = true;
    for (int i = 0; i < n; i++) {
      String ssid = WiFi.SSID(i);
      if (ssid.length() == 0) continue;
      bool dup = false;  // same SSID from several access points – keep the strongest
      for (int k = 0; k < n && !dup; k++) dup = k != i && WiFi.SSID(k) == ssid && (WiFi.RSSI(k) > WiFi.RSSI(i) || (WiFi.RSSI(k) == WiFi.RSSI(i) && k < i));
      if (dup) continue;
      if (!first) j += ',';
      first = false;
      j += "{\"ssid\":\"" + jsonEscape(ssid.c_str()) + "\",\"rssi\":" + WiFi.RSSI(i) +
           ",\"open\":" + (WiFi.encryptionType(i) == ENC_TYPE_NONE ? "true" : "false") + "}";
    }
    j += "]}";
    WiFi.scanDelete();
    server.send(200, "application/json", j);
  });
  server.on("/wifi/connect", HTTP_POST, [] {
    newSsid = server.arg("ssid");
    newPass = server.arg("pass");
    if (newSsid.length() == 0 || newSsid.length() > 32 || newPass.length() > 64) {
      server.send(400, "text/plain; charset=utf-8", "Invalid network or password");
      return;
    }
    server.send(200, "text/plain; charset=utf-8", "OK");
    scheduleRestart();
  });
  // On the hotspot every unknown URL (phones probing for internet) leads to the candle page.
  server.onNotFound([] {
    if (apActive) {
      server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/");
      server.send(302, "text/plain", "");
    } else {
      server.send(404, "text/plain", "404");
    }
  });
  server.begin();
}

static void startNetworkServices() {
  MDNS.begin(cfg.hostname);
  MDNS.addService("http", "tcp", 80);

  ArduinoOTA.setHostname(cfg.hostname);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.onStart([] {
    strip.clear();
    strip.show();
  });
  ArduinoOTA.begin();

  startClock();  // again, in case the boot-time NTP request went nowhere

  netStarted = true;
  Serial.printf("Connected: http://%s.local  (%s)\n", cfg.hostname, WiFi.localIP().toString().c_str());
}

static void startAp() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP((String(cfg.hostname) + "-setup").c_str());
  dns.start(53, "*", WiFi.softAPIP());
  apActive = true;
  // A station hunting for a missing router hops channels and kicks phones off the hotspot – stop it, retry later.
  WiFi.setAutoReconnect(false);
  wifi_station_disconnect();
  lastRetry = millis();
  Serial.printf("Hotspot %s-setup, http://%s\n", cfg.hostname, WiFi.softAPIP().toString().c_str());
}

static void stopAp() {
  dns.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  apActive = false;
  Serial.println("Hotspot off");
}

// Home WiFi when it's there, own hotspot when it isn't – the candle is controllable either way.
static void manageWifi() {
  static uint32_t lastCheck = 0;
  if (millis() - lastCheck < 1000) return;
  lastCheck = millis();
  const bool hasCreds = WiFi.SSID().length() > 0;
  const int phones = apActive ? WiFi.softAPgetStationNum() : 0;
  if (WiFi.status() == WL_CONNECTED) {
    lastConnected = millis();
    if (!netStarted) startNetworkServices();
    if (apActive && phones == 0) stopAp();
    return;
  }
  if (!apActive) {
    if (!hasCreds || millis() - lastConnected > 20000) startAp();
    return;
  }
  // On the hotspot: every 2 minutes try the home WiFi for 15 s, but only while no phone is using the hotspot.
  if (!hasCreds) return;
  const uint32_t since = millis() - lastRetry;
  if (phones == 0 && since > 120000) {
    lastRetry = millis();
    WiFi.begin();
  } else if (since > 15000 && WiFi.status() != WL_IDLE_STATUS) {
    wifi_station_disconnect();
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nJack-o'-Lantern starting");
  randomSeed(ESP.getChipId() ^ micros());

  loadSettings();
  // Plugged into the socket = light it with the last settings. Software restarts (OTA, rename) keep the state.
  rst_info *reset = ESP.getResetInfoPtr();
  if (reset->reason == REASON_DEFAULT_RST || reset->reason == REASON_EXT_SYS_RST) cfg.on = 1;
  applyStripConfig();

  settimeofday_cb([](bool fromSntp) { clockFromPhone = !fromSntp; });
  startClock();

  WiFi.persistent(true);
  WiFi.mode(WIFI_STA);
  WiFi.hostname(cfg.hostname);
  if (WiFi.SSID().length()) WiFi.begin();  // saved network; without one (or unreachable) manageWifi() opens the hotspot
  startWebServer();
}

void loop() {
  manageWifi();
  if (apActive) dns.processNextRequest();
  server.handleClient();
  if (netStarted) {
    MDNS.update();
    ArduinoOTA.handle();
  }

  checkSchedule();

  static uint32_t lastFrame = 0;
  if (millis() - lastFrame >= 16) {
    lastFrame = millis();
    renderFrame();
  }

  if (dirty && millis() - dirtyAt > 3000) saveSettings();

  if (restartAt && (int32_t)(millis() - restartAt) >= 0) {
    if (dirty) saveSettings();
    if (newSsid.length()) {
      WiFi.persistent(true);
      WiFi.begin(newSsid.c_str(), newPass.c_str());  // stores the new credentials
      delay(200);
    }
    ESP.restart();
  }
}
