// Desktop host for the shared firmware state machine. Reads one command per line via simulator/protocol.h,
// advances a millisecond clock through vex::App, writes the frame and prints JSON from simulator/state.h.
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include "canvas.h"
#include "wav.h"
#include "../firmware/VEX_Pocket_Rival/app.h"
#include "protocol.h"

struct FileStore : vexsim::SimStore {
 std::string path;
 explicit FileStore(std::string p) : path(std::move(p)) {
  std::ifstream in(path, std::ios::binary);
  if (in) in.read((char*)bytes, sizeof bytes);
 }
 void flush() override {
  if (!dirty) return;
  std::ofstream out(path, std::ios::binary);
  out.write((const char*)bytes, sizeof bytes);
  dirty = false;
 }
};

static bool loadAudioFile(vexsim::SimHost& host, const char* kind, const char* path, double start, double length, void* ctx) {
 auto* errOut = static_cast<std::string*>(ctx);
 std::vector<int16_t> pcm;
 std::string error;
 if (!(std::strcmp(kind, "wav") == 0 ? readWav(path, pcm, error) : readRaw(path, pcm, error))) {
  if (errOut) *errOut = error;
  return false;
 }
 size_t a = std::min(pcm.size(), size_t(start * 44.1));
 size_t b = length < 0 ? pcm.size() : std::min(pcm.size(), a + size_t(length * 44.1));
 host.feed(pcm.data() + a, b - a);
 return true;
}

static std::string toJson(const vexsim::SimHost& sim) {
 std::ostringstream o;
 o << "{";
 struct JsonEmit {
  std::ostringstream& o;
  bool first = true;
  void sep(const char* k) { if (!first) o << ","; first = false; o << "\"" << k << "\":"; }
  void u(const char* k, uint64_t v) { sep(k); o << v; }
  void i(const char* k, int64_t v) { sep(k); o << v; }
  void f(const char* k, float v) { sep(k); o << v; }
  void b(const char* k, bool v) { sep(k); o << (v ? "true" : "false"); }
  void s(const char* k, const char* v) {
   sep(k); o << "\"";
   for (const char* p = v ? v : ""; *p; ++p) {
    if (*p == '"' || *p == '\\') o << '\\';
    o << *p;
   }
   o << "\"";
  }
 } emit{o};
 vexsim::stateFields(sim.stateSource(), emit);
 if (sim.emit_events) {
  const auto& nt = sim.app().notes;
  uint32_t first = nt.total_events > vex::NoteTracker::EVENT_CAP ? nt.total_events - vex::NoteTracker::EVENT_CAP + 1 : 1;
  o << ",\"events\":[";
  for (uint32_t sq = first; sq <= nt.total_events; sq++) {
   const auto& ev = nt.atSeq(sq);
   char name[8]; vex::midiNoteName(ev.midi, name);
   if (sq > first) o << ",";
   o << "{\"seq\":" << ev.seq
     << ",\"onset_ms\":" << ev.onset_ms_exact
     << ",\"midi\":" << ev.midi
     << ",\"note\":\"" << name << "\""
     << ",\"cents\":" << ev.cents
     << ",\"bend_cents\":" << ev.bend_cents
     << ",\"peak\":" << ev.peak
     << ",\"rms\":" << ev.rms
     << ",\"pitched\":" << (ev.pitched ? "true" : "false")
     << ",\"legato\":" << (ev.legato ? "true" : "false") << "}";
  }
  o << "]";
 }
 o << "}";
 return o.str();
}

int main(int argc, char** argv) {
 const char* frame = argc > 1 ? argv[1] : "build/screen.ppm";
 const char* savepath = argc > 2 ? argv[2] : "build/sim-progress.bin";
 static FileStore store(savepath);
 static vexsim::SimHost sim(store);
 static DesktopCanvas canvas;
 std::string line, ioErr;
 while (std::getline(std::cin, line)) {
  ioErr.clear();
  vexsim::CmdResult r = sim.exec(line.c_str(), loadAudioFile, &ioErr);
  if (r == vexsim::CMD_NOP) continue;
  if (r != vexsim::CMD_OK) {
   if (r == vexsim::CMD_IO_ERR && !ioErr.empty()) std::cerr << ioErr << std::endl;
   return int(r);
  }
  store.flush();
  if (sim.test) vex::drawTestPattern(canvas, sim.app().now);
  else vex::drawScreen(canvas, sim.app().view());
  canvas.write(frame);
  std::cout << toJson(sim) << std::endl;
 }
}
