#include "zmachine.h"

#include <cstdio>
#include <cstring>

namespace zmachine {

namespace {

// The default Unicode table's letters (ZSCII 155 to 223), as the plain ASCII letters the panel
// can draw: accents are dropped, and a few become two letters.
const char* const UNI_ASCII[69] = {
    "a",  "o",  "u",  "A",  "O",  "U",  "ss", ">>", "<<", "e", "i", "y", "E", "I", "a", "e", "i", "o",
    "u",  "y",  "A",  "E",  "I",  "O",  "U",  "Y",  "a",  "e", "i", "o", "u", "A", "E", "I", "O", "U",
    "a",  "e",  "i",  "o",  "u",  "A",  "E",  "I",  "O",  "U", "a", "A", "o", "O", "a", "n", "o", "A",
    "N",  "O",  "ae", "AE", "c",  "C",  "th", "th", "Th", "Th", "L", "oe", "OE", "!", "?"};

// The same letters as Unicode code points, for print_unicode (the source's UNI).
const uint16_t UNI[69] = {0xe4, 0xf6, 0xfc, 0xc4, 0xd6, 0xdc, 0xdf, 0xbb, 0xab, 0xeb, 0xef, 0xff, 0xcb, 0xcf,
                          0xe1, 0xe9, 0xed, 0xf3, 0xfa, 0xfd, 0xc1, 0xc9, 0xcd, 0xd3, 0xda, 0xdd, 0xe0, 0xe8,
                          0xec, 0xf2, 0xf9, 0xc0, 0xc8, 0xcc, 0xd2, 0xd9, 0xe2, 0xea, 0xee, 0xf4, 0xfb, 0xc2,
                          0xca, 0xce, 0xd4, 0xdb, 0xe5, 0xc5, 0xf8, 0xd8, 0xe3, 0xf1, 0xf5, 0xc3, 0xd1, 0xd5,
                          0xe6, 0xc6, 0xe7, 0xc7, 0xfe, 0xf0, 0xde, 0xd0, 0xa3, 0x153, 0x152, 0xa1, 0xbf};

const char A0[] = "abcdefghijklmnopqrstuvwxyz";
const char A1[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const char A2[] = " \n0123456789.,!?_#'\"/\\-:()";

int16_t sgn(uint32_t x) { return static_cast<int16_t>(x & 0xFFFF); }

constexpr uint32_t SNAPSHOT_MAGIC = 0x5A4D5031;  // "ZMP1"

void putU32(std::vector<uint8_t>& out, uint32_t x) {
  for (int s = 24; s >= 0; s -= 8) out.push_back(static_cast<uint8_t>(x >> s));
}
void putU16(std::vector<uint8_t>& out, uint32_t x) {
  out.push_back(static_cast<uint8_t>(x >> 8));
  out.push_back(static_cast<uint8_t>(x));
}

struct Reader {
  const uint8_t* p;
  size_t size;
  size_t at = 0;
  bool ok = true;
  uint32_t u8() {
    if (at + 1 > size) {
      ok = false;
      return 0;
    }
    return p[at++];
  }
  uint32_t u16() { return (u8() << 8) | u8(); }
  uint32_t u32() { return (u16() << 16) | u16(); }
};

}  // namespace

// ------------------------------------------------------------------ loading and memory

bool Machine::load(const uint8_t* story, size_t size, std::string& error) {
  if (size < 64) {
    error = "THAT FILE IS TOO SHORT TO BE A STORY.";
    return false;
  }
  const int v = story[0];
  if (v != 3 && v != 4 && v != 5 && v != 7 && v != 8) {
    char text[64];
    std::snprintf(text, sizeof(text), "STORY VERSION %d ISN'T SUPPORTED.", v);
    error = text;
    return false;
  }
  const uint32_t base = (story[0x0E] << 8) | story[0x0F];
  if (base < 64 || base > size) {
    error = "THAT STORY FILE IS DAMAGED.";
    return false;
  }
  story_ = story;
  storySize_ = size;
  v_ = v;
  reset();
  return true;
}

void Machine::reset() {
  staticBase_ = (story_[0x0E] << 8) | story_[0x0F];
  dyn_.assign(story_, story_ + staticBase_);
  pc_ = (story_[0x06] << 8) | story_[0x07];
  stack_.clear();
  frames_.clear();
  lower_.clear();
  out3_.clear();
  upper_.clear();
  upperH_ = 0;
  window_ = 0;
  curRow_ = 1;
  curCol_ = 1;
  rngSeed_ = 0;
  done_ = false;
  failed_ = false;
  error_.clear();
  reading_ = false;
  readingChar_ = false;
  setHeader();
}

void Machine::fail(const char* what, uint32_t at) {
  if (failed_) return;
  failed_ = true;
  char text[96];
  std::snprintf(text, sizeof(text), "%s AT %X", what, static_cast<unsigned>(at ? at : start_));
  error_ = text;
}

uint8_t Machine::rb(uint32_t a) {
  if (a < staticBase_) return dyn_[a];
  return a < storySize_ ? story_[a] : 0;
}

void Machine::wb(uint32_t a, uint32_t val) {
  if (a >= staticBase_) {
    fail("WRITE TO STATIC MEMORY", a);
    return;
  }
  dyn_[a] = static_cast<uint8_t>(val);
}

// Version 7 adds the header's routine or string offset; the source treats it as version 8.
uint32_t Machine::unpackAddr(uint32_t p, bool string) const {
  if (v_ <= 3) return 2 * p;
  if (v_ <= 5) return 4 * p;
  if (v_ == 7) {
    const uint32_t offset = string ? ((story_[0x2A] << 8) | story_[0x2B]) : ((story_[0x28] << 8) | story_[0x29]);
    return 4 * p + 8 * offset;
  }
  return 8 * p;
}

void Machine::setHeader() {
  uint32_t f1 = rb(1);
  if (v_ <= 3) {
    f1 = (f1 & ~0x70u) | 0x20;  // status line available, split screen available
  } else {
    f1 = 0;  // no colours, styles or timed input
  }
  wb(1, f1);
  wb(0x10, rb(0x10) & ~0x01u);  // flags 2: no transcript
  if (v_ >= 4) {
    wb(0x1E, 6);
    wb(0x1F, 'A');
    wb(0x20, kRows);
    wb(0x21, kCols);
  }
  if (v_ >= 5) {
    ww(0x22, kCols);
    ww(0x24, kRows);
    wb(0x26, 1);
    wb(0x27, 1);
    wb(0x2C, 9);  // background white
    wb(0x2D, 2);  // text black
  }
  wb(0x32, 1);
  wb(0x33, 1);
}

// ------------------------------------------------------------------ text

uint8_t Machine::alphabet(int n, int i) {
  const uint32_t t = v_ >= 5 ? rw(0x34) : 0;
  if (t != 0) return rb(t + n * 26 + i - 6);
  if (n == 2 && i == 7) return 13;
  const char* s = n == 0 ? A0 : n == 1 ? A1 : A2;
  return static_cast<uint8_t>(s[i - 6]);
}

// The z-string at a, as ZSCII; *after is the address after it.
std::string Machine::decode(uint32_t a, uint32_t* after, bool noAbbrev, int depth) {
  std::string out;
  int alpha = 0;
  int shiftOnce = -1;
  int abbrev = 0;
  int zh = -2;  // -2 none, -1 waiting for the high half of a 10-bit code, else the high half
  for (int words = 0; words < 4096 && !failed_; words++) {
    const uint16_t word = rw(a);
    a += 2;
    for (int k = 2; k >= 0; k--) {
      const int c = (word >> (k * 5)) & 31;
      const int cur = shiftOnce >= 0 ? shiftOnce : alpha;
      shiftOnce = -1;
      if (zh == -1) {
        zh = c;
      } else if (zh >= 0) {
        out.push_back(static_cast<char>((zh << 5) | c));
        zh = -2;
      } else if (abbrev) {
        const uint32_t idx = 32 * (abbrev - 1) + c;
        const uint32_t addr = rw(rw(0x18) + 2 * idx) * 2u;
        if (depth < 2) out += decode(addr, nullptr, true, depth + 1);
        abbrev = 0;
      } else if (c == 0) {
        out.push_back(' ');
      } else if (c >= 1 && c <= 3) {
        if (!noAbbrev) abbrev = c;
      } else if (c == 4) {
        shiftOnce = 1;
      } else if (c == 5) {
        shiftOnce = 2;
      } else if (cur == 2 && c == 6) {
        zh = -1;
      } else {
        out.push_back(static_cast<char>(alphabet(cur, c)));
      }
    }
    if (word & 0x8000) break;
  }
  if (after) *after = a;
  return out;
}

// A word's dictionary form: 6 (v3) or 9 z-chars, packed into 2 or 3 words.
std::vector<uint16_t> Machine::encode(const std::string& word) {
  const size_t n = v_ <= 3 ? 6 : 9;
  std::vector<int> zs;
  for (unsigned char ch : word) {
    bool found = false;
    for (int alpha = 0; alpha <= 2 && !found; alpha++) {
      for (int zc = 6; zc <= 31; zc++) {
        if (alpha == 2 && (zc == 6 || zc == 7)) continue;
        if (alphabet(alpha, zc) == ch) {
          if (alpha > 0) zs.push_back(alpha + 3);
          zs.push_back(zc);
          found = true;
          break;
        }
      }
    }
    if (!found) {
      zs.push_back(5);
      zs.push_back(6);
      zs.push_back((ch >> 5) & 31);
      zs.push_back(ch & 31);
    }
    if (zs.size() >= n) break;
  }
  while (zs.size() < n) zs.push_back(5);
  std::vector<uint16_t> words;
  for (size_t i = 0; i < n; i += 3) words.push_back(static_cast<uint16_t>((zs[i] << 10) | (zs[i + 1] << 5) | zs[i + 2]));
  words.back() |= 0x8000;
  return words;
}

// ------------------------------------------------------------------ output

void Machine::printChar(uint8_t z) {
  if (!out3_.empty()) {
    Stream3& t = out3_.back();
    wb(t.addr + 2 + t.n, z);
    t.n++;
    return;
  }
  std::string ascii;
  if (z == 13 || z == 10) {
    ascii = "\n";
  } else if (z >= 32 && z <= 126) {
    ascii.push_back(static_cast<char>(z));
  } else if (z >= 155 && z <= 223) {
    ascii = UNI_ASCII[z - 155];
  } else {
    return;
  }
  if (window_ == 1) {
    for (char ch : ascii) {
      if (ch == '\n') {
        curRow_++;
        curCol_ = 1;
        continue;
      }
      if (curRow_ >= 1 && curRow_ <= static_cast<int>(upper_.size()) && curCol_ >= 1 && curCol_ <= kCols) {
        upper_[curRow_ - 1][curCol_ - 1] = ch;
      }
      curCol_++;
    }
    return;
  }
  lower_ += ascii;
}

void Machine::print(const std::string& zscii) {
  for (char c : zscii) printChar(static_cast<uint8_t>(c));
}

std::string Machine::takeOutput() {
  std::string out;
  out.swap(lower_);
  return out;
}

void Machine::split(int n) {
  if (n < 0) n = 0;
  if (n > kRows) n = kRows;
  upperH_ = n;
  while (static_cast<int>(upper_.size()) < n) upper_.emplace_back(kCols, ' ');
  if (v_ == 3) {
    for (int r = 0; r < n; r++) upper_[r].assign(kCols, ' ');
  }
}

void Machine::eraseUpper() {
  for (std::string& row : upper_) row.assign(kCols, ' ');
}

void Machine::status(std::string& left, std::string& right) {
  left.clear();
  right.clear();
  if (v_ <= 3) {
    const uint32_t g = rw(0x0C);
    left = objName(rw(g));
    std::string ascii;
    for (char c : left) {
      const uint8_t z = static_cast<uint8_t>(c);
      if (z >= 32 && z <= 126) ascii.push_back(c);
      else if (z >= 155 && z <= 223) ascii += UNI_ASCII[z - 155];
    }
    left = ascii;
    const int a = sgn(rw(g + 2));
    const unsigned b = rw(g + 4);
    char text[40];
    if (rb(1) & 0x02) {
      std::snprintf(text, sizeof(text), "%d:%02u", a, b);
    } else {
      std::snprintf(text, sizeof(text), "SCORE %d   MOVES %u", a, b);
    }
    right = text;
    return;
  }
  // The first line of the upper window, its runs of spaces closed up.
  if (upperH_ == 0 || upper_.empty()) return;
  const std::string& row = upper_[0];
  size_t i = 0;
  while (i < row.size() && row[i] == ' ') i++;
  int spaces = 0;
  for (; i < row.size(); i++) {
    if (row[i] == ' ') {
      spaces++;
      continue;
    }
    if (spaces) left += spaces > 1 ? "   " : " ";
    spaces = 0;
    left.push_back(row[i]);
  }
}

// ------------------------------------------------------------------ objects

uint32_t Machine::objAddr(uint32_t o) {
  if (v_ <= 3) return rw(0x0A) + 62 + (o - 1) * 9;
  return rw(0x0A) + 126 + (o - 1) * 14;
}
uint32_t Machine::parent(uint32_t o) {
  const uint32_t a = objAddr(o);
  return v_ <= 3 ? rb(a + 4) : rw(a + 6);
}
uint32_t Machine::sibling(uint32_t o) {
  const uint32_t a = objAddr(o);
  return v_ <= 3 ? rb(a + 5) : rw(a + 8);
}
uint32_t Machine::child(uint32_t o) {
  const uint32_t a = objAddr(o);
  return v_ <= 3 ? rb(a + 6) : rw(a + 10);
}
// which: 1 parent, 2 sibling, 3 child.
void Machine::setLink(uint32_t o, int which, uint32_t val) {
  const uint32_t a = objAddr(o);
  if (v_ <= 3) {
    wb(a + 3 + which, val);
  } else {
    ww(a + 4 + which * 2, val);
  }
}
uint32_t Machine::props(uint32_t o) { return rw(objAddr(o) + (v_ <= 3 ? 7 : 12)); }

std::string Machine::objName(uint32_t o) {
  if (o == 0) return std::string();
  const uint32_t p = props(o);
  if (rb(p) == 0) return std::string();
  return decode(p + 1);
}

bool Machine::attr(uint32_t o, uint32_t n) { return (rb(objAddr(o) + (n >> 3)) & (0x80 >> (n & 7))) != 0; }

void Machine::setAttr(uint32_t o, uint32_t n, bool on) {
  const uint32_t a = objAddr(o) + (n >> 3);
  uint32_t b = rb(a);
  if (on) {
    b |= 0x80u >> (n & 7);
  } else {
    b &= ~(0x80u >> (n & 7));
  }
  wb(a, b);
}

void Machine::removeObj(uint32_t o) {
  const uint32_t p = parent(o);
  if (p == 0) return;
  const uint32_t first = child(p);
  if (first == o) {
    setLink(p, 3, sibling(o));
  } else {
    uint32_t x = first;
    for (int guard = 0; x != 0 && guard < 65536; guard++) {
      const uint32_t s = sibling(x);
      if (s == o) {
        setLink(x, 2, sibling(o));
        break;
      }
      x = s;
    }
  }
  setLink(o, 1, 0);
  setLink(o, 2, 0);
}

void Machine::insertObj(uint32_t o, uint32_t d) {
  removeObj(o);
  setLink(o, 2, child(d));
  setLink(d, 3, o);
  setLink(o, 1, d);
}

// The first property entry of o: the address of its size byte.
uint32_t Machine::firstProp(uint32_t o) {
  const uint32_t p = props(o);
  return p + 1 + 2 * rb(p);
}

// The number, data address and length of the property entry at a; number 0 is the end.
int Machine::propAt(uint32_t a, uint32_t& data, int& len) {
  const uint32_t b = rb(a);
  if (v_ <= 3) {
    if (b == 0) return 0;
    data = a + 1;
    len = static_cast<int>(b >> 5) + 1;
    return b & 31;
  }
  if (b & 0x80) {
    len = rb(a + 1) & 63;
    if (len == 0) len = 64;
    data = a + 2;
    return b & 63;
  }
  if (b == 0) return 0;
  data = a + 1;
  len = (b & 0x40) ? 2 : 1;
  return b & 63;
}

bool Machine::findProp(uint32_t o, int n, uint32_t& data, int& len) {
  uint32_t a = firstProp(o);
  for (int guard = 0; guard < 256; guard++) {
    const int num = propAt(a, data, len);
    if (num == 0) return false;
    if (num == n) return true;
    if (num < n) return false;
    a = data + len;
  }
  return false;
}

int Machine::propLen(uint32_t data) {
  if (data == 0) return 0;
  const uint32_t b = rb(data - 1);
  if (v_ <= 3) return static_cast<int>(b >> 5) + 1;
  if (b & 0x80) {
    const int l = b & 63;
    return l == 0 ? 64 : l;
  }
  return (b & 0x40) ? 2 : 1;
}

// ------------------------------------------------------------------ variables and calls

void Machine::push(uint32_t x) {
  if (stack_.size() >= kMaxStack) {
    fail("STACK OVERFLOW");
    return;
  }
  stack_.push_back(static_cast<uint16_t>(x));
}

uint16_t Machine::pop() {
  const size_t base = frames_.empty() ? 0 : frames_.back().base;
  if (stack_.size() <= base) {
    fail("STACK UNDERFLOW");
    return 0;
  }
  const uint16_t x = stack_.back();
  stack_.pop_back();
  return x;
}

uint16_t Machine::getvar(int n, bool peek) {
  if (n == 0) {
    if (peek) {
      if (stack_.empty()) {
        fail("STACK UNDERFLOW");
        return 0;
      }
      return stack_.back();
    }
    return pop();
  }
  if (n < 16) {
    // A local the routine didn't declare reads as 0, as in the source.
    if (frames_.empty() || n > frames_.back().nlocals) return 0;
    return frames_.back().locals[n - 1];
  }
  return rw(rw(0x0C) + 2 * (n - 16));
}

void Machine::setvar(int n, uint32_t x, bool inPlace) {
  x &= 0xFFFF;
  if (n == 0) {
    if (inPlace) {
      if (stack_.empty()) {
        fail("STACK UNDERFLOW");
        return;
      }
      stack_.back() = static_cast<uint16_t>(x);
    } else {
      push(x);
    }
  } else if (n < 16) {
    // And writing one changes nothing.
    if (frames_.empty() || n > frames_.back().nlocals) return;
    frames_.back().locals[n - 1] = static_cast<uint16_t>(x);
  } else {
    ww(rw(0x0C) + 2 * (n - 16), x);
  }
}

void Machine::call(uint32_t addr, const uint16_t* args, int nargs, int store) {
  if (addr == 0) {
    if (store >= 0) setvar(store, 0);
    return;
  }
  if (frames_.size() >= kMaxFrames) {
    fail("TOO MANY NESTED CALLS");
    return;
  }
  uint32_t a = unpackAddr(addr, false);
  Frame f;
  const int nl = rb(a);
  if (nl > 15) {
    fail("A ROUTINE WITH TOO MANY LOCALS", a);
    return;
  }
  a++;
  f.nlocals = static_cast<uint8_t>(nl);
  for (int i = 0; i < nl; i++) {
    if (v_ <= 4) {
      f.locals[i] = rw(a);
      a += 2;
    } else {
      f.locals[i] = 0;
    }
  }
  for (int i = 0; i < nargs && i < nl; i++) f.locals[i] = args[i];
  f.ret = pc_;
  f.store = static_cast<int16_t>(store);
  f.nargs = static_cast<uint8_t>(nargs);
  f.base = static_cast<uint32_t>(stack_.size());
  frames_.push_back(f);
  pc_ = a;
}

void Machine::ret(uint32_t x) {
  if (frames_.empty()) {
    fail("RETURN FROM THE MAIN ROUTINE");
    return;
  }
  const Frame f = frames_.back();
  frames_.pop_back();
  stack_.resize(f.base);
  pc_ = f.ret;
  if (f.store >= 0) setvar(f.store, x);
}

void Machine::branch(bool cond) {
  const uint32_t b = rb(pc_);
  pc_++;
  const bool on = (b & 0x80) != 0;
  int32_t off;
  if (b & 0x40) {
    off = b & 63;
  } else {
    off = static_cast<int32_t>(((b & 63) << 8) | rb(pc_));
    pc_++;
    if (off >= 0x2000) off -= 0x4000;
  }
  if (cond != on) return;
  if (off == 0) {
    ret(0);
  } else if (off == 1) {
    ret(1);
  } else {
    pc_ = static_cast<uint32_t>(static_cast<int32_t>(pc_) + off - 2);
  }
}

void Machine::storeResult(uint32_t x) {
  const int n = rb(pc_);
  pc_++;
  setvar(n, x);
}

// ------------------------------------------------------------------ input

// The words of a line, as the dictionary splits them, with each one's 1-based start.
void Machine::splitWords(const std::string& text, uint32_t dict, std::vector<std::pair<std::string, int>>& out) {
  const int nsep = rb(dict);
  bool seps[256] = {};
  for (int i = 1; i <= nsep; i++) seps[rb(dict + i)] = true;
  size_t i = 0;
  while (i < text.size()) {
    const uint8_t ch = static_cast<uint8_t>(text[i]);
    if (ch == ' ') {
      i++;
    } else if (seps[ch]) {
      out.emplace_back(text.substr(i, 1), static_cast<int>(i) + 1);
      i++;
    } else {
      size_t j = i;
      while (j < text.size() && text[j] != ' ' && !seps[static_cast<uint8_t>(text[j])]) j++;
      out.emplace_back(text.substr(i, j - i), static_cast<int>(i) + 1);
      i = j;
    }
  }
}

uint32_t Machine::lookupIn(const std::string& word, uint32_t dict) {
  const int nsep = rb(dict);
  const uint32_t base = dict + 1 + nsep;
  const int elen = rb(base);
  const int count = sgn(rw(base + 1));
  const uint32_t first = base + 3;
  const std::vector<uint16_t> enc = encode(word);
  auto cmp = [&](uint32_t e) {
    for (size_t k = 0; k < enc.size(); k++) {
      const uint16_t w = rw(e + 2 * k);
      if (w != enc[k]) return w < enc[k] ? -1 : 1;
    }
    return 0;
  };
  if (count < 0) {
    for (int i = 0; i < -count; i++) {
      if (cmp(first + i * elen) == 0) return first + i * elen;
    }
    return 0;
  }
  int lo = 0;
  int hi = count - 1;
  while (lo <= hi) {
    const int mid = (lo + hi) / 2;
    const int c = cmp(first + mid * elen);
    if (c == 0) return first + mid * elen;
    if (c < 0) {
      lo = mid + 1;
    } else {
      hi = mid - 1;
    }
  }
  return 0;
}

uint32_t Machine::lookup(const std::string& word) { return story_ ? lookupIn(word, rw(0x08)) : 0; }

void Machine::tokenise(uint32_t textAddr, uint32_t parse, uint32_t dict, bool flag) {
  if (dict == 0) dict = rw(0x08);
  std::string text;
  int off;
  if (v_ <= 4) {
    for (int i = 1; i < 256; i++) {
      const uint8_t c = rb(textAddr + i);
      if (c == 0) break;
      text.push_back(static_cast<char>(c));
    }
    off = 0;
  } else {
    const int n = rb(textAddr + 1);
    for (int i = 0; i < n; i++) text.push_back(static_cast<char>(rb(textAddr + 2 + i)));
    off = 1;
  }
  std::vector<std::pair<std::string, int>> words;
  splitWords(text, dict, words);
  const int max = rb(parse);
  const int count = static_cast<int>(words.size()) < max ? static_cast<int>(words.size()) : max;
  wb(parse + 1, count);
  for (int k = 0; k < count; k++) {
    const uint32_t d = lookupIn(words[k].first, dict);
    const uint32_t e = parse + 2 + k * 4;
    if (flag && d == 0) continue;
    ww(e, d);
    wb(e + 2, words[k].first.size());
    wb(e + 3, words[k].second + off);
  }
}

// The line typed, into the pending read's text buffer, then tokenised.
void Machine::sendLine(const std::string& raw) {
  if (!reading_ || readingChar_) return;
  reading_ = false;
  std::string line;
  for (char c : raw) {
    if (c >= 32 && c <= 126) line.push_back(c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c);
  }
  size_t a = 0;
  while (a < line.size() && line[a] == ' ') a++;
  size_t b = line.size();
  while (b > a && line[b - 1] == ' ') b--;
  line = line.substr(a, b - a);
  const int maxlen = rb(readText_);
  if (v_ <= 4) {
    const int room = maxlen - 1 > 0 ? maxlen - 1 : 0;
    if (static_cast<int>(line.size()) > room) line.resize(room);
    for (size_t i = 0; i < line.size(); i++) wb(readText_ + 1 + i, static_cast<uint8_t>(line[i]));
    wb(readText_ + line.size() + 1, 0);
  } else {
    if (static_cast<int>(line.size()) > maxlen) line.resize(maxlen);
    for (size_t i = 0; i < line.size(); i++) wb(readText_ + 2 + i, static_cast<uint8_t>(line[i]));
    wb(readText_ + 1, line.size());
  }
  lower_ += line + "\n";
  if (readParse_ != 0) tokenise(readText_, readParse_, 0, false);
  if (v_ >= 5) storeResult(13);
}

void Machine::sendChar(uint8_t c) {
  if (!reading_ || !readingChar_) return;
  reading_ = false;
  readingChar_ = false;
  storeResult(c);
}

// ------------------------------------------------------------------ save and restore

// Dynamic memory, the stack and the frames, with the story's release and serial so a save
// only goes back into its own game.
bool Machine::snapshot(std::vector<uint8_t>& out) const {
  out.clear();
  putU32(out, SNAPSHOT_MAGIC);
  for (int i = 0x02; i < 0x04; i++) out.push_back(story_[i]);
  for (int i = 0x12; i < 0x18; i++) out.push_back(story_[i]);
  // At a read, the read again, so a restored game asks for its line; else (the game's own
  // SAVE) the pc after the instruction, where its branch or store byte waits.
  putU32(out, reading_ ? readStart_ : pc_);
  putU32(out, static_cast<uint32_t>(stack_.size()));
  putU16(out, static_cast<uint32_t>(frames_.size()));
  for (uint16_t x : stack_) putU16(out, x);
  for (const Frame& f : frames_) {
    putU32(out, f.ret);
    putU16(out, static_cast<uint16_t>(f.store));
    out.push_back(f.nargs);
    putU32(out, f.base);
    out.push_back(f.nlocals);
    for (int i = 0; i < f.nlocals; i++) putU16(out, f.locals[i]);
  }
  putU32(out, staticBase_);
  out.insert(out.end(), dyn_.begin(), dyn_.end());
  putU16(out, static_cast<uint32_t>(upperH_));
  return true;
}

bool Machine::restore(const uint8_t* data, size_t size) {
  Reader r{data, size};
  if (r.u32() != SNAPSHOT_MAGIC) return false;
  for (int i = 0x02; i < 0x04; i++) {
    if (r.u8() != story_[i]) return false;
  }
  for (int i = 0x12; i < 0x18; i++) {
    if (r.u8() != story_[i]) return false;
  }
  const uint32_t pc = r.u32();
  const uint32_t sp = r.u32();
  const uint32_t nf = r.u16();
  if (!r.ok || pc >= storySize_ || sp > kMaxStack || nf > kMaxFrames) return false;
  std::vector<uint16_t> stack(sp);
  for (uint32_t i = 0; i < sp; i++) stack[i] = static_cast<uint16_t>(r.u16());
  std::vector<Frame> frames(nf);
  uint32_t lastBase = 0;
  for (Frame& f : frames) {
    f.ret = r.u32();
    f.store = static_cast<int16_t>(r.u16());
    f.nargs = static_cast<uint8_t>(r.u8());
    f.base = r.u32();
    f.nlocals = static_cast<uint8_t>(r.u8());
    if (f.nlocals > 15 || f.base > sp || f.base < lastBase || f.ret >= storySize_ || f.store < -1 || f.store > 255) return false;
    lastBase = f.base;
    for (int i = 0; i < f.nlocals; i++) f.locals[i] = static_cast<uint16_t>(r.u16());
  }
  // The memory, then the upper window's height, and nothing after: anything else is damaged.
  if (r.u32() != staticBase_ || !r.ok || r.at + staticBase_ + 2 != size) return false;
  // The header's flags 2 and the interpreter's own fields stay as they are now.
  const uint8_t keep10 = dyn_[0x10];
  const uint8_t keep11 = dyn_[0x11];
  dyn_.assign(data + r.at, data + r.at + staticBase_);
  r.at += staticBase_;
  dyn_[0x10] = keep10;
  dyn_[0x11] = keep11;
  setHeader();
  pc_ = pc;
  stack_.swap(stack);
  frames_.swap(frames);
  split(static_cast<int>(r.u16()));
  reading_ = false;
  readingChar_ = false;
  done_ = false;
  failed_ = false;
  error_.clear();
  return true;
}

void Machine::doSave() {
  bool ok = false;
  if (onSave) {
    // Not at a read, so the snapshot keeps the pc after the instruction, and a restore goes
    // on to its branch or store.
    std::vector<uint8_t> state;
    snapshot(state);
    ok = onSave(state);
  }
  if (v_ <= 3) {
    branch(ok);
  } else {
    storeResult(ok ? 1 : 0);
  }
}

void Machine::doRestore() {
  std::vector<uint8_t> state;
  if (onRestore && onRestore(state) && restore(state.data(), state.size())) {
    if (v_ <= 3) {
      branch(true);
    } else {
      storeResult(2);
    }
    return;
  }
  if (v_ <= 3) {
    branch(false);
  } else {
    storeResult(0);
  }
}

// random: negative seeds a predictable sequence, 0 goes back to real randomness.
uint16_t Machine::random(int16_t r) {
  if (r < 0) {
    rngSeed_ = -r;
    rngN_ = 0;
    rngX_ = static_cast<uint32_t>(-r);
    return 0;
  }
  if (r == 0) {
    rngSeed_ = 0;
    return 0;
  }
  if (rngSeed_ > 0 && rngSeed_ < 1000) {
    rngN_ = rngN_ % rngSeed_ + 1;
    return static_cast<uint16_t>((rngN_ - 1) % r + 1);
  }
  if (rngSeed_ > 0) {
    rngX_ = (rngX_ * 1103515245u + 12345u) & 0x7FFFFFFFu;
    return static_cast<uint16_t>(rngX_ % r + 1);
  }
  rngState_ ^= rngState_ << 13;
  rngState_ ^= rngState_ >> 17;
  rngState_ ^= rngState_ << 5;
  return static_cast<uint16_t>(rngState_ % r + 1);
}

// ------------------------------------------------------------------ instructions

void Machine::op2(int n, const uint16_t* a, int count) {
  if (count < 2 && n != 1) {
    fail("A 2OP WITH TOO FEW OPERANDS");
    return;
  }
  uint32_t data = 0;
  int len = 0;
  switch (n) {
    case 1: {
      bool any = false;
      for (int i = 1; i < count; i++) any = any || a[i] == a[0];
      branch(any);
      break;
    }
    case 2: branch(sgn(a[0]) < sgn(a[1])); break;
    case 3: branch(sgn(a[0]) > sgn(a[1])); break;
    case 4: {
      const int16_t x = sgn(getvar(a[0], true) - 1);
      setvar(a[0], static_cast<uint16_t>(x), true);
      branch(x < sgn(a[1]));
      break;
    }
    case 5: {
      const int16_t x = sgn(getvar(a[0], true) + 1);
      setvar(a[0], static_cast<uint16_t>(x), true);
      branch(x > sgn(a[1]));
      break;
    }
    case 6: branch(parent(a[0]) == a[1]); break;
    case 7: branch((a[0] & a[1]) == a[1]); break;
    case 8: storeResult(a[0] | a[1]); break;
    case 9: storeResult(a[0] & a[1]); break;
    case 10: branch(a[0] != 0 && attr(a[0], a[1])); break;
    case 11:
      if (a[0] != 0) setAttr(a[0], a[1], true);
      break;
    case 12:
      if (a[0] != 0) setAttr(a[0], a[1], false);
      break;
    case 13: setvar(a[0], a[1], true); break;
    case 14:
      if (a[0] != 0 && a[1] != 0) insertObj(a[0], a[1]);
      break;
    case 15: storeResult(rw((a[0] + 2 * sgn(a[1])) & 0xFFFF)); break;
    case 16: storeResult(rb((a[0] + sgn(a[1])) & 0xFFFF)); break;
    case 17:
      if (!findProp(a[0], a[1], data, len)) {
        storeResult(rw(rw(0x0A) + 2 * (a[1] - 1)));
      } else if (len == 1) {
        storeResult(rb(data));
      } else {
        storeResult(rw(data));
      }
      break;
    case 18: storeResult(a[0] != 0 && findProp(a[0], a[1], data, len) ? data : 0); break;
    case 19: {
      uint32_t p = firstProp(a[0]);
      if (a[1] != 0) {
        for (int guard = 0;; guard++) {
          const int num = propAt(p, data, len);
          if (num == 0 || guard > 255) {
            fail("GET_NEXT_PROP: NO SUCH PROPERTY");
            return;
          }
          p = data + len;
          if (num == a[1]) break;
        }
      }
      storeResult(propAt(p, data, len));
      break;
    }
    case 20: storeResult(static_cast<uint16_t>(sgn(a[0]) + sgn(a[1]))); break;
    case 21: storeResult(static_cast<uint16_t>(sgn(a[0]) - sgn(a[1]))); break;
    case 22: storeResult(static_cast<uint16_t>(sgn(a[0]) * sgn(a[1]))); break;
    case 23:
      if (a[1] == 0) {
        fail("DIVISION BY ZERO");
        return;
      }
      storeResult(static_cast<uint16_t>(sgn(a[0]) / sgn(a[1])));
      break;
    case 24:
      if (a[1] == 0) {
        fail("DIVISION BY ZERO");
        return;
      }
      storeResult(static_cast<uint16_t>(sgn(a[0]) % sgn(a[1])));
      break;
    case 25: {
      const int s = rb(pc_);
      pc_++;
      call(a[0], a + 1, 1, s);
      break;
    }
    case 26: call(a[0], a + 1, 1, -1); break;
    case 27: break;  // set_colour
    case 28:
      while (frames_.size() > a[1]) frames_.pop_back();
      ret(a[0]);
      break;
    default: fail("UNKNOWN 2OP");
  }
}

void Machine::op1(int n, uint16_t a) {
  switch (n) {
    case 0: branch(a == 0); break;
    case 1: {
      const uint32_t s = a != 0 ? sibling(a) : 0;
      storeResult(s);
      branch(s != 0);
      break;
    }
    case 2: {
      const uint32_t c = a != 0 ? child(a) : 0;
      storeResult(c);
      branch(c != 0);
      break;
    }
    case 3: storeResult(a != 0 ? parent(a) : 0); break;
    case 4: storeResult(propLen(a)); break;
    case 5: setvar(a, static_cast<uint16_t>(sgn(getvar(a, true)) + 1), true); break;
    case 6: setvar(a, static_cast<uint16_t>(sgn(getvar(a, true)) - 1), true); break;
    case 7: print(decode(a)); break;
    case 8: {
      const int s = rb(pc_);
      pc_++;
      call(a, nullptr, 0, s);
      break;
    }
    case 9:
      if (a != 0) removeObj(a);
      break;
    case 10: print(objName(a)); break;
    case 11: ret(a); break;
    case 12: pc_ = static_cast<uint32_t>(static_cast<int32_t>(pc_) + sgn(a) - 2); break;
    case 13: print(decode(unpackAddr(a, true))); break;
    case 14: storeResult(getvar(a, true)); break;
    case 15:
      if (v_ <= 4) {
        storeResult(static_cast<uint16_t>(~a));
      } else {
        call(a, nullptr, 0, -1);
      }
      break;
    default: fail("UNKNOWN 1OP");
  }
}

void Machine::op0(int n) {
  switch (n) {
    case 0: ret(1); break;
    case 1: ret(0); break;
    case 2: print(decode(pc_, &pc_)); break;
    case 3:
      print(decode(pc_, &pc_));
      printChar(13);
      ret(1);
      break;
    case 4: break;
    case 5: doSave(); break;
    case 6: doRestore(); break;
    case 7: {
      const uint32_t seedKeep = rngState_;
      reset();
      rngState_ = seedKeep;
      break;
    }
    case 8: ret(pop()); break;
    case 9:
      if (v_ <= 4) {
        pop();
      } else {
        storeResult(static_cast<uint32_t>(frames_.size()));
      }
      break;
    case 10: done_ = true; break;
    case 11: printChar(13); break;
    case 12: break;  // show_status: the screen draws it every time
    case 13: branch(true); break;  // verify
    case 15: branch(true); break;  // piracy
    default: fail("UNKNOWN 0OP");
  }
}

void Machine::opVar(int n, const uint16_t* a, int count) {
  auto arg = [&](int i, uint16_t def) { return i < count ? a[i] : def; };
  switch (n) {
    case 0:
    case 12: {
      const int s = rb(pc_);
      pc_++;
      call(arg(0, 0), a + 1, count > 0 ? count - 1 : 0, s);
      break;
    }
    case 1: ww((arg(0, 0) + 2 * sgn(arg(1, 0))) & 0xFFFF, arg(2, 0)); break;
    case 2: wb((arg(0, 0) + sgn(arg(1, 0))) & 0xFFFF, arg(2, 0)); break;
    case 3: {
      uint32_t data = 0;
      int len = 0;
      if (!findProp(arg(0, 0), arg(1, 0), data, len)) {
        fail("PUT_PROP: NO SUCH PROPERTY");
        return;
      }
      if (len == 1) {
        wb(data, arg(2, 0));
      } else {
        ww(data, arg(2, 0));
      }
      break;
    }
    case 4:
      reading_ = true;
      readingChar_ = false;
      readText_ = arg(0, 0);
      readParse_ = arg(1, 0);
      readStart_ = start_;
      break;
    case 5:
      if (arg(0, 0) != 0) printChar(static_cast<uint8_t>(arg(0, 0)));
      break;
    case 6: {
      char text[8];
      std::snprintf(text, sizeof(text), "%d", sgn(arg(0, 0)));
      print(text);
      break;
    }
    case 7: storeResult(random(sgn(arg(0, 0)))); break;
    case 8: push(arg(0, 0)); break;
    case 9: {
      const uint16_t x = pop();
      setvar(arg(0, 0), x, true);
      break;
    }
    case 10: split(arg(0, 0)); break;
    case 11:
      window_ = arg(0, 0);
      if (window_ == 1) {
        curRow_ = 1;
        curCol_ = 1;
      }
      break;
    case 13: {
      const int16_t w = sgn(arg(0, 0));
      if (w == -1) {
        split(0);
        eraseUpper();
        window_ = 0;
      } else if (w == -2 || w == 1) {
        eraseUpper();
      }
      break;
    }
    case 14: break;  // erase_line
    case 15:
      if (window_ == 1) {
        curRow_ = sgn(arg(0, 1));
        curCol_ = sgn(arg(1, 1));
      }
      break;
    case 16:
      ww(arg(0, 0), curRow_);
      ww(arg(0, 0) + 2, curCol_);
      break;
    case 17: break;  // set_text_style
    case 18: break;  // buffer_mode
    case 19: {
      const int16_t s = sgn(arg(0, 0));
      if (s == 3) {
        if (out3_.size() >= 16) {
          fail("OUTPUT STREAM 3 NESTED TOO DEEP");
          return;
        }
        out3_.push_back({arg(1, 0), 0});
      } else if (s == -3 && !out3_.empty()) {
        const Stream3 t = out3_.back();
        out3_.pop_back();
        ww(t.addr, t.n);
      }
      break;
    }
    case 20: break;  // input_stream
    case 21: break;  // sound_effect
    case 22:
      reading_ = true;
      readingChar_ = true;
      readStart_ = start_;
      break;
    case 23: {
      const uint16_t x = arg(0, 0);
      const uint32_t t = arg(1, 0);
      const int len = arg(2, 0);
      const uint16_t form = arg(3, 0x82);
      const int size = form & 0x7F;
      const bool words = (form & 0x80) != 0;
      for (int i = 0; i < len; i++) {
        const uint32_t e = t + static_cast<uint32_t>(i) * size;
        if ((words ? rw(e) : rb(e)) == x) {
          storeResult(e);
          branch(true);
          return;
        }
      }
      storeResult(0);
      branch(false);
      break;
    }
    case 24: storeResult(static_cast<uint16_t>(~arg(0, 0))); break;
    case 25:
    case 26: call(arg(0, 0), a + 1, count > 0 ? count - 1 : 0, -1); break;
    case 27: tokenise(arg(0, 0), arg(1, 0), arg(2, 0), arg(3, 0) != 0); break;
    case 28: {
      std::string word;
      for (int i = 0; i < arg(1, 0); i++) word.push_back(static_cast<char>(rb(arg(0, 0) + arg(2, 0) + i)));
      const std::vector<uint16_t> enc = encode(word);
      for (size_t k = 0; k < enc.size(); k++) ww(arg(3, 0) + 2 * k, enc[k]);
      break;
    }
    case 29: {
      const uint32_t from = arg(0, 0);
      const uint32_t to = arg(1, 0);
      const int16_t size = sgn(arg(2, 0));
      const int n = size < 0 ? -size : size;
      if (to == 0) {
        for (int i = 0; i < n; i++) wb(from + i, 0);
      } else if (size < 0 || from > to) {
        for (int i = 0; i < n; i++) wb(to + i, rb(from + i));
      } else {
        for (int i = n - 1; i >= 0; i--) wb(to + i, rb(from + i));
      }
      break;
    }
    case 30: {
      const uint32_t addr = arg(0, 0);
      const int w = arg(1, 0);
      const int h = arg(2, 1);
      const int skip = arg(3, 0);
      for (int r = 0; r < h; r++) {
        for (int c = 0; c < w; c++) printChar(rb(addr + r * (w + skip) + c));
        if (r < h - 1) printChar(13);
      }
      break;
    }
    case 31: branch(!frames_.empty() && arg(0, 0) <= frames_.back().nargs); break;
    default: fail("UNKNOWN VAR OPCODE");
  }
}

void Machine::opExt(int n, const uint16_t* a, int count) {
  auto arg = [&](int i) { return i < count ? a[i] : static_cast<uint16_t>(0); };
  switch (n) {
    case 0: doSave(); break;
    case 1: doRestore(); break;
    case 2: {
      const int16_t s = sgn(arg(1));
      storeResult(s >= 0 ? static_cast<uint32_t>(arg(0)) << s : static_cast<uint32_t>(arg(0)) >> -s);
      break;
    }
    case 3: {
      const int16_t s = sgn(arg(1));
      const int16_t x = sgn(arg(0));
      storeResult(static_cast<uint16_t>(s >= 0 ? x * (1 << s) : x >> -s));
      break;
    }
    case 4: storeResult(arg(0) == 1 ? 1 : 0); break;  // set_font
    case 9: storeResult(0xFFFF); break;                 // save_undo: not available
    case 10: storeResult(0); break;                     // restore_undo
    case 11: {
      const uint16_t cp = arg(0);
      uint8_t z = '?';
      if (cp >= 32 && cp <= 126) z = static_cast<uint8_t>(cp);
      for (int k = 0; k < 69; k++) {
        if (UNI[k] == cp) z = static_cast<uint8_t>(155 + k);
      }
      printChar(z);
      break;
    }
    case 12: storeResult(arg(0) >= 32 && arg(0) <= 126 ? 3 : 0); break;
    case 13: break;  // set_true_colour
    default: fail("UNKNOWN EXTENDED OPCODE");
  }
}

uint16_t Machine::operand(int type) {
  const uint32_t pc = pc_;
  if (type == 0) {
    pc_ = pc + 2;
    return rw(pc);
  }
  if (type == 1) {
    pc_ = pc + 1;
    return rb(pc);
  }
  pc_ = pc + 1;
  return getvar(rb(pc));
}

int Machine::varOperands(int bytes, uint16_t* out) {
  uint8_t types[2] = {};
  for (int k = 0; k < bytes; k++) types[k] = rb(pc_++);
  int count = 0;
  for (int k = 0; k < bytes; k++) {
    for (int s = 6; s >= 0; s -= 2) {
      const int t = (types[k] >> s) & 3;
      if (t == 3) return count;
      out[count++] = operand(t);
    }
  }
  return count;
}

// One instruction. False once the game waits for input, ends or fails.
bool Machine::step() {
  start_ = pc_;
  if (pc_ >= storySize_) {
    fail("THE PROGRAM RAN OFF THE END OF THE STORY");
    return false;
  }
  const uint8_t op = rb(pc_);
  pc_++;
  uint16_t a[8] = {};
  if (op == 0xBE && v_ >= 5) {
    const int n = rb(pc_);
    pc_++;
    const int count = varOperands(1, a);
    opExt(n, a, count);
  } else if (op >= 0xC0) {
    const int n = op & 0x1F;
    const bool two = op >= 0xE0 && (n == 12 || n == 26);
    const int count = varOperands(two ? 2 : 1, a);
    if (op < 0xE0) {
      op2(n, a, count);
    } else {
      opVar(n, a, count);
    }
  } else if (op >= 0x80) {
    const int t = (op >> 4) & 3;
    const int n = op & 0x0F;
    if (t == 3) {
      op0(n);
    } else {
      op1(n, operand(t));
    }
  } else {
    const int n = op & 0x1F;
    a[0] = operand((op & 0x40) ? 2 : 1);
    a[1] = operand((op & 0x20) ? 2 : 1);
    op2(n, a, 2);
  }
  return !failed_ && !done_ && !reading_;
}

Result Machine::run(int limit) {
  if (!story_) return Result::Error;
  if (failed_) return Result::Error;
  if (done_) return Result::Quit;
  if (reading_) return readingChar_ ? Result::Char : Result::Input;
  for (int i = 0; i < limit; i++) {
    if (!step()) {
      if (failed_) return Result::Error;
      if (done_) return Result::Quit;
      return readingChar_ ? Result::Char : Result::Input;
    }
  }
  return Result::More;
}

}  // namespace zmachine
