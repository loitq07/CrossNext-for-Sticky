#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

// The game screen's text, translated from the Lua of touch-licorice-apps' apps/textadv
// (src/main.lua, MIT, LICENSE-licorice.txt), with no screen or card, so it can be tested off the
// device: the transcript and its paging, the word chips, and the command being built.
namespace textadv {

// How wide a run of text is on the screen, in pixels: the screen passes the font's measure.
using Measure = std::function<int(const std::string&)>;

// The transcript, wrapped to the screen's width. The game's prompt and the command typed after
// it share one line, so text that doesn't end with a new line stays open for the next.
class Transcript {
 public:
  static constexpr size_t kMaxLines = 400;

  void clear() {
    lines_.clear();
    joins_.clear();
    open_ = false;
    top_ = 0;
  }

  // Adds printed text, wrapping each paragraph to `width`.
  void add(const std::string& raw, const Measure& measure, int width) {
    std::string s;
    for (char c : raw) {
      if (c != '\r') s.push_back(c);
    }
    if (s.empty()) return;
    if (open_ && !lines_.empty()) {
      s = lines_.back() + s;
      lines_.pop_back();
      joins_.pop_back();
    }
    open_ = s.back() != '\n';
    if (!open_) s.pop_back();
    size_t start = 0;
    while (true) {
      const size_t end = s.find('\n', start);
      wrap(s.substr(start, end == std::string::npos ? std::string::npos : end - start), measure, width);
      if (end == std::string::npos) break;
      start = end + 1;
    }
    while (lines_.size() > kMaxLines) {
      lines_.erase(lines_.begin());
      joins_.erase(joins_.begin());
      if (top_ > 0) top_--;
    }
  }

  // Rewraps everything, for a new text size: lines the wrapper broke are joined back into their
  // paragraphs first, with the space it took out.
  void rewrap(const Measure& measure, int width) {
    std::string all;
    for (size_t i = 0; i < lines_.size(); i++) {
      all += lines_[i];
      if (joins_[i] == Join::Space) {
        all += " ";
      } else if (joins_[i] == Join::Paragraph && (i + 1 < lines_.size() || !open_)) {
        all += "\n";
      }
    }
    const bool wasOpen = open_;
    lines_.clear();
    joins_.clear();
    open_ = false;
    add(all, measure, width);
    if (wasOpen) open_ = true;
  }

  // After a reply: show it from its start, so the player reads down, unless it fits with
  // what's above it. `firstNew` is the line count before the reply was added.
  void showReply(size_t firstNew, int pageLines) {
    const size_t n = static_cast<size_t>(pageLines);
    if (lines_.size() - firstNew >= n) {
      top_ = firstNew;
    } else {
      top_ = lines_.size() > n ? lines_.size() - n : 0;
    }
  }

  void showEnd(int pageLines) {
    const size_t n = static_cast<size_t>(pageLines);
    top_ = lines_.size() > n ? lines_.size() - n : 0;
  }

  // A page back or on; false if already at that end.
  bool page(int delta, int pageLines) {
    const size_t n = static_cast<size_t>(pageLines);
    const size_t last = lines_.size() > n ? lines_.size() - n : 0;
    long next = static_cast<long>(top_) + delta * static_cast<long>(n);
    if (next < 0) next = 0;
    if (next > static_cast<long>(last)) next = static_cast<long>(last);
    if (static_cast<size_t>(next) == top_) return false;
    top_ = static_cast<size_t>(next);
    return true;
  }

  const std::vector<std::string>& lines() const { return lines_; }
  size_t top() const { return top_; }
  size_t size() const { return lines_.size(); }

  // The last `count` lines, for the autosave, and back again.
  std::string tail(size_t count) const {
    std::string out;
    const size_t from = lines_.size() > count ? lines_.size() - count : 0;
    for (size_t i = from; i < lines_.size(); i++) {
      out += lines_[i];
      if (i + 1 < lines_.size()) out += "\n";
    }
    return out;
  }
  // A saved tail keeps its lines as they were wrapped: each is its own paragraph from then on.
  void setLines(const std::string& text) {
    clear();
    size_t start = 0;
    while (start <= text.size() && !text.empty()) {
      const size_t end = text.find('\n', start);
      lines_.push_back(text.substr(start, end == std::string::npos ? std::string::npos : end - start));
      joins_.push_back(Join::Paragraph);
      if (end == std::string::npos) break;
      start = end + 1;
    }
  }

 private:
  // How a line goes on into the next: a new paragraph, or one the wrapper broke at a space or
  // mid-word.
  enum class Join : uint8_t { Paragraph, Space, None };

  void push(const std::string& line, Join join) {
    lines_.push_back(line);
    joins_.push_back(join);
  }

  // One paragraph into lines: at spaces where it can, mid-word only for a word too long.
  void wrap(const std::string& para, const Measure& measure, int width) {
    if (para.empty()) {
      push(std::string(), Join::Paragraph);
      return;
    }
    size_t at = 0;
    while (at < para.size()) {
      size_t fit = at;
      size_t lastSpace = std::string::npos;
      while (fit < para.size() && measure(para.substr(at, fit - at + 1)) <= width) {
        if (para[fit] == ' ') lastSpace = fit;
        fit++;
      }
      if (fit >= para.size()) {
        push(para.substr(at), Join::Paragraph);
        return;
      }
      // A space just past the line breaks there; else the last space in it, else mid-word.
      size_t cut;
      if (para[fit] == ' ') {
        cut = fit;
      } else if (lastSpace != std::string::npos && lastSpace > at) {
        cut = lastSpace;
      } else {
        cut = fit > at ? fit : at + 1;
      }
      const bool atSpace = para[cut] == ' ';
      push(para.substr(at, cut - at), atSpace ? Join::Space : Join::None);
      at = cut;
      while (at < para.size() && para[at] == ' ') at++;
    }
  }

