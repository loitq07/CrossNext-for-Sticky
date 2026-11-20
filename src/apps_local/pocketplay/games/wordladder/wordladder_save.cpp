#include "wordladder_save.h"

namespace wordladder {

std::vector<uint8_t> packDays(const State& state) {
  std::vector<uint8_t> out;
  out.push_back(SAVE_VERSION);
  const size_t count = state.days.size();
  out.push_back(static_cast<uint8_t>(count & 0xFF));
  out.push_back(static_cast<uint8_t>(count >> 8));
  for (const auto& entry : state.days) {
    const uint16_t offset = static_cast<uint16_t>(static_cast<int16_t>(entry.first - kStartDay));
    out.push_back(static_cast<uint8_t>(offset & 0xFF));
    out.push_back(static_cast<uint8_t>(offset >> 8));
    const Record& r = entry.second;
    const size_t n = r.steps.size();
    out.push_back(static_cast<uint8_t>(n));
    out.push_back(r.solved ? 1 : 0);
    for (size_t b = 0; b < (n + 7) / 8; b++) out.push_back(static_cast<uint8_t>(r.hinted >> (8 * b)));
    for (const std::string& w : r.steps) {
      for (char c : w) out.push_back(static_cast<uint8_t>(c));
    }
  }
  return out;
}

namespace {

// One day's ladder from the bytes at `at`, checked against that day's puzzle.
bool readDay(const uint8_t* in, size_t size, size_t& at, long& day, Record& r) {
  if (size - at < 4) return false;
  const int16_t offset = static_cast<int16_t>(in[at] | (in[at + 1] << 8));
  day = kStartDay + offset;
  const size_t n = in[at + 2];
  const uint8_t solved = in[at + 3];
  at += 4;
  if (n > static_cast<size_t>(kMaxSteps) || solved > 1) return false;
  const size_t hintBytes = (n + 7) / 8;
  if (size - at < hintBytes) return false;
  r.hinted = 0;
  for (size_t b = 0; b < hintBytes; b++) r.hinted |= static_cast<uint64_t>(in[at + b]) << (8 * b);
  at += hintBytes;
  // The bits past the last step must be clear, so a save is read only one way.
  if (n < 64 && (r.hinted >> n) != 0) return false;

  const Puzzle p = puzzleFor(day);
  const size_t len = p.length;
  if (size - at < n * len) return false;
  r.steps.clear();
  std::string last = p.start;
  for (size_t i = 0; i < n; i++) {
    std::string w(reinterpret_cast<const char*>(in + at), len);
    at += len;
    for (char c : w) {
      if (c < 'a' || c > 'z') return false;
    }
    if (!isWord(w) || !oneApart(last, w) || w == p.start) return false;
    for (const std::string& s : r.steps) {
      if (s == w) return false;
    }
    // Nothing is played after the target.
    if (w == p.target && i + 1 != n) return false;
    r.steps.push_back(w);
    last = w;
  }
  r.solved = solved == 1;
  if (r.solved != (n > 0 && last == p.target)) return false;
  // An empty, unsolved day is never kept.
  return n > 0;
}

}  // namespace

bool unpackDays(const uint8_t* in, size_t size, State& state) {
  if (size < 3 || in[0] != SAVE_VERSION) return false;
  const size_t count = in[1] | (in[2] << 8);
  if (count > kMaxDays) return false;
  size_t at = 3;
  std::map<long, Record> days;
  long previous = 0;
  for (size_t k = 0; k < count; k++) {
    long day;
    Record r;
    if (!readDay(in, size, at, day, r)) return false;
    // In order, each day once.
    if (k > 0 && day <= previous) return false;
    previous = day;
    days.emplace(day, std::move(r));
  }
  if (at != size) return false;
  state.days = std::move(days);
  return true;
}

}  // namespace wordladder
