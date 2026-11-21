#pragma once
// Shared scenario command interpreter and synthetic guitar tone generator for both the native
// simulator (simulator/main.cpp) and WebAssembly (simulator/web/vex_web.cpp).
// Uses a small C tokenizer and placement-new storage so WebAssembly stays free of iostreams,
// exceptions and per-command heap allocations.
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <new>
#include "../firmware/VEX_Pocket_Rival/app.h"
#include "../firmware/VEX_Pocket_Rival/analyzer.h"
#include "state.h"

namespace vexsim {

enum CmdResult : int {
 CMD_NOP = 0,
 CMD_OK = 1,
 CMD_BAD_ARG = 2,
 CMD_UNKNOWN = 3,
 CMD_IO_ERR = 4
};

struct Tok {
 const char* p;
 explicit Tok(const char* line) : p(line ? line : "") {}
 void skipWs() { while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') ++p; }
 bool empty() { skipWs(); return *p == '\0' || *p == '#'; }
 bool word(char* out, size_t cap) {
  skipWs();
  if (*p == '\0' || *p == '#' || cap == 0) return false;
  size_t n = 0;
  while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') {
   if (n + 1 < cap) out[n++] = *p;
   ++p;
  }
  out[n] = '\0';
  return n > 0;
 }
 bool u32(uint32_t& out) {
  skipWs();
  if (*p == '\0' || *p == '-' || *p == '#') return false;
  char* end = nullptr;
  unsigned long v = strtoul(p, &end, 10);
  if (end == p) return false;
  p = end;
  out = uint32_t(v);
  return true;
 }
 bool i32(long& out) {
  skipWs();
  if (*p == '\0' || *p == '#') return false;
  char* end = nullptr;
  long v = strtol(p, &end, 10);
  if (end == p) return false;
  p = end;
  out = v;
  return true;
 }
 bool f32(float& out) {
  skipWs();
  if (*p == '\0' || *p == '#') return false;
  char* end = nullptr;
  float v = strtof(p, &end);
  if (end == p || !isfinite(v)) return false;
  p = end;
  out = v;
  return true;
 }
 bool f64(double& out) {
  skipWs();
  if (*p == '\0' || *p == '#') return false;
  char* end = nullptr;
  double v = strtod(p, &end);
  if (end == p || !isfinite(v)) return false;
  p = end;
  out = v;
  return true;
 }
};

struct SimStore : vex::Store {
 static constexpr int CAPACITY = 128;
 uint8_t bytes[CAPACITY]{};
 long cut = -1;
 bool powered = true, dirty = false;
 uint8_t read(int a) override { return a >= 0 && a < CAPACITY ? bytes[a] : 0xff; }
 void update(int a, uint8_t v) override {
  if (!powered || a < 0 || a >= CAPACITY) return;
  if (cut == 0) { powered = false; return; }
  if (cut > 0) cut--;
  if (bytes[a] != v) { bytes[a] = v; dirty = true; }
 }
 virtual void flush() {}
 void wipe() {
  memset(bytes, 0, sizeof bytes);
  dirty = true;
  cut = -1;
  powered = true;
  flush();
 }
};

struct SimHost {
 SimStore& store;
 int stage = 4;
 alignas(8) uint8_t appStorage[sizeof(vex::App)]{};
 vex::App* appPtr = nullptr;
 vex::Analyzer analyzer{};
 bool raw_mode = false, raw_quest = false, raw_foot = false, test = false;
 bool inject = false;
 float rms = 0, peak = 0, hz = 0, probability = 0;
 double sample_ms = 0;
 uint32_t analyses = 0;

 explicit SimHost(SimStore& s) : store(s) { boot(0); }
 ~SimHost() { if (appPtr) appPtr->~App(); }

 vex::App& app() { return *appPtr; }
 const vex::App& app() const { return *appPtr; }

 StateSource stateSource() const {
  return {app(), analyzer, store.powered, analyses, test};
 }

 void boot(uint32_t now = 0) {
  store.powered = true;
  store.cut = -1;
  if (appPtr) appPtr->~App();
  appPtr = new (appStorage) vex::App(store, stage);
  appPtr->begin(now, true);
  analyzer = vex::Analyzer();
  raw_mode = raw_quest = raw_foot = false;
  inject = false;
  sample_ms = now;
 }

 void reset() {
  store.wipe();
  test = false;
  stage = 4;
  analyses = 0;
  boot(0);
 }

