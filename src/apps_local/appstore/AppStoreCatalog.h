#pragma once

#include <cstddef>
#include <cstdint>

#include "ResourcePacks.generated.h"

enum class StoreCategory : uint8_t {
  All = 0,
  Games = 1,
  Apps = 2,
  Languages = 3,  // interface language packs
  Resources = 4,  // microSD content packs (StoreItem::resources)
  Count
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
  // FORK CHANGE: microSD content an app reads at run time (PET / Reading Quest /
  // Intercept artwork, the Noto Serif reading font). Several files, each checked
  // against the size and SHA-256 the firmware carries; folder is where they go.
  const ResourcePack* resources = nullptr;
};

// Only apps with no built-in copy are listed. A small host-API plugin named
// like a built-in would replace it on the shelf with a lesser version, so a
// built-in app appears here only once it ships as a native module.
inline const StoreItem kStoreItems[] = {
  {
    "2048", "2048", "2048",
    "Number tile puzzle", "Trò chơi ghép ô số 2048",
    StoreCategory::Games, "/games", "2048.elf", 94564, "92.3 KB", true
  },
  {
    "minesweeper", "MINESWEEPER", "Dò Mìn",
    "Classic grid minefield", "Trò chơi dò mìn kinh điển",
    StoreCategory::Games, "/games", "minesweeper.elf", 129092, "126.1 KB", true
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
    StoreCategory::Games, "/games", "connectfour.elf", 115992, "113.3 KB", true
  },
  {
    "checkers", "CHECKERS", "Cờ Đam",
    "Draughts board game", "Cờ đam quốc tế truyền thống",
    StoreCategory::Games, "/games", "checkers.elf", 191596, "187.1 KB", true
  },
  {
    "battleship", "BATTLESHIP", "Bắn Tàu",
    "Naval fleet combat", "Bắn tàu chiến hải quân",
    StoreCategory::Games, "/games", "battleship.elf", 119840, "117.0 KB", true
  },
  {
    "othello", "OTHELLO", "Cờ Lật",
    "Reversi disc-flipping", "Cờ lật Othello Reversi",
    StoreCategory::Games, "/games", "othello.elf", 132560, "129.5 KB", true
  },
  {
    "chess", "CHESS", "Cờ Vua",
    "Chess with puzzles", "Cờ vua và câu đố cờ",
    StoreCategory::Games, "/games", "chess.elf", 284040, "277.4 KB", true
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
    StoreCategory::Games, "/games", "hearts.elf", 139992, "136.7 KB", true
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
    StoreCategory::Apps, "/apps", "pet.elf", 418984, "409.2 KB", true
  },
  {
    "readingquest", "READING QUEST", "Hành Trình Đọc",
    "Gamified reading adventure", "Phiêu lưu nhập vai từ việc đọc sách",
    StoreCategory::Apps, "/apps", "readingquest.elf", 388460, "379.4 KB", true
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
    StoreCategory::Apps, "/apps", "calculator.elf", 133212, "130.1 KB", true
  },
  {
    "focus", "FOCUS", "Tập Trung",
    "Projects, tasks and focus timer", "Dự án, công việc và đồng hồ tập trung",
    StoreCategory::Apps, "/apps", "focus.elf", 157164, "153.5 KB", true
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
    StoreCategory::Games, "/games", "trivia.elf", 112592, "110.0 KB", true
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
    StoreCategory::Apps, "/apps", "wikipedia.elf", 144736, "141.3 KB", true
  },
  {
    "dice", "DICE", "Xúc Xắc",
    "3D Dice roller physics", "Đổ xúc xắc 3D mô phỏng",
    StoreCategory::Games, "/games", "dice.elf", 4752, "4.7 KB"
  },
  {
    "tictactoe", "TIC-TAC-TOE", "Cờ Caro",
    "Classic 3x3 tic-tac-toe", "Trò chơi cờ ca-rô đối kháng kinh điển",
    StoreCategory::Games, "/games", "tictactoe.elf", 128736, "125.7 KB", true
  },
  {
    "dungeon", "D&DIAGRAMS", "Mê Cung D&D",
    "64 dungeon logic puzzles", "64 câu đố hầm ngục logic D&D",
    StoreCategory::Games, "/games", "dungeon.elf", 157024, "153.3 KB", true
  },
  {
    "oneclue", "ONE CLUE", "Ô Chữ Manh Mối",
    "One-clue crossword puzzle", "Ô chữ giải đố qua một manh mối",
    StoreCategory::Games, "/games", "oneclue.elf", 141832, "138.5 KB", true
  },
  {
    "lightsout", "LIGHTS OUT", "Tắt Đèn",
    "5x5 light toggle logic puzzle", "Giải đố lật ô tắt hết đèn",
    StoreCategory::Games, "/games", "lightsout.elf", 132856, "129.7 KB", true
  },
  {
    "suguru", "SUGURU", "Suguru",
    "800 regional number logic puzzles", "800 câu đố điền số vùng logic",
    StoreCategory::Games, "/games", "suguru.elf", 228532, "223.2 KB", true
  },
  {
    "sokoban", "SOKOBAN", "Sokoban",
    "Push boxes onto goals (385 levels)", "Đẩy hộp vào đích 385 màn",
    StoreCategory::Games, "/games", "sokoban.elf", 220064, "214.9 KB", true
  },
  {
    "queens", "QUEENS", "Đặt Hậu",
    "1200 queens and star battle puzzles", "1200 câu đố đặt hậu và ngôi sao",
    StoreCategory::Games, "/games", "queens.elf", 263916, "257.7 KB", true
  },
  {
    "symboldice", "SYMBOL DICE", "Xúc Xắc Ký Hiệu",
    "Storytelling symbol dice", "Đổ xúc xắc biểu tượng phiêu lưu kỳ bí",
    StoreCategory::Games, "/games", "symboldice.elf", 128460, "125.4 KB", true
  },
  {
    "pegsolitaire", "PEG SOLITAIRE", "Nhảy Chốt",
    "Classic jump peg solitaire", "Nhảy chốt bàn cờ đơn cổ điển",
    StoreCategory::Games, "/games", "pegsolitaire.elf", 133120, "130.0 KB", true
  },
  {
    "textadv", "TEXT ADVENTURES", "Phiêu Lưu Chữ",
    "Interactive fiction with Zork I-III", "Game phiêu lưu nhập vai chữ Zork I, II, III",
    StoreCategory::Games, "/games", "textadv.elf", 457772, "447.0 KB", true
  },
  {
    "wordladder", "WORD LADDER", "Thang Từ",
    "Step-by-step word ladder puzzle", "Biến đổi từ từng bước một chữ cái",
    StoreCategory::Games, "/games", "wordladder.elf", 272000, "265.6 KB", true
  },
  {
    "diceroller", "DICE ROLLER", "Đổ Xúc Xắc RPG",
    "RPG polyhedral dice roller", "Đổ xúc xắc nhập vai d4 đến d100",
    StoreCategory::Games, "/games", "diceroller.elf", 127900, "124.9 KB", true
  },
  {
    "nonogram", "NONOGRAM", "Nonogram",
    "600 picture logic puzzles", "Tranh giải đố số ô vuông 600 màn",
    StoreCategory::Games, "/games", "nonogram.elf", 166076, "162.2 KB", true
  },
  {
    "kakuro", "KAKURO", "Kakuro",
    "600 cross sums number puzzles", "600 câu đố điền số tổng chéo",
    StoreCategory::Games, "/games", "kakuro.elf", 185208, "180.9 KB", true
  },
  {
    "bingocaller", "BINGO CALLER", "Quay Số Bingo",
    "Electronic bingo number caller", "Máy quay số gọi Bingo tự động",
    StoreCategory::Games, "/games", "bingocaller.elf", 130128, "127.1 KB", true
  },
  {
    "islandmaps", "ISLAND MAPS", "Bản Đồ Đảo",
    "Forbidden Island map generator", "Trình tạo bản đồ đảo phiêu lưu",
    StoreCategory::Games, "/games", "islandmaps.elf", 132008, "128.9 KB", true
  },
  {
    "thinkalike", "THINK ALIKE", "Đồng Điệu",
    "Party word association game", "Trò chơi liên tưởng từ nhóm",
    StoreCategory::Games, "/games", "thinkalike.elf", 134296, "131.1 KB", true
  },
  {
    "authoritytracker", "AUTHORITY TRACKER", "Theo Dõi Điểm",
    "Card game health and authority tracker", "Bộ đếm điểm máu thẻ bài",
    StoreCategory::Games, "/games", "authoritytracker.elf", 127924, "124.9 KB", true
  },
  {
    "gridgambit", "GRID GAMBIT", "Grid Gambit",
    "Tactical 4x4 chess puzzle", "Câu đố chiến thuật cờ vua 4x4",
    StoreCategory::Games, "/games", "gridgambit.elf", 141884, "138.6 KB", true
  },
  {
    "chesspuzzles", "CHESS PUZZLES", "Thế Cờ Vua",
    "2398 Lichess tactics puzzles", "2398 câu đố chiến thuật cờ Lichess",
    StoreCategory::Games, "/games", "chesspuzzles.elf", 451084, "440.5 KB", true
  },
  {
    "polyominopanic", "POLYOMINO PANIC", "Polyomino Panic",
    "Pentomino packing puzzle", "Ghép hình các khối Pentomino",
    StoreCategory::Games, "/games", "polyominopanic.elf", 132444, "129.3 KB", true
  },
  {
    "crowncrest", "CROWN & CREST", "Vương Miện & Huy Hiệu",
    "Dice push-your-luck game", "Đổ xúc xắc tích điểm may rủi",
    StoreCategory::Games, "/games", "crowncrest.elf", 230328, "224.9 KB", true
  },
  {
    "lastpiece", "LAST PIECE", "Quân Cờ Cuối",
    "Chess endgame tactical puzzle", "Câu đố tàn cuộc cờ vua độc đáo",
    StoreCategory::Games, "/games", "lastpiece.elf", 158368, "154.7 KB", true
  },
  {
    "lang-vi", "VIETNAMESE", "Tiếng Việt",
    "Vietnamese interface language", "Gói ngôn ngữ giao diện tiếng Việt",
    StoreCategory::Languages, "/languages", "vi.cplang", 426214, "416.2 KB", true, true
  },
  // Resource packs (ResourcePacks.generated.h). Sizes are the whole pack.
  {
    "res-pet", "PET RESOURCES", "Tài nguyên Thú cưng",
    "Artwork the PET app needs", "Hình ảnh cần cho app Thú cưng (PET)",
    StoreCategory::Resources, "/apps/pet-v2", "", 10903670, "10.4 MB", false, false, &kResourcePacks[0]
  },
  {
    "res-quest", "READING QUEST RESOURCES", "Tài nguyên Reading Quest",
    "Artwork Reading Quest needs", "Hình ảnh cần cho Reading Quest",
    StoreCategory::Resources, "/apps/reading-adventure-v2", "", 28724452, "27.4 MB", false, false, &kResourcePacks[1]
  },
  {
    "res-intercept", "THE INTERCEPT RESOURCES", "Tài nguyên The Intercept",
    "Artwork The Intercept needs", "Hình ảnh cần cho The Intercept",
    StoreCategory::Resources, "/apps/intercept", "", 276357, "269.9 KB", false, false, &kResourcePacks[2]
  },
};

inline constexpr size_t kStoreItemCount = sizeof(kStoreItems) / sizeof(kStoreItems[0]);
