#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "wordladder_data.h"

// Word Ladder's rules, translated from the Lua of touch-licorice-apps' apps/wordladder
// (src/main.lua, MIT, LICENSE-licorice.txt), with no screen or card so they can be tested off
// the device. Turn the start word into the target one letter at a time, and every step must be
// a word. Day N from 1 November 2026 plays puzzle N, and the list starts over after the last.
// Words are kept in lower case, as the lists have them; the screen shows them in capitals.
namespace wordladder {

// --- Days: Howard Hinnant's civil calendar, counted from 1 January 1970 ---

constexpr long daysFromCivil(int year, int month, int day) {
  const long y = month <= 2 ? year - 1 : year;
  const long era = (y >= 0 ? y : y - 399) / 400;
  const long yoe = y - era * 400;
  const long doy = (153 * (month > 2 ? month - 3 : month + 9) + 2) / 5 + day - 1;
  const long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + doe - 719468;
}

inline void civilFromDays(long z, int& year, int& month, int& day) {
  z += 719468;
  const long era = (z >= 0 ? z : z - 146096) / 146097;
  const long doe = z - era * 146097;
  const long yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  const long doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  const long mp = (5 * doy + 2) / 153;
  day = static_cast<int>(doy - (153 * mp + 2) / 5 + 1);
  month = static_cast<int>(mp < 10 ? mp + 3 : mp - 9);
  year = static_cast<int>(yoe + era * 400 + (month <= 2 ? 1 : 0));
}

constexpr long kStartDay = daysFromCivil(2026, 11, 1);

// "SAT 7 NOV": 1 January 1970 was a Thursday.
inline std::string dateLabel(long day) {
  static const char* const kDays[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
  static const char* const kMonths[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
  int y, m, d;
  civilFromDays(day, y, m, d);
  const long wd = ((day + 4) % 7 + 7) % 7;
  return std::string(kDays[wd]) + " " + std::to_string(d) + " " + kMonths[m - 1];
}

// --- Words ---

inline const char* listFor(size_t length, int& count) {
  if (length == 4) {
    count = kWords4Count;
    return kWords4;
  }
  if (length == 5) {
    count = kWords5Count;
    return kWords5;
  }
  count = 0;
  return nullptr;
}

// The word's place in its list, or -1.
inline int indexOf(const std::string& w) {
  int count;
  const char* list = listFor(w.size(), count);
  if (!list) return -1;
  int lo = 0;
  int hi = count - 1;
  while (lo <= hi) {
    const int mid = (lo + hi) / 2;
    const int c = std::strncmp(list + static_cast<size_t>(mid) * w.size(), w.c_str(), w.size());
    if (c == 0) return mid;
    if (c < 0) {
      lo = mid + 1;
    } else {
      hi = mid - 1;
    }
  }
  return -1;
}

inline bool isWord(const std::string& w) { return indexOf(w) >= 0; }

inline std::string wordAt(size_t length, int index) {
  int count;
  const char* list = listFor(length, count);
  return std::string(list + static_cast<size_t>(index) * length, length);
}

inline bool oneApart(const std::string& a, const std::string& b) {
  if (a.size() != b.size()) return false;
  int diff = 0;
  for (size_t i = 0; i < a.size(); i++) diff += a[i] != b[i];
  return diff == 1;
}

// --- Puzzles ---

struct Puzzle {
  int number = 0;                 // 1 to kPuzzleCount
  std::vector<std::string> path;  // a shortest ladder, start to target
  std::string start, target;
  size_t length = 0;
  int par = 0;
};

inline int puzzleIndex(long day) {
  const long k = (day - kStartDay) % kPuzzleCount;
  return static_cast<int>(k < 0 ? k + kPuzzleCount : k);
}

inline Puzzle puzzleFor(long day) {
  Puzzle p;
  const int index = puzzleIndex(day);
  p.number = index + 1;
  const char* line = kPuzzles[index];
  std::string word;
  for (const char* c = line;; c++) {
    if (*c == ' ' || *c == 0) {
      if (!word.empty()) p.path.push_back(word);
      word.clear();
      if (*c == 0) break;
    } else {
      word.push_back(*c);
    }
  }
  p.start = p.path.front();
  p.target = p.path.back();
  p.length = p.start.size();
  p.par = static_cast<int>(p.path.size()) - 1;
  return p;
}

// --- A ladder being played ---

constexpr int kMaxSteps = 60;

struct Ladder {
  Puzzle puzzle;
  std::vector<std::string> steps;  // the words after the start
  uint64_t hinted = 0;             // bit i: step i came from HINT
  bool solved = false;
  int pos = 0;                     // the letter of the newest word to change
};

inline const std::string& current(const Ladder& l) { return l.steps.empty() ? l.puzzle.start : l.steps.back(); }

inline int hintCount(const Ladder& l) {
  int n = 0;
  for (size_t i = 0; i < l.steps.size(); i++) n += (l.hinted >> i) & 1;
  return n;
}

// The first letter of the newest word that isn't yet the target's.
inline int firstOpen(const Ladder& l) {
  const std::string& w = current(l);
  for (size_t i = 0; i < w.size(); i++) {
    if (w[i] != l.puzzle.target[i]) return static_cast<int>(i);
  }
  return 0;
}

inline bool inLadder(const Ladder& l, const std::string& w) {
  if (w == l.puzzle.start) return true;
  for (const std::string& s : l.steps) {
    if (s == w) return true;
  }
  return false;
}

inline void start(Ladder& l, const Puzzle& p) {
  l.puzzle = p;
  l.steps.clear();
  l.hinted = 0;
  l.solved = false;
  l.pos = firstOpen(l);
}

inline void restart(Ladder& l) { start(l, l.puzzle); }

enum class Play : uint8_t { Same, NotWord, Used, Full, Added, Solved, Over, NoLadder };

inline Play addStep(Ladder& l, const std::string& w, bool fromHint) {
  if (static_cast<int>(l.steps.size()) >= kMaxSteps) return Play::Full;
  if (fromHint) l.hinted |= uint64_t{1} << l.steps.size();
  l.steps.push_back(w);
  if (w == l.puzzle.target) {
    l.solved = true;
    return Play::Solved;
  }
  l.pos = firstOpen(l);
  return Play::Added;
}

// A key: the framed letter of the newest word becomes `letter`. `word` gets the word tried.
inline Play play(Ladder& l, char letter, std::string& word) {
  if (l.solved) return Play::Over;
  word = current(l);
  word[static_cast<size_t>(l.pos)] = letter;
  if (word == current(l)) return Play::Same;
  if (!isWord(word)) return Play::NotWord;
  if (inLadder(l, word)) return Play::Used;
  return addStep(l, word, false);
}

inline bool undo(Ladder& l) {
  if (l.steps.empty()) return false;
  l.steps.pop_back();
  l.hinted &= (uint64_t{1} << l.steps.size()) - 1;
  l.solved = false;
  l.pos = firstOpen(l);
  return true;
}

// The next word of a shortest ladder from the newest word to the target: the stored one if
// the ladder is on it, else a breadth-first search over every word of this length. Words
// already in the ladder are never offered, since a word can't be used twice. Empty if the
// target can't be reached from here.
inline std::string nextWord(const Ladder& l) {
  const std::string& w = current(l);
  const std::vector<std::string>& path = l.puzzle.path;
  for (size_t i = 0; i + 1 < path.size(); i++) {
    if (path[i] == w && !inLadder(l, path[i + 1])) return path[i + 1];
  }
  const size_t n = w.size();
  int count;
  listFor(n, count);
  const int from = indexOf(w);
  const int to = indexOf(l.puzzle.target);
  if (from < 0 || to < 0) return std::string();
  // Searched from the target, so the first step found back to here is the answer. A word
  // already used counts as seen, so the search goes round it.
  std::vector<int16_t> next(static_cast<size_t>(count), -1);
  auto block = [&](const std::string& used) {
    const int k = indexOf(used);
    if (k >= 0 && k != from) next[static_cast<size_t>(k)] = static_cast<int16_t>(k);
  };
  block(l.puzzle.start);
  for (const std::string& s : l.steps) block(s);
  std::vector<int16_t> queue;
  queue.reserve(static_cast<size_t>(count));
  next[static_cast<size_t>(to)] = static_cast<int16_t>(to);
  queue.push_back(static_cast<int16_t>(to));
  for (size_t head = 0; head < queue.size(); head++) {
    const int x = queue[head];
    std::string v = wordAt(n, x);
    for (size_t i = 0; i < n; i++) {
      const char was = v[i];
      for (char c = 'a'; c <= 'z'; c++) {
        if (c == was) continue;
        v[i] = c;
        const int k = indexOf(v);
        if (k >= 0 && next[static_cast<size_t>(k)] < 0) {
          next[static_cast<size_t>(k)] = static_cast<int16_t>(x);
          if (k == from) return wordAt(n, x);
          queue.push_back(static_cast<int16_t>(k));
        }
      }
      v[i] = was;
    }
  }
  return std::string();
}

// HINT: the next word of a shortest ladder joins it. `word` gets the word added.
inline Play hint(Ladder& l, std::string& word) {
  if (l.solved) return Play::Over;
  word = nextWord(l);
  if (word.empty()) return Play::NoLadder;
  return addStep(l, word, true);
}

}  // namespace wordladder