 void poll() {
  float r, p;
  if (analyzer.takeLevel(r, p)) app().observe(r, p, app().now);
  if (app().wantsPitch()) {
   if (analyzer.analyze()) { analyses++; app().observePitch(analyzer.hz, analyzer.probability, app().now); }
  } else {
   analyzer.skip();
  }
 }

 void step() {
  uint32_t now = app().now + 1;
  if (inject) {
   app().observe(rms, peak, now);
   if (hz > 0) app().observePitch(hz, probability, now);
  }
  app().controls(raw_mode, raw_quest, raw_foot, now);
  app().tick(now);
 }

 void run(uint32_t ms) { for (uint32_t i = 0; i < ms; i++) step(); }

 void beginFeed() { sample_ms = app().now; }

 void feedBlock(const int16_t* block) {
  analyzer.push(block, vex::Analyzer::BLOCK);
  sample_ms += 1000.0 * vex::Analyzer::BLOCK / vex::Analyzer::RATE;
  poll();
  while (app().now + 1 <= uint32_t(sample_ms)) step();
 }

 void feedSamples(const int16_t* pcm, size_t count) {
  for (size_t i = 0; i < count; i += vex::Analyzer::BLOCK) {
   int16_t block[vex::Analyzer::BLOCK]{};
   size_t n = count - i < size_t(vex::Analyzer::BLOCK) ? count - i : size_t(vex::Analyzer::BLOCK);
   memcpy(block, pcm + i, n * sizeof(int16_t));
   feedBlock(block);
  }
 }

 void feed(const int16_t* pcm, size_t count) {
  beginFeed();
  feedSamples(pcm, count);
 }

 // Streamed harmonic guitar pluck / gapped strum generator (0 heap bytes).
 void playTone(uint32_t ms, float freq, float dbfs, float decay_ms, float gap_ms) {
  constexpr double PI = 3.14159265358979323846;
  size_t n = size_t(uint64_t(ms) * vex::Analyzer::RATE / 1000);
  beginFeed();
  if (!n) return;
  if (freq <= 0) {
   int16_t silent[vex::Analyzer::BLOCK]{};
   for (size_t i = 0; i < n; i += vex::Analyzer::BLOCK) feedBlock(silent);
   return;
  }
  float amp = powf(10.f, dbfs / 20.f);
  float ring_ms = decay_ms > 0 ? decay_ms : (gap_ms > 0 ? 200.f : 0.f);
  float period_ms = gap_ms > 0 ? ring_ms + gap_ms : 0.f;
  double phase = 0, stepRad = 2.0 * PI * freq / vex::Analyzer::RATE;
  for (size_t i = 0; i < n; i += vex::Analyzer::BLOCK) {
   int16_t block[vex::Analyzer::BLOCK]{};
   size_t chunk = n - i < size_t(vex::Analyzer::BLOCK) ? n - i : size_t(vex::Analyzer::BLOCK);
   for (size_t k = 0; k < chunk; k++) {
    float t_ms = float(i + k) * 1000.f / vex::Analyzer::RATE;
    float in_ms = period_ms > 0 ? fmodf(t_ms, period_ms) : t_ms;
    if (gap_ms > 0 && in_ms >= ring_ms) { phase = 0; continue; }
    float env = decay_ms > 0 ? expf(-(gap_ms > 0 ? 1.5f : 1.f) * in_ms / decay_ms) : 1.f;
    phase += stepRad;
    if (phase > 2.0 * PI) phase -= 2.0 * PI;
    float v = amp * env * (sinf(float(phase)) + .45f * sinf(float(2 * phase)) + .15f * sinf(float(3 * phase))) / 1.6f;
    if (v > 1.f) v = 1.f;
    else if (v < -1.f) v = -1.f;
    block[k] = int16_t(lroundf(v * 32767.f));
   }
   feedBlock(block);
  }
 }

 bool press(const char* name, uint32_t ms) {
  bool* ctl = nullptr;
  if (strcmp(name, "mode") == 0) ctl = &raw_mode;
  else if (strcmp(name, "quest") == 0) ctl = &raw_quest;
  else if (strcmp(name, "foot") == 0) ctl = &raw_foot;
  else return false;
  *ctl = true;
  run(ms);
  *ctl = false;
  run(30);
  return true;
 }

