#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

// A Z-machine interpreter (versions 3, 4, 5, 7 and 8) for text adventures, translated from the
// Lua of touch-licorice-apps' apps/textadv (src/zmachine.lua, MIT, LICENSE-licorice.txt), after
// the Z-Machine Standards Document 1.1. No screen or card, so it can be tested off the device:
// the game screen sends lines and reads what the machine printed.
//
//   Machine m;  m.load(story, size, error)   the story stays the caller's, unchanged, for as
//                                            long as the machine runs it
//   m.run(limit)                             at most `limit` instructions; More when it ran out
//   m.sendLine(text), m.sendChar(c)          answers a read; then run again
//   m.takeOutput()                           what the lower window printed since, as ASCII
//   m.status(left, right)                    the status line: v3's location and score, or the
//                                            first line of the upper window
//   m.snapshot(out) / m.restore(data, size)  the whole state, for the autosave and SAVE/RESTORE
//
// Text is kept as ZSCII inside the machine, so stream 3 writes the game's own codes, and turned
// into ASCII only for the screen: the panel's font is ASCII, so accented letters lose their
// accents. Every memory, stack and frame access is checked, and a bad one stops the game with a
// message rather than reading or writing past the end.
namespace zmachine {

enum class Result : uint8_t { Input, Char, More, Quit, Error };

class Machine {
 public:
  bool load(const uint8_t* story, size_t size, std::string& error);

  Result run(int limit);
  void sendLine(const std::string& line);
  void sendChar(uint8_t c);
  std::string takeOutput();

  int version() const { return v_; }
  const std::string& error() const { return error_; }
  bool reading() const { return reading_; }
  bool readingChar() const { return readingChar_; }

  void status(std::string& left, std::string& right);

  // The dictionary entry for a word, or 0 if the game doesn't know it.
  uint32_t lookup(const std::string& word);

  // At a read, the state that replays it: the autosave. Fails anywhere else.
  bool snapshot(std::vector<uint8_t>& out) const;
  bool restore(const uint8_t* data, size_t size);

  // The game's own SAVE and RESTORE, one slot a game.
  std::function<bool(const std::vector<uint8_t>&)> onSave;
  std::function<bool(std::vector<uint8_t>&)> onRestore;

  // The random numbers the game draws when it hasn't asked for a predictable sequence.
  void seed(uint32_t s) { rngState_ = s ? s : 1; }

  // Memory, for the word chips and the tests.
  uint8_t rb(uint32_t a);
  uint16_t rw(uint32_t a) { return static_cast<uint16_t>((rb(a) << 8) | rb(a + 1)); }

 private:
  struct Frame {
    uint32_t ret = 0;
    int16_t store = -1;  // the variable the result goes to, or -1 to throw it away
    uint8_t nargs = 0;
    uint32_t base = 0;   // the stack depth when it was called
    uint8_t nlocals = 0;
    uint16_t locals[15] = {};
  };
  struct Stream3 {
    uint32_t addr;
    uint16_t n;
  };

  static constexpr size_t kMaxStack = 8192;
  static constexpr size_t kMaxFrames = 512;
  static constexpr int kCols = 40;
  static constexpr int kRows = 25;

  void reset();
  void setHeader();
  void fail(const char* what, uint32_t at = 0);

  void wb(uint32_t a, uint32_t val);
  void ww(uint32_t a, uint32_t val) {
    wb(a, val >> 8);
    wb(a + 1, val);
  }
  uint32_t unpackAddr(uint32_t p, bool string) const;

  uint8_t alphabet(int n, int i);
  std::string decode(uint32_t a, uint32_t* after = nullptr, bool noAbbrev = false, int depth = 0);
  std::vector<uint16_t> encode(const std::string& word);

  void print(const std::string& zscii);
  void printChar(uint8_t z);
  void split(int n);
  void eraseUpper();

  uint32_t objAddr(uint32_t o);
  uint32_t parent(uint32_t o);
  uint32_t sibling(uint32_t o);
  uint32_t child(uint32_t o);
  void setLink(uint32_t o, int which, uint32_t val);
  uint32_t props(uint32_t o);
  std::string objName(uint32_t o);
  bool attr(uint32_t o, uint32_t n);
  void setAttr(uint32_t o, uint32_t n, bool on);
  void removeObj(uint32_t o);
  void insertObj(uint32_t o, uint32_t d);
  uint32_t firstProp(uint32_t o);
  int propAt(uint32_t a, uint32_t& data, int& len);
  bool findProp(uint32_t o, int n, uint32_t& data, int& len);
  int propLen(uint32_t data);

  void push(uint32_t x);
  uint16_t pop();
  uint16_t getvar(int n, bool peek = false);
  void setvar(int n, uint32_t x, bool inPlace = false);
  void call(uint32_t addr, const uint16_t* args, int nargs, int store);
  void ret(uint32_t x);
  void branch(bool cond);
  void storeResult(uint32_t x);

  void splitWords(const std::string& text, uint32_t dict, std::vector<std::pair<std::string, int>>& out);
  uint32_t lookupIn(const std::string& word, uint32_t dict);
  void tokenise(uint32_t text, uint32_t parse, uint32_t dict, bool flag);

  void doSave();
  void doRestore();
  uint16_t random(int16_t r);

  bool step();
  uint16_t operand(int type);
  int varOperands(int bytes, uint16_t* out);
  void op2(int n, const uint16_t* a, int count);
  void op1(int n, uint16_t a);
  void op0(int n);
  void opVar(int n, const uint16_t* a, int count);
  void opExt(int n, const uint16_t* a, int count);

  const uint8_t* story_ = nullptr;
  size_t storySize_ = 0;
  int v_ = 0;
  uint32_t staticBase_ = 0;
  std::vector<uint8_t> dyn_;
  uint32_t pc_ = 0;
  uint32_t start_ = 0;  // the instruction being run
  std::vector<uint16_t> stack_;
  std::vector<Frame> frames_;
  std::string lower_;  // ASCII, for the screen
  std::vector<Stream3> out3_;
  std::vector<std::string> upper_;
  int upperH_ = 0;
  int window_ = 0;
  int curRow_ = 1;
  int curCol_ = 1;
  bool done_ = false;
  bool failed_ = false;
  std::string error_;

  // The pending read: the text and parse buffers, or a single key, and where it started.
  bool reading_ = false;
  bool readingChar_ = false;
  uint32_t readText_ = 0;
  uint32_t readParse_ = 0;
  uint32_t readStart_ = 0;

  // random: 0 for real random numbers, else the game's predictable sequence.
  uint32_t rngState_ = 1;
  int32_t rngSeed_ = 0;
  uint32_t rngN_ = 0;
  uint32_t rngX_ = 0;
};

}  // namespace zmachine
