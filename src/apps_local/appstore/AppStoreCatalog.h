#pragma once

#include <cstddef>
#include <cstdint>

enum class StoreCategory : uint8_t {
  All = 0,
  Games = 1,
  Apps = 2
};

struct StoreItem {
  const char* id;          // e.g. "dice"
  const char* name;        // e.g. "DICE"
  const char* viName;      // e.g. "Xúc xắc 3D"
  const char* desc;        // English description
  const char* viDesc;      // Vietnamese description
  StoreCategory category;  // Games or Apps
  const char* folder;      // "/games" or "/apps"
  const char* filename;    // "dice.elf"
  size_t sizeBytes;        // size in bytes (progress fallback when the server sends none)
  const char* sizeStr;     // "4.7 KB"
  // A built-in app shipped as a native module (its original source, see
  // tools_local/native_modules): fetched from modules/<firmware ABI>/ and
  // flagged for update when the card holds a build for another firmware.
  bool module = false;
  // FORK CHANGE: an upstream 2.0 interface language pack (/languages/*.cplang).
  // Published with the modules of each firmware build (its string keys change
  // with the firmware); needed, and refreshed after every firmware update, when
  // the saved interface language uses it. Requires module = true for the URL.
  bool languagePack = false;
};

// Only apps with no built-in copy are listed. A small host-API plugin named
// like a built-in would replace it on the shelf with a lesser version, so a
// built-in app appears here only once it ships as a native module.
inline const StoreItem kStoreItems[] = {
  {
    "2048", "2048", "2048",
    "Number tile puzzle", "Trò chơi ghép ô số 2048",
    StoreCategory::Games, "/games", "2048.elf", 94532, "92.3 KB", true
  },
  {
    "minesweeper", "MINESWEEPER", "Dò Mìn",
    "Classic grid minefield", "Trò chơi dò mìn kinh điển",
    StoreCategory::Games, "/games", "minesweeper.elf", 129060, "126.0 KB", true
  },
  {
    "picross", "PICROSS", "Picross",
    "Nonogram logic picture", "Tranh giải đố số Nonogram",
    StoreCategory::Games, "/games", "picross.elf", 173436, "169.4 KB", true
  },
  {
    "sudoku", "SUDOKU", "Sudoku",
    "Number logic grid puzzle", "Giải đố điền số Sudoku",
    StoreCategory::Games, "/games", "sudoku.elf", 128568, "125.6 KB", true
  },
  {
    "wordle", "WORDLE", "Đoán Từ",
    "5-letter word puzzle", "Trò chơi đoán từ 5 chữ cái",
    StoreCategory::Games, "/games", "wordle.elf", 266552, "260.3 KB", true
  },
  {
    "connectfour", "CONNECT FOUR", "Nối 4 Điểm",
    "Drop & connect 4 discs", "Cờ nối 4 điểm đối kháng",
    StoreCategory::Games, "/games", "connectfour.elf", 115960, "113.2 KB", true
  },
  {
    "checkers", "CHECKERS", "Cờ Đam",
    "Draughts board game", "Cờ đam quốc tế truyền thống",
    StoreCategory::Games, "/games", "checkers.elf", 191564, "187.1 KB", true
  },
  {
    "battleship", "BATTLESHIP", "Bắn Tàu",
    "Naval fleet combat", "Bắn tàu chiến hải quân",
    StoreCategory::Games, "/games", "battleship.elf", 119840, "117.0 KB", true
  },
  {
    "othello", "OTHELLO", "Cờ Lật",
    "Reversi disc-flipping", "Cờ lật Othello Reversi",
    StoreCategory::Games, "/games", "othello.elf", 132516, "129.4 KB", true
  },
  {
    "chess", "CHESS", "Cờ Vua",
    "Chess with puzzles", "Cờ vua và câu đố cờ",
    StoreCategory::Games, "/games", "chess.elf", 284008, "277.4 KB", true
  },
  {
    "go", "GO", "Cờ Vây",
    "The board game Go", "Cờ vây",
    StoreCategory::Games, "/games", "go.elf", 255820, "249.8 KB", true
  },
  {
    "theintercept", "THE INTERCEPT", "The Intercept",
    "Decode the intercepted message", "Giải mã thông điệp",
    StoreCategory::Games, "/games", "theintercept.elf", 96232, "94.0 KB", true
  },
  {
    "solitaire", "SOLITAIRE", "Xếp Bài",
    "Klondike solitaire", "Xếp bài Solitaire",
    StoreCategory::Games, "/games", "solitaire.elf", 152452, "148.9 KB", true
  },
  {
    "hearts", "HEARTS", "Bài Hearts",
    "Card game Hearts", "Đánh bài Hearts",
    StoreCategory::Games, "/games", "hearts.elf", 140044, "136.8 KB", true
  },
  {
    "murdle", "MURDLE", "Murdle",
    "Logic murder mysteries", "Giải đố phá án",
    StoreCategory::Games, "/games", "murdle.elf", 283584, "276.9 KB", true
  },
  {
    "carnivoro", "CARNIVORO", "Carnivoro",
    "Party deduction game", "Trò chơi suy luận",
    StoreCategory::Games, "/games", "carnivoro.elf", 166032, "162.1 KB", true
  },
  {
    "pet", "PET", "Thú Cưng",
    "Virtual pet that grows as you read", "Thú cưng lớn lên theo việc đọc sách",
    StoreCategory::Apps, "/apps", "pet.elf", 418944, "409.1 KB", true
  },
  {
    "readingquest", "READING QUEST", "Hành Trình Đọc",
    "Gamified reading adventure", "Phiêu lưu nhập vai từ việc đọc sách",
    StoreCategory::Apps, "/apps", "readingquest.elf", 388420, "379.3 KB", true
  },
  {
    "habits", "HABITS", "Thói Quen",
    "Daily habit tracker", "Theo dõi thói quen hàng ngày",
    StoreCategory::Apps, "/apps", "habits.elf", 105420, "102.9 KB", true
  },
  {
    "pomodoro", "POMODORO", "Pomodoro",
    "Focus timer", "Đồng hồ tập trung Pomodoro",
    StoreCategory::Apps, "/apps", "pomodoro.elf", 103044, "100.6 KB", true
  },
  {
    "calculator", "CALCULATOR", "Máy Tính",
    "Pocket calculator", "Máy tính bỏ túi",
    StoreCategory::Apps, "/apps", "calculator.elf", 133180, "130.1 KB", true
  },
  {
    "focus", "FOCUS", "Tập Trung",
    "Projects, tasks and focus timer", "Dự án, công việc và đồng hồ tập trung",
    StoreCategory::Apps, "/apps", "focus.elf", 157160, "153.5 KB", true
  },
  {
    "instapaper", "INSTAPAPER", "Instapaper",
    "Read saved web articles", "Đọc bài viết đã lưu từ Instapaper",
    StoreCategory::Apps, "/apps", "instapaper.elf", 155500, "151.9 KB", true
  },
  {
    "connections", "CONNECTIONS", "Connections",
    "Find groups of 4 words", "Tìm nhóm 4 từ liên kết",
    StoreCategory::Games, "/games", "connections.elf", 119900, "117.1 KB", true
  },
  {
    "knucklebones", "KNUCKLEBONES", "Cờ Xương",
    "Dice placement duel", "Đấu xúc xắc chiến thuật",
    StoreCategory::Games, "/games", "knucklebones.elf", 114060, "111.4 KB", true
  },
  {
    "yahtzee", "YAHTZEE", "Yahtzee",
    "Classic dice score sheet", "Đổ xúc xắc tính điểm Yahtzee",
    StoreCategory::Games, "/games", "yahtzee.elf", 119848, "117.0 KB", true
  },
  {
    "seasalt", "SEA SALT", "Sea Salt & Paper",
    "Origami card combo game", "Trò chơi thẻ bài Origami",
    StoreCategory::Games, "/games", "seasalt.elf", 162904, "159.1 KB", true
  },
  {
    "toybattle", "TOY BATTLE", "Toy Battle",
    "Mini tactics board game", "Chiến thuật bàn cờ thu nhỏ",
    StoreCategory::Games, "/games", "toybattle.elf", 202596, "197.8 KB", true
  },
  {
    "trivia", "TRIVIA", "Đố Vui",
    "Trivia questions quiz", "Câu hỏi đố vui kiến thức",
    StoreCategory::Games, "/games", "trivia.elf", 112560, "109.9 KB", true
  },
  {
    "underhand", "UNDERHAND", "Underhand",
    "Cultist card management game", "Quản lý thẻ bài giáo phái bí ẩn",
    StoreCategory::Games, "/games", "underhand.elf", 415072, "405.3 KB", true
  },
  {
    "wavelength", "WAVELENGTH", "Wavelength",
    "Telepathic spectrum guessing game", "Đoán phổ bước sóng thần giao",
    StoreCategory::Games, "/games", "wavelength.elf", 88960, "86.9 KB", true
  },
  {
    "forehead", "FOREHEAD", "Đoán Chữ",
    "Party word guessing game", "Trò chơi đoán từ trên trán",
    StoreCategory::Games, "/games", "forehead.elf", 158624, "154.9 KB", true
  },
  {
    "insider", "INSIDER", "Kẻ Nội Gián",
    "Secret word deduction party game", "Trò chơi suy luận kẻ nội gián",
    StoreCategory::Games, "/games", "insider.elf", 100520, "98.2 KB", true
  },
  {
    "xkcd", "XKCD", "Truyện XKCD",
    "XKCD offline webcomics reader", "Đọc truyện tranh XKCD offline",
    StoreCategory::Apps, "/apps", "xkcd.elf", 114512, "111.8 KB", true
  },
  {
    "hackernews", "HACKER NEWS", "Hacker News",
    "Tech news & discussion reader", "Đọc tin tức công nghệ Hacker News",
    StoreCategory::Apps, "/apps", "hackernews.elf", 140404, "137.1 KB", true
  },
  {
    "wikipedia", "WIKIPEDIA", "Wikipedia",
    "Offline Wikipedia article reader", "Đọc bách khoa toàn thư Wikipedia",
    StoreCategory::Apps, "/apps", "wikipedia.elf", 144704, "141.3 KB", true
  },
  {
    "dice", "DICE", "Xúc Xắc",
    "3D Dice roller physics", "Đổ xúc xắc 3D mô phỏng",
    StoreCategory::Games, "/games", "dice.elf", 4752, "4.7 KB"
  },
  {
    "lang-vi", "VIETNAMESE", "Tiếng Việt",
    "Vietnamese interface language", "Gói ngôn ngữ giao diện tiếng Việt",
    StoreCategory::Apps, "/languages", "vi.cplang", 193383, "188.9 KB", true, true
  },
};

inline constexpr size_t kStoreItemCount = sizeof(kStoreItems) / sizeof(kStoreItems[0]);
