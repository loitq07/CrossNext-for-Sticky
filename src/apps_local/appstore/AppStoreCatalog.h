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
};

// Only apps with no built-in copy are listed. A small host-API plugin named
// like a built-in would replace it on the shelf with a lesser version, so a
// built-in app appears here only once it ships as a native module.
inline const StoreItem kStoreItems[] = {
  {
    "2048", "2048", "2048",
    "Number tile puzzle", "Trò chơi ghép ô số 2048",
    StoreCategory::Games, "/games", "2048.elf", 79332, "77.5 KB", true
  },
  {
    "minesweeper", "MINESWEEPER", "Dò Mìn",
    "Classic grid minefield", "Trò chơi dò mìn kinh điển",
    StoreCategory::Games, "/games", "minesweeper.elf", 112964, "110.3 KB", true
  },
  {
    "picross", "PICROSS", "Picross",
    "Nonogram logic picture", "Tranh giải đố số Nonogram",
    StoreCategory::Games, "/games", "picross.elf", 159896, "156.1 KB", true
  },
  {
    "sudoku", "SUDOKU", "Sudoku",
    "Number logic grid puzzle", "Giải đố điền số Sudoku",
    StoreCategory::Games, "/games", "sudoku.elf", 111112, "108.5 KB", true
  },
  {
    "wordle", "WORDLE", "Đoán Từ",
    "5-letter word puzzle", "Trò chơi đoán từ 5 chữ cái",
    StoreCategory::Games, "/games", "wordle.elf", 231792, "226.4 KB", true
  },
  {
    "connectfour", "CONNECT FOUR", "Nối 4 Điểm",
    "Drop & connect 4 discs", "Cờ nối 4 điểm đối kháng",
    StoreCategory::Games, "/games", "connectfour.elf", 99264, "96.9 KB", true
  },
  {
    "checkers", "CHECKERS", "Cờ Đam",
    "Draughts board game", "Cờ đam quốc tế truyền thống",
    StoreCategory::Games, "/games", "checkers.elf", 187048, "182.7 KB", true
  },
  {
    "battleship", "BATTLESHIP", "Bắn Tàu",
    "Naval fleet combat", "Bắn tàu chiến hải quân",
    StoreCategory::Games, "/games", "battleship.elf", 103500, "101.1 KB", true
  },
  {
    "othello", "OTHELLO", "Cờ Lật",
    "Reversi disc-flipping", "Cờ lật Othello Reversi",
    StoreCategory::Games, "/games", "othello.elf", 116372, "113.6 KB", true
  },
  {
    "chess", "CHESS", "Cờ Vua",
    "Chess with puzzles", "Cờ vua và câu đố cờ",
    StoreCategory::Games, "/games", "chess.elf", 0, "", true
  },
  {
    "go", "GO", "Cờ Vây",
    "The board game Go", "Cờ vây",
    StoreCategory::Games, "/games", "go.elf", 0, "", true
  },
  {
    "theintercept", "THE INTERCEPT", "The Intercept",
    "Decode the intercepted message", "Giải mã thông điệp",
    StoreCategory::Games, "/games", "theintercept.elf", 0, "", true
  },
  {
    "solitaire", "SOLITAIRE", "Xếp Bài",
    "Klondike solitaire", "Xếp bài Solitaire",
    StoreCategory::Games, "/games", "solitaire.elf", 0, "", true
  },
  {
    "hearts", "HEARTS", "Bài Hearts",
    "Card game Hearts", "Đánh bài Hearts",
    StoreCategory::Games, "/games", "hearts.elf", 0, "", true
  },
  {
    "murdle", "MURDLE", "Murdle",
    "Logic murder mysteries", "Giải đố phá án",
    StoreCategory::Games, "/games", "murdle.elf", 0, "", true
  },
  {
    "carnivoro", "CARNIVORO", "Carnivoro",
    "Party deduction game", "Trò chơi suy luận",
    StoreCategory::Games, "/games", "carnivoro.elf", 0, "", true
  },
  {
    "pet", "PET", "Thú Cưng",
    "Virtual pet that grows as you read", "Thú cưng lớn lên theo việc đọc sách",
    StoreCategory::Apps, "/apps", "pet.elf", 349616, "341.4 KB", true
  },
  {
    "readingquest", "READING QUEST", "Hành Trình Đọc",
    "Gamified reading adventure", "Phiêu lưu nhập vai từ việc đọc sách",
    StoreCategory::Apps, "/apps", "readingquest.elf", 96148, "93.9 KB", true
  },
  {
    "habits", "HABITS", "Thói Quen",
    "Daily habit tracker", "Theo dõi thói quen hàng ngày",
    StoreCategory::Apps, "/apps", "habits.elf", 90548, "88.4 KB", true
  },
  {
    "pomodoro", "POMODORO", "Pomodoro",
    "Focus timer", "Đồng hồ tập trung Pomodoro",
    StoreCategory::Apps, "/apps", "pomodoro.elf", 62900, "61.4 KB", true
  },
  {
    "calculator", "CALCULATOR", "Máy Tính",
    "Pocket calculator", "Máy tính bỏ túi",
    StoreCategory::Apps, "/apps", "calculator.elf", 107204, "104.7 KB", true
  },
  {
    "dice", "DICE", "Xúc Xắc",
    "3D Dice roller physics", "Đổ xúc xắc 3D mô phỏng",
    StoreCategory::Games, "/games", "dice.elf", 4752, "4.7 KB"
  },
};

inline constexpr size_t kStoreItemCount = sizeof(kStoreItems) / sizeof(kStoreItems[0]);
