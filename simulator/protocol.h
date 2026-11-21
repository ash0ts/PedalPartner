#pragma once
// Shared scenario command interpreter and synthetic guitar performance generator for both the native
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

// Parse note token: `E2`, `F#3`, `Bb3`, `A4`, `~G3` (legato), `A3@+5` (+5 dB accent), `R`/`-` (rest),
// `m60` (MIDI), or raw frequency in Hz (`440`, `82.41`). Also optional `+15c`/`-10c` detune suffix.
inline bool parseNoteSpec(const char* raw, float& hz, float& dbDelta, bool& isRest, bool& isLegato) {
 hz = 0; dbDelta = 0; isRest = false; isLegato = false;
 if (!raw || !*raw) return false;
 char buf[32];
 strncpy(buf, raw, sizeof(buf) - 1);
 buf[sizeof(buf) - 1] = '\0';
 char* s = buf;
 if (*s == '~') { isLegato = true; ++s; }
 char* at = strchr(s, '@');
 if (at) {
  *at = '\0';
  char* end = nullptr;
  float d = strtof(at + 1, &end);
  if (end == at + 1 || !isfinite(d)) return false;
  dbDelta = d;
 }
 float detuneCents = 0;
 char* ct = strchr(s, ':');
 if (ct) {
  *ct = '\0';
  char* end = nullptr;
  float c = strtof(ct + 1, &end);
  if (end == ct + 1 || !isfinite(c)) return false;
  detuneCents = c;
 }
 if (strcmp(s, "R") == 0 || strcmp(s, "r") == 0 || strcmp(s, "-") == 0) {
  isRest = true;
  return true;
 }
 if ((s[0] >= 'A' && s[0] <= 'G') || (s[0] >= 'a' && s[0] <= 'g')) {
  static constexpr int SEMI[7] = {9, 11, 0, 2, 4, 5, 7}; // A B C D E F G
  char up = (s[0] >= 'a') ? char(s[0] - 'a' + 'A') : s[0];
  int semi = SEMI[up - 'A'];
  int idx = 1;
  if (s[idx] == '#') { semi++; idx++; }
  else if (s[idx] == 'b') { semi--; idx++; }
  if (!s[idx]) return false;
  char* end = nullptr;
  long oct = strtol(s + idx, &end, 10);
  if (end == s + idx || *end != '\0') return false;
  int midi = int((oct + 1) * 12 + semi);
  hz = 440.f * powf(2.f, (float(midi - 69) + detuneCents / 100.f) / 12.f);
  return hz > 0;
 }
 if (s[0] == 'm' && s[1] >= '0' && s[1] <= '9') {
  char* end = nullptr;
  long midi = strtol(s + 1, &end, 10);
  if (end == s + 1 || *end != '\0') return false;
  hz = 440.f * powf(2.f, (float(midi - 69) + detuneCents / 100.f) / 12.f);
  return hz > 0;
 }
 char* end = nullptr;
 float f = strtof(s, &end);
 if (end == s || *end != '\0' || !isfinite(f) || f < 0) return false;
 if (f == 0.f) { isRest = true; return true; }
 hz = f * powf(2.f, detuneCents / 1200.f);
 return true;
}