 void audio(uint32_t ms, float r, float p, float f, float q) {
  inject = true; rms = r; peak = p; hz = f; probability = q;
  run(ms);
  inject = false;
 }

 // Optional host callback for `wav` and `raw` file commands on native builds.
 using FileFeeder = bool (*)(SimHost& host, const char* kind, const char* path, double start_ms, double len_ms, void* ctx);

 CmdResult exec(const char* line, FileFeeder fileFeeder = nullptr, void* fileCtx = nullptr) {
  Tok t(line);
  if (t.empty()) return CMD_NOP;
  char cmd[32];
  if (!t.word(cmd, sizeof cmd)) return CMD_NOP;

  if (strcmp(cmd, "press") == 0) {
   char name[16];
   if (!t.word(name, sizeof name)) return CMD_UNKNOWN;
   uint32_t ms = 100;
   t.u32(ms);
   if (ms > 60000 || !press(name, ms)) return CMD_UNKNOWN;
   return CMD_OK;
  }
  if (strcmp(cmd, "audio") == 0) {
   uint32_t ms = 0; float r = 0, p = 0, f = 0, q = 1;
   if (!t.u32(ms) || !t.f32(r) || !t.f32(p) || ms > 3600000) return CMD_BAD_ARG;
   if (t.f32(f)) t.f32(q);
   audio(ms, r, p, f, q);
   return CMD_OK;
  }
  if (strcmp(cmd, "wait") == 0) {
   uint32_t ms = 0;
   if (!t.u32(ms) || ms > 3600000) return CMD_BAD_ARG;
   run(ms);
   return CMD_OK;
  }
  if (strcmp(cmd, "wav") == 0 || strcmp(cmd, "raw") == 0) {
   char path[512];
   if (!t.word(path, sizeof path) || !fileFeeder) return CMD_IO_ERR;
   double start = 0, length = -1;
   if (t.f64(start)) t.f64(length);
   return fileFeeder(*this, cmd, path, start, length, fileCtx) ? CMD_OK : CMD_IO_ERR;
  }
  if (strcmp(cmd, "tone") == 0) {
   uint32_t ms = 0; float freq = 0, dbfs = -20, decay = 0, gap = 0;
   if (!t.u32(ms) || !t.f32(freq) || ms > 3600000) return CMD_BAD_ARG;
   if (t.f32(dbfs)) { if (t.f32(decay)) t.f32(gap); }
   if (decay < 0 || gap < 0) return CMD_BAD_ARG;
   playTone(ms, freq, dbfs, decay, gap);
   return CMD_OK;
  }
  if (strcmp(cmd, "serial") == 0) {
   char arg[8];
   if (t.word(arg, sizeof arg)) app().command(arg[0], app().now);
   return CMD_OK;
  }
  if (strcmp(cmd, "save") == 0) { app().command('s', app().now); return CMD_OK; }
  if (strcmp(cmd, "calibrate") == 0) {
   float r = .001f;
   t.f32(r);
   app().command('c', app().now);
   audio(3100, r, r * 2, 0, 0);
   return CMD_OK;
  }
  if (strcmp(cmd, "stage") == 0) {
   long s = 4;
   if (!t.i32(s) || s < 2 || s > 4) return CMD_BAD_ARG;
   stage = int(s);
   boot(0);
   return CMD_OK;
  }
  if (strcmp(cmd, "powercut") == 0) {
   long n = 0;
   if (!t.i32(n)) return CMD_BAD_ARG;
   store.cut = n;
   return CMD_OK;
  }
  if (strcmp(cmd, "reboot") == 0) { store.flush(); boot(0); return CMD_OK; }
  if (strcmp(cmd, "corrupt") == 0) {
   if (app().saved_slot >= 0) {
    store.powered = true;
    store.bytes[64 * app().saved_slot + 40] ^= 1;
    store.dirty = true;
   }
   return CMD_OK;
  }
  if (strcmp(cmd, "reset") == 0) { reset(); return CMD_OK; }
  if (strcmp(cmd, "test") == 0) {
   char v[8];
   if (!t.word(v, sizeof v) || (strcmp(v, "on") != 0 && strcmp(v, "off") != 0)) return CMD_BAD_ARG;
   test = strcmp(v, "on") == 0;
   return CMD_OK;
  }
  if (strcmp(cmd, "state") == 0) return CMD_OK;
  return CMD_UNKNOWN;
 }
};

} // namespace vexsim