  std::vector<std::string> lines_;
  std::vector<Join> joins_;
  bool open_ = false;
  size_t top_ = 0;
};

// The verbs every chip page ends with (VERBS), and the little words that never become chips.
constexpr const char* kVerbs[] = {"take", "drop", "examine", "open", "close", "read", "put", "in",
                                  "on",   "with", "turn",    "push", "pull",  "give", "attack", "eat",
                                  "drink", "wear", "unlock", "light", "climb", "enter", "all"};
constexpr const char* kSkip[] = {"a",     "an",   "the",   "and",  "of",    "to",    "is",   "it",    "you",
                                 "your",  "are",  "there", "here", "this",  "that",  "with", "from",  "into",
                                 "onto",  "some", "for",   "but",  "what",  "which", "where", "who", "why",
                                 "how",   "can",  "does"};
constexpr size_t kMaxFresh = 24;

inline bool listed(const char* const* list, size_t count, const std::string& w) {
  for (size_t i = 0; i < count; i++) {
    if (w == list[i]) return true;
  }
  return false;
}

// The chips for the last reply (make_chips): the game's own words in it, newest first, then the
// words of earlier replies up to 24, so a short reply such as "What do you want to put?" keeps
// the room's words; then the verbs. `known` holds the earlier words and is updated.
inline std::vector<std::string> makeChips(const std::string& reply, std::vector<std::string>& known,
                                          const std::function<bool(const std::string&)>& inDictionary) {
  const size_t verbCount = sizeof(kVerbs) / sizeof(kVerbs[0]);
  const size_t skipCount = sizeof(kSkip) / sizeof(kSkip[0]);
  std::vector<std::string> seen;
  auto wasSeen = [&](const std::string& w) {
    for (const std::string& s : seen) {
      if (s == w) return true;
    }
    return listed(kVerbs, verbCount, w);
  };
  std::vector<std::string> fresh;
  std::string word;
  for (size_t i = 0; i <= reply.size(); i++) {
    const char c = i < reply.size() ? reply[i] : ' ';
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
      word.push_back(c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c);
      continue;
    }
    if (word.size() >= 3 && !wasSeen(word) && !listed(kSkip, skipCount, word)) {
      seen.push_back(word);
      if (inDictionary(word)) fresh.push_back(word);
    }
    word.clear();
  }
  for (const std::string& w : known) {
    if (fresh.size() >= kMaxFresh) break;
    if (!wasSeen(w)) {
      seen.push_back(w);
      fresh.push_back(w);
    }
  }
  known = fresh;
  std::vector<std::string> chips = fresh;
  for (size_t i = 0; i < verbCount; i++) chips.emplace_back(kVerbs[i]);
  return chips;
}

// The reply without the command echoed at its start: the chips are the game's words, not the
// player's.
inline std::string withoutEcho(const std::string& out) {
  const size_t nl = out.find('\n');
  return nl == std::string::npos ? std::string() : out.substr(nl + 1);
}

// The command being built from chips.
inline void addWord(std::string& cmd, const std::string& word, size_t maxLength) {
  const std::string next = cmd.empty() ? word : cmd + " " + word;
  if (next.size() <= maxLength) cmd = next;
}

// DEL: the last word goes.
inline bool deleteWord(std::string& cmd) {
  if (cmd.empty()) return false;
  size_t end = cmd.size();
  while (end > 0 && cmd[end - 1] == ' ') end--;
  while (end > 0 && cmd[end - 1] != ' ') end--;
  while (end > 0 && cmd[end - 1] == ' ') end--;
  cmd.resize(end);
  return true;
}

// The name a game's saves are kept under: a hash of its file name, as the source's key_of.
inline uint32_t keyOf(const std::string& name) {
  uint32_t h = 5381;
  for (unsigned char c : name) h = ((h * 33) + c) & 0x7fffffff;
  return h;
}

// A card file the interpreter can try: .z3, .z4, .z5, .z7 or .z8, in any case. The name without
// its extension goes in `base`.
inline bool isStoryFile(const std::string& file, std::string& base) {
  if (file.size() < 4 || file[0] == '.') return false;
  const size_t dot = file.size() - 3;
  if (file[dot] != '.' || (file[dot + 1] != 'z' && file[dot + 1] != 'Z')) return false;
  const char v = file[dot + 2];
  if (v != '3' && v != '4' && v != '5' && v != '7' && v != '8') return false;
  base = file.substr(0, dot);
  return true;
}

// The settings save: the text size and the last game played.
constexpr uint8_t SETTINGS_VERSION = 1;
constexpr size_t kMaxFileName = 96;

inline size_t packSettings(bool small, const std::string& last, uint8_t* out) {
  size_t at = 0;
  out[at++] = small ? 1 : 0;
  const size_t n = last.size() < kMaxFileName ? last.size() : kMaxFileName;
  out[at++] = static_cast<uint8_t>(n);
  std::memcpy(out + at, last.data(), n);
  return at + n;
}

inline bool unpackSettings(uint8_t version, const uint8_t* in, size_t size, bool& small, std::string& last) {
  if (version != SETTINGS_VERSION || size < 2 || in[0] > 1 || in[1] > kMaxFileName || size != 2u + in[1]) return false;
  for (size_t i = 0; i < in[1]; i++) {
    if (in[2 + i] < 32 || in[2 + i] > 126) return false;
  }
  small = in[0] == 1;
  last.assign(reinterpret_cast<const char*>(in + 2), in[1]);
  return true;
}

}  // namespace textadv