inline bool parseTempoStepMs(const char* tok, float& stepMs) {
 char* end = nullptr;
 float bpm = strtof(tok, &end);
 if (end == tok || !isfinite(bpm) || bpm <= 0.f) return false;
 float subdiv = 1.f;
 if (*end == 'x' || *end == '*') {
  char* e2 = nullptr;
  subdiv = strtof(end + 1, &e2);
  if (e2 == end + 1 || *e2 != '\0' || subdiv <= 0.f) return false;
 } else if (*end != '\0') {
  return false;
 }
 stepMs = 60000.f / (bpm * subdiv);
 return stepMs >= 45.f && stepMs <= 10000.f;
}

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
 static constexpr uint32_t DEFAULT_SEED = 0x56455831u;
 SimStore& store;
 int stage = 4;
 alignas(8) uint8_t appStorage[sizeof(vex::App)]{};
 vex::App* appPtr = nullptr;
 vex::Analyzer analyzer{};
 bool raw_mode = false, raw_quest = false, raw_foot = false, test = false;
 bool inject = false, emit_events = false;
 float rms = 0, peak = 0, hz = 0, probability = 0;
 double sample_ms = 0;
 uint32_t analyses = 0;
 uint32_t rng = DEFAULT_SEED;

 explicit SimHost(SimStore& s) : store(s) { boot(0); }
 ~SimHost() { if (appPtr) appPtr->~App(); }

 vex::App& app() { return *appPtr; }
 const vex::App& app() const { return *appPtr; }

 uint32_t rngU32() {
  uint32_t x = rng ? rng : DEFAULT_SEED;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  rng = x;
  return x;
 }
 // Triangular symmetric value in [-1, +1] with zero mean.
 float rngTri() {
  float a = float(rngU32() & 0xffffu) * (1.f / 65535.f);
  float b = float(rngU32() & 0xffffu) * (1.f / 65535.f);
  return a + b - 1.f;
 }

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
  emit_events = false;
  sample_ms = now;
  rng = DEFAULT_SEED;
 }

 void reset() {
  store.wipe();
  test = false;
  stage = 4;
  analyses = 0;
  boot(0);
 }

 void poll() {
  analyzer.setGate(app().progress.gate);
  float r, p;
  uint32_t sc = 0;
  if (analyzer.takeLevel(r, p, &sc)) app().observe(r, p, app().now, sc);
  vex::Onset ons;
  while (analyzer.takeOnset(ons)) app().observeOnset(ons, app().now);
  vex::PitchMode pm = app().pitchMode();
  if (pm != vex::PITCH_OFF) {
   if (analyzer.analyze(pm, app().slow_run_hop)) {
    analyses++;
    app().observePitch(analyzer.hz, analyzer.probability, app().now, analyzer.pitch_sample,
                       vex::pitchProfile(pm, app().slow_run_hop).window);
   }
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

 static inline int16_t pluckSample(double phase, float amp) {
  float v = amp * (sinf(float(phase)) + .45f * sinf(float(2 * phase)) + .15f * sinf(float(3 * phase))) * (1.f / 1.6f);
  if (v > 1.f) v = 1.f;
  else if (v < -1.f) v = -1.f;
  return int16_t(lroundf(v * 32767.f));
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
    block[k] = pluckSample(phase, amp * env);
   }
   feedBlock(block);
  }
 }

 // Multi-note synthetic guitar phrase with fixed-seed timing jitter, drift, rests, accents and legato.
 struct SynthNote { float start_ms, hz, amp; bool rest, legato; };
 void playNotes(float step_ms, float jitter_ms, float drift_ms, float base_dbfs, const SynthNote* rawNotes, int count) {
  constexpr double PI = 3.14159265358979323846;
  if (count <= 0) return;
  SynthNote notes[64];
  if (count > 64) count = 64;
  float lead_ms = 50.f, prev_ms = 0.f;
  for (int i = 0; i < count; i++) {
   notes[i] = rawNotes[i];
   float jit = jitter_ms > 0.f ? rngTri() * jitter_ms : 0.f;
   float t = lead_ms + float(i) * (step_ms + drift_ms) + jit;
   if (i > 0 && t < prev_ms + 45.f) t = prev_ms + 45.f;
   if (t < 10.f) t = 10.f;
   notes[i].start_ms = t;
   notes[i].amp = powf(10.f, (base_dbfs + rawNotes[i].amp) / 20.f);
   prev_ms = t;
  }
  float tail_ms = fmaxf(step_ms * 1.05f, 260.f);
  float total_ms = notes[count - 1].start_ms + tail_ms;
  size_t total_samples = size_t(double(total_ms) * vex::Analyzer::RATE / 1000.0);
  float tau = fminf(400.f, fmaxf(150.f, step_ms * .82f));
  beginFeed();
  int cur = -1;
  double phase = 0;
  float pluck_origin_ms = 0, cur_amp = 0, cur_hz = 0;
  for (size_t i = 0; i < total_samples; i += vex::Analyzer::BLOCK) {
   int16_t block[vex::Analyzer::BLOCK]{};
   size_t chunk = total_samples - i < size_t(vex::Analyzer::BLOCK) ? total_samples - i : size_t(vex::Analyzer::BLOCK);
   for (size_t k = 0; k < chunk; k++) {
    float t_ms = float(i + k) * (1000.f / vex::Analyzer::RATE);
    while (cur + 1 < count && t_ms >= notes[cur + 1].start_ms) {
     cur++;
     if (notes[cur].rest) {
      cur_hz = 0;
     } else if (notes[cur].legato && cur_hz > 0) {
      float cur_env = expf(-(t_ms - pluck_origin_ms) / tau);
      cur_hz = notes[cur].hz;
      // Hammer-on keeps continuous phase and current decayed amplitude (no pick energy jump).
      cur_amp = fmaxf(cur_amp * cur_env, notes[cur].amp * .28f);
      pluck_origin_ms = t_ms;
     } else {
      cur_hz = notes[cur].hz;
      cur_amp = notes[cur].amp;
      pluck_origin_ms = t_ms;
      phase = 0;
     }
    }
    if (cur < 0 || notes[cur].rest || cur_hz <= 0.f) continue;
    float in_ms = t_ms - pluck_origin_ms;
    float atk = (!notes[cur].legato && (t_ms - notes[cur].start_ms) < 2.5f) ? (t_ms - notes[cur].start_ms) / 2.5f : 1.f;
    float env = atk * expf(-in_ms / tau);
    phase += 2.0 * PI * double(cur_hz) / vex::Analyzer::RATE;
    if (phase > 2.0 * PI) phase -= 2.0 * PI;
    block[k] = pluckSample(phase, cur_amp * env);
   }
   feedBlock(block);
  }
 }

 // Smooth raised-cosine guitar string bend from `base_hz` to `base_hz * 2^(target_cents / 1200)`.
 void playBend(uint32_t ms, float base_hz, float target_cents, float hold_ms, float dbfs) {
  constexpr double PI = 3.14159265358979323846;
  size_t total = size_t(uint64_t(ms) * vex::Analyzer::RATE / 1000);
  beginFeed();
  if (!total || base_hz <= 0.f) return;
  float lead_ms = 40.f;
  float base_ms = fminf(220.f, float(ms) * .26f);
  if (hold_ms <= 0.f) hold_ms = fminf(340.f, float(ms) * .38f);
  float ramp_ms = float(ms) - lead_ms - base_ms - hold_ms;
  if (ramp_ms < 40.f) ramp_ms = 40.f;
  float amp = powf(10.f, dbfs / 20.f);
  float tau = fmaxf(900.f, float(ms) * 1.4f);
  double phase = 0;
  for (size_t i = 0; i < total; i += vex::Analyzer::BLOCK) {
   int16_t block[vex::Analyzer::BLOCK]{};
   size_t chunk = total - i < size_t(vex::Analyzer::BLOCK) ? total - i : size_t(vex::Analyzer::BLOCK);
   for (size_t k = 0; k < chunk; k++) {
    float t_ms = float(i + k) * (1000.f / vex::Analyzer::RATE);
    if (t_ms < lead_ms) continue;
    float nt = t_ms - lead_ms;
    float cents = 0.f;
    if (nt > base_ms) {
     float u = (nt - base_ms) / ramp_ms;
     if (u >= 1.f) cents = target_cents;
     else cents = target_cents * .5f * (1.f - cosf(float(PI) * u));
    }
    float f = base_hz * powf(2.f, cents / 1200.f);
    float atk = nt < 2.5f ? nt / 2.5f : 1.f;
    float env = atk * expf(-nt / tau);
    phase += 2.0 * PI * double(f) / vex::Analyzer::RATE;
    if (phase > 2.0 * PI) phase -= 2.0 * PI;
    block[k] = pluckSample(phase, amp * env);
   }
   feedBlock(block);
  }
 }

 // Held note with sinusoidal vibrato (`width_cents` = peak-to-peak pitch swing in cents).
 void playVib(uint32_t ms, float base_hz, float rate_hz, float width_cents, float dbfs) {
  constexpr double PI = 3.14159265358979323846;
  size_t total = size_t(uint64_t(ms) * vex::Analyzer::RATE / 1000);
  beginFeed();
  if (!total || base_hz <= 0.f) return;
  float lead_ms = 40.f, settle_ms = 140.f;
  float amp = powf(10.f, dbfs / 20.f);
  float tau = fmaxf(1200.f, float(ms) * 1.5f);
  double phase = 0;
  for (size_t i = 0; i < total; i += vex::Analyzer::BLOCK) {
   int16_t block[vex::Analyzer::BLOCK]{};
   size_t chunk = total - i < size_t(vex::Analyzer::BLOCK) ? total - i : size_t(vex::Analyzer::BLOCK);
   for (size_t k = 0; k < chunk; k++) {
    float t_ms = float(i + k) * (1000.f / vex::Analyzer::RATE);
    if (t_ms < lead_ms) continue;
    float nt = t_ms - lead_ms;
    float cents = 0.f;
    if (nt > settle_ms && rate_hz > 0.f) {
     float vt_s = (nt - settle_ms) * .001f;
     float fade = fminf(1.f, vt_s * 12.f);
     cents = fade * (.5f * width_cents) * sinf(float(2.0 * PI) * rate_hz * vt_s);
    }
    float f = base_hz * powf(2.f, cents / 1200.f);
    float atk = nt < 2.5f ? nt / 2.5f : 1.f;
    float env = atk * expf(-nt / tau);
    phase += 2.0 * PI * double(f) / vex::Analyzer::RATE;
    if (phase > 2.0 * PI) phase -= 2.0 * PI;
    block[k] = pluckSample(phase, amp * env);
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

 using FileFeeder = bool (*)(SimHost& host, const char* kind, const char* path, double start_ms, double len_ms, void* ctx);

 CmdResult exec(const char* line, FileFeeder fileFeeder = nullptr, void* fileCtx = nullptr) {
  emit_events = false;
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
  if (strcmp(cmd, "notes") == 0) {
   char tempoTok[32];
   float step_ms = 0, jitter_ms = 0, drift_ms = 0, dbfs = -18;
   if (!t.word(tempoTok, sizeof tempoTok) || !parseTempoStepMs(tempoTok, step_ms)) return CMD_BAD_ARG;
   if (!t.f32(jitter_ms) || !t.f32(drift_ms) || !t.f32(dbfs) || jitter_ms < 0) return CMD_BAD_ARG;
   SynthNote specs[64]{};
   int n = 0;
   char ntok[32];
   while (n < 64 && t.word(ntok, sizeof ntok)) {
    float nhz = 0, dbd = 0; bool rest = false, leg = false;
    if (!parseNoteSpec(ntok, nhz, dbd, rest, leg)) return CMD_BAD_ARG;
    specs[n++] = {0.f, nhz, dbd, rest, leg};
   }
   if (!n) return CMD_BAD_ARG;
   playNotes(step_ms, jitter_ms, drift_ms, dbfs, specs, n);
   return CMD_OK;
  }
  if (strcmp(cmd, "bend") == 0) {
   uint32_t ms = 0; char ntok[32]; float target = 0, hold = 0, dbfs = -18;
   if (!t.u32(ms) || !t.word(ntok, sizeof ntok) || !t.f32(target) || ms > 60000) return CMD_BAD_ARG;
   float nhz = 0, dbd = 0; bool rest = false, leg = false;
   if (!parseNoteSpec(ntok, nhz, dbd, rest, leg) || rest) return CMD_BAD_ARG;
   if (t.f32(hold)) t.f32(dbfs);
   playBend(ms, nhz, target, hold, dbfs + dbd);
   return CMD_OK;
  }
  if (strcmp(cmd, "vib") == 0) {
   uint32_t ms = 0; char ntok[32]; float rate = 0, width = 0, dbfs = -18;
   if (!t.u32(ms) || !t.word(ntok, sizeof ntok) || !t.f32(rate) || !t.f32(width) || ms > 60000 || rate < 0 || width < 0) return CMD_BAD_ARG;
   float nhz = 0, dbd = 0; bool rest = false, leg = false;
   if (!parseNoteSpec(ntok, nhz, dbd, rest, leg) || rest) return CMD_BAD_ARG;
   t.f32(dbfs);
   playVib(ms, nhz, rate, width, dbfs + dbd);
   return CMD_OK;
  }
  if (strcmp(cmd, "events") == 0) {
   char sub[16];
   if (t.word(sub, sizeof sub)) {
    if (strcmp(sub, "clear") != 0) return CMD_BAD_ARG;
    app().notes.clear();
   }
   emit_events = true;
   return CMD_OK;
  }
  if (strcmp(cmd, "pitch") == 0) {
   char m[16];
   if (!t.word(m, sizeof m)) return CMD_BAD_ARG;
   if (strcmp(m, "off") == 0) app().pitch_override = vex::PITCH_OFF;
   else if (strcmp(m, "tuner") == 0) app().pitch_override = vex::PITCH_TUNER;
   else if (strcmp(m, "fast") == 0) app().pitch_override = vex::PITCH_FAST;
   else if (strcmp(m, "run") == 0) app().pitch_override = vex::PITCH_RUN;
   else return CMD_BAD_ARG;
   return CMD_OK;
  }
  if (strcmp(cmd, "seed") == 0) {
   uint32_t s = 0;
   if (!t.u32(s)) return CMD_BAD_ARG;
   rng = s ? s : DEFAULT_SEED;
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
