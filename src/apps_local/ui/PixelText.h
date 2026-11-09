#pragma once

#include <cstddef>
#include <cstring>

namespace pixelui {
// Use translated mixed case as supplied; soften display-only all-caps labels.
enum class LabelCase { Sentence, Preserve };

inline bool isUtf8Lower(const unsigned char* p, size_t bytes) {
  if (bytes == 1) return *p >= 'a' && *p <= 'z';
  if (bytes == 2) {
    if (*p == 0xC3 && p[1] >= 0xA0 && p[1] <= 0xBE && p[1] != 0xB7) return true;
    if (*p == 0xC4 && (p[1] == 0x83 || p[1] == 0x91 || p[1] == 0xA9)) return true;
    if (*p == 0xC5 && p[1] == 0xA9) return true;
    if (*p == 0xC6 && (p[1] == 0xA1 || p[1] == 0xB0)) return true;
  } else if (bytes == 3) {
    if (*p == 0xE1 && (p[1] == 0xBA || p[1] == 0xBB) && (p[2] & 1)) return true;
  }
  return false;
}

inline void utf8ToLower(char* out, size_t bytes) {
  auto* p = reinterpret_cast<unsigned char*>(out);
  if (bytes == 1) {
    if (*p >= 'A' && *p <= 'Z') *p = static_cast<unsigned char>(*p + 'a' - 'A');
  } else if (bytes == 2) {
    if (*p == 0xC3 && p[1] >= 0x80 && p[1] <= 0x9E && p[1] != 0x97) {
      p[1] = static_cast<unsigned char>(p[1] + 0x20);
    } else if (*p == 0xC4 && (p[1] == 0x82 || p[1] == 0x90 || p[1] == 0xA8)) {
      p[1] = static_cast<unsigned char>(p[1] + 1);
    } else if (*p == 0xC5 && p[1] == 0xA8) {
      p[1] = static_cast<unsigned char>(p[1] + 1);
    } else if (*p == 0xC6) {
      if (p[1] == 0xA0) p[1] = 0xA1;
      else if (p[1] == 0xAF) p[1] = 0xB0;
    }
  } else if (bytes == 3) {
    if (*p == 0xE1 && (p[1] == 0xBA || p[1] == 0xBB) && ((p[2] & 1) == 0)) {
      p[2] = static_cast<unsigned char>(p[2] + 1);
    }
  }
}

inline void utf8ToUpper(char* out, size_t bytes) {
  auto* p = reinterpret_cast<unsigned char*>(out);
  if (bytes == 1) {
    if (*p >= 'a' && *p <= 'z') *p = static_cast<unsigned char>(*p - 'a' + 'A');
  } else if (bytes == 2) {
    if (*p == 0xC3 && p[1] >= 0xA0 && p[1] <= 0xBE && p[1] != 0xB7) {
      p[1] = static_cast<unsigned char>(p[1] - 0x20);
    } else if (*p == 0xC4 && (p[1] == 0x83 || p[1] == 0x91 || p[1] == 0xA9)) {
      p[1] = static_cast<unsigned char>(p[1] - 1);
    } else if (*p == 0xC5 && p[1] == 0xA9) {
      p[1] = static_cast<unsigned char>(p[1] - 1);
    } else if (*p == 0xC6) {
      if (p[1] == 0xA1) p[1] = 0xA0;
      else if (p[1] == 0xB0) p[1] = 0xAF;
    }
  } else if (bytes == 3) {
    if (*p == 0xE1 && (p[1] == 0xBA || p[1] == 0xBB) && ((p[2] & 1) == 1)) {
      p[2] = static_cast<unsigned char>(p[2] - 1);
    }
  }
}

inline bool copyNatural(char* out, size_t size, const char* text, LabelCase casing = LabelCase::Preserve) {
  if (!size) return false;
  if (!text) text = "";
  bool mixed = false;
  for (const unsigned char* p = reinterpret_cast<const unsigned char*>(text); *p; ) {
    const size_t bytes = *p < 0x80 ? 1 : *p < 0xE0 ? 2 : *p < 0xF0 ? 3 : 4;
    if (isUtf8Lower(p, bytes)) {
      mixed = true;
      break;
    }
    p += bytes;
  }
  size_t n = 0;
  bool first = true, unicode = false;
  while (*text && n + 1 < size) {
    const auto ch = static_cast<unsigned char>(*text);
    const size_t bytes = ch < 0x80 ? 1 : ch < 0xE0 ? 2 : ch < 0xF0 ? 3 : 4;
    bool complete = n + bytes < size;
    for (size_t i = 1; complete && i < bytes; ++i)
      complete = text[i] && (static_cast<unsigned char>(text[i]) & 0xC0) == 0x80;
    if (!complete) break;
    std::memmove(out + n, text, bytes);
    if (casing == LabelCase::Sentence && !mixed && !first) {
      utf8ToLower(out + n, bytes);
    }
    if (ch != ' ' && ch != '\t') first = false;
    unicode |= bytes > 1;
    n += bytes;
    text += bytes;
  }
  out[n] = '\0';
  return unicode;
}

// Preserve complete UTF-8 codepoints and uppercase Latin-1 and Vietnamese letters.
inline bool copyCaps(char* out, const size_t size, const char* text) {
  if (!size) return false;
  if (!text) text = "";
  size_t n = 0;
  bool unicode = false;
  while (*text && n + 1 < size) {
    const auto ch = static_cast<unsigned char>(*text);
    const size_t bytes = ch < 0x80 ? 1 : (ch < 0xE0 ? 2 : (ch < 0xF0 ? 3 : 4));
    if (bytes > 1) {
      unicode = true;
      bool complete = true;
      for (size_t i = 1; i < bytes; ++i) {
        if (!text[i] || (static_cast<unsigned char>(text[i]) & 0xC0) != 0x80) {
          complete = false;
          break;
        }
      }
      if (!complete) {
        ++text;
        continue;
      }
    }
    if (n + bytes >= size) break;
    std::memmove(out + n, text, bytes);
    utf8ToUpper(out + n, bytes);
    n += bytes;
    text += bytes;
  }
  out[n] = '\0';
  return unicode;
}
}  // namespace pixelui
