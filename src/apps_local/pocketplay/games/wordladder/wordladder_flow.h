#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "wordladder_rules.h"

// What each tap does to Word Ladder, with no screen or card, so it can be tested off the device:
// the board, the menu and its pages, the message line, and the ladder kept for every day played.
namespace wordladder {

enum class Mode : uint8_t { Board, Menu, Answer, Rules };

enum class Message : uint8_t { Help, NotWord, Used, Full, Hint, NoLadder, Solved };

// What a tap changed, for the screen to choose its refresh and whether to save.
enum class Change : uint8_t { None, View, Move, NewLadder, Solved };

// One day's ladder as kept: the steps after the start, which came from HINT, and if solved.
struct Record {
  std::vector<std::string> steps;
  uint64_t hinted = 0;
  bool solved = false;
};

// The source keeps the newest 400 days.
constexpr size_t kMaxDays = 400;

struct State {
  long today = kStartDay;
  long day = kStartDay;
  Ladder ladder;
  std::map<long, Record> days;
  Mode mode = Mode::Board;
  Message message = Message::Help;
  std::string word;  // the word a message names
};

// The ladder on screen, into `days`. A day with nothing played isn't kept.
inline void keep(State& s) {
  const Ladder& l = s.ladder;
  const long offset = s.day - kStartDay;
  if ((l.steps.empty() && !l.solved) || offset < -32768 || offset > 32767) {
    s.days.erase(s.day);
  } else {
    Record& r = s.days[s.day];
    r.steps = l.steps;
    r.hinted = l.hinted;
    r.solved = l.solved;
  }
  while (s.days.size() > kMaxDays) s.days.erase(s.days.begin());
}

// Opens a day's puzzle, with its kept ladder if there is one.
inline void openDay(State& s, long day) {
  s.day = day;
  start(s.ladder, puzzleFor(day));
  const auto it = s.days.find(day);
  if (it != s.days.end()) {
    s.ladder.steps = it->second.steps;
    s.ladder.hinted = it->second.hinted;
    s.ladder.solved = it->second.solved;
    s.ladder.pos = firstOpen(s.ladder);
  }
  s.mode = Mode::Board;
  s.message = s.ladder.solved ? Message::Solved : Message::Help;
  s.word.clear();
}

inline Change setMessage(State& s, Message m, const std::string& word = std::string()) {
  if (s.message == m && s.word == word) return Change::None;
  s.message = m;
  s.word = word;
  return Change::View;
}

inline Change result(State& s, Play p, const std::string& word) {
  switch (p) {
    case Play::Same:
    case Play::Over:
      return Change::None;
    case Play::NotWord:
      return setMessage(s, Message::NotWord, word);
    case Play::Used:
      return setMessage(s, Message::Used, word);
    case Play::Full:
      return setMessage(s, Message::Full);
    case Play::NoLadder:
      return setMessage(s, Message::NoLadder);
    case Play::Added:  // the caller has set the message
      keep(s);
      return Change::Move;
    case Play::Solved:
      keep(s);
      s.message = Message::Solved;
      s.word.clear();
      return Change::Solved;
  }
  return Change::None;
}

// A letter key.
inline Change pressKey(State& s, char letter) {
  if (s.mode != Mode::Board) return Change::None;
  std::string word;
  const Play p = play(s.ladder, letter, word);
  if (p == Play::Added) {
    s.message = Message::Help;
    s.word.clear();
  }
  return result(s, p, word);
}

// A letter of the newest word, to frame it.
inline Change choose(State& s, int pos) {
  if (s.mode != Mode::Board || s.ladder.solved || pos < 0 || pos >= static_cast<int>(s.ladder.puzzle.length) ||
      pos == s.ladder.pos) {
    return Change::None;
  }
  s.ladder.pos = pos;
  s.message = Message::Help;
  s.word.clear();
  return Change::View;
}

// The Up and Down buttons move the frame along the newest word, round at the ends.
inline Change stepFrame(State& s, int delta) {
  if (s.mode != Mode::Board || s.ladder.solved) return Change::None;
  const int n = static_cast<int>(s.ladder.puzzle.length);
  s.ladder.pos = ((s.ladder.pos + delta) % n + n) % n;
  s.message = Message::Help;
  s.word.clear();
  return Change::View;
}

inline Change undoTap(State& s) {
  if (s.mode != Mode::Board || !undo(s.ladder)) return Change::None;
  keep(s);
  s.message = Message::Help;
  s.word.clear();
  return Change::Move;
}

inline Change hintTap(State& s) {
  if (s.mode != Mode::Board) return Change::None;
  std::string word;
  const Play p = hint(s.ladder, word);
  if (p == Play::Added) {
    s.message = Message::Hint;
    s.word = word;
  }
  return result(s, p, word);
}

inline bool canUndo(const State& s) { return !s.ladder.steps.empty(); }
inline bool canHint(const State& s) { return !s.ladder.solved; }

// --- The menu ---

inline Change openMenu(State& s) {
  if (s.mode != Mode::Board) return Change::None;
  s.mode = Mode::Menu;
  return Change::View;
}

// BACK from a page goes back to the board.
inline Change closePage(State& s) {
  if (s.mode == Mode::Board) return Change::None;
  s.mode = Mode::Board;
  return Change::View;
}

inline bool canToday(const State& s) { return s.day != s.today; }
inline bool canLater(const State& s) { return s.day < s.today; }
// The save keeps a day as two bytes from 1 November 2026, so EARLIER stops 89 years back.
inline bool canEarlier(const State& s) { return s.day - 1 - kStartDay > -32768; }
inline bool canStartOver(const State& s) { return !s.ladder.steps.empty() || s.ladder.solved; }

inline Change goToDay(State& s, long day) {
  openDay(s, day);
  return Change::NewLadder;
}

inline Change todayTap(State& s) { return canToday(s) ? goToDay(s, s.today) : Change::None; }
inline Change earlierTap(State& s) { return canEarlier(s) ? goToDay(s, s.day - 1) : Change::None; }
inline Change laterTap(State& s) { return canLater(s) ? goToDay(s, s.day + 1) : Change::None; }

inline Change startOverTap(State& s) {
  if (!canStartOver(s)) return Change::None;
  restart(s.ladder);
  keep(s);
  s.mode = Mode::Board;
  s.message = Message::Help;
  s.word.clear();
  return Change::NewLadder;
}

inline Change openAnswer(State& s) {
  s.mode = Mode::Answer;
  return Change::View;
}

inline Change openRules(State& s) {
  s.mode = Mode::Rules;
  return Change::View;
}

// --- The rows of the ladder on screen ---

// The start at the top and the target waiting at the foot, with the steps between. When there
// are more steps than room, the newest are shown and one row counts the rest.
constexpr int kRows = 7;

enum class RowKind : uint8_t { Start, Step, Gap, Target };

struct Row {
  RowKind kind;
  int step;  // a Step's index, or a Gap's count of hidden steps
  int slot;  // 0 at the top to kRows - 1
};

inline std::vector<Row> rows(const Ladder& l) {
  std::vector<Row> out;
  out.push_back({RowKind::Start, 0, 0});
  const int n = static_cast<int>(l.steps.size());
  const int room = kRows - 1 - (l.solved ? 0 : 1);
  int first = 0;
  int slot = 1;
  if (n > room) {
    first = n - (room - 1);
    out.push_back({RowKind::Gap, first, slot++});
  }
  for (int i = first; i < n; i++) out.push_back({RowKind::Step, i, slot++});
  if (!l.solved) out.push_back({RowKind::Target, 0, kRows - 1});
  return out;
}

// The slot of the newest word, the one being changed.
inline int newestSlot(const Ladder& l) {
  for (const Row& r : rows(l)) {
    if ((r.kind == RowKind::Step && r.step == static_cast<int>(l.steps.size()) - 1) ||
        (r.kind == RowKind::Start && l.steps.empty())) {
      return r.slot;
    }
  }
  return 0;
}

}  // namespace wordladder
