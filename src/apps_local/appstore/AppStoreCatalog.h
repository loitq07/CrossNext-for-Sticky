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
    StoreCategory::Games, "/games", "2048.elf", 94692, "92.5 KB", true
  },
  {
    "minesweeper", "MINESWEEPER", "Dò Mìn",
    "Classic grid minefield", "Trò chơi dò mìn kinh điển",
    StoreCategory::Games, "/games", "minesweeper.elf", 129220, "126.2 KB", true
  },
  {
    "picross", "PICROSS", "Picross",
    "Nonogram logic picture", "Tranh giải đố số Nonogram",
    StoreCategory::Games, "/games", "picross.elf", 173516, "169.4 KB", true
  },
  {
    "sudoku", "SUDOKU", "Sudoku",
    "Number logic grid puzzle", "Giải đố điền số Sudoku",
    StoreCategory::Games, "/games", "sudoku.elf", 128648, "125.6 KB", true
  },
  {
    "wordle", "WORDLE", "Đoán Từ",
    "5-letter word puzzle", "Trò chơi đoán từ 5 chữ cái",
    StoreCategory::Games, "/games", "wordle.elf", 266632, "260.4 KB", true
  },
  {
    "connectfour", "CONNECT FOUR", "Nối 4 Điểm",
    "Drop & connect 4 discs", "Cờ nối 4 điểm đối kháng",
    StoreCategory::Games, "/games", "connectfour.elf", 116124, "113.4 KB", true
  },
  {
    "checkers", "CHECKERS", "Cờ Đam",
    "Draughts board game", "Cờ đam quốc tế truyền thống",
    StoreCategory::Games, "/games", "checkers.elf", 191724, "187.2 KB", true
  },
  {
    "battleship", "BATTLESHIP", "Bắn Tàu",
    "Naval fleet combat", "Bắn tàu chiến hải quân",
    StoreCategory::Games, "/games", "battleship.elf", 119920, "117.1 KB", true
  },
  {
    "othello", "OTHELLO", "Cờ Lật",
    "Reversi disc-flipping", "Cờ lật Othello Reversi",
    StoreCategory::Games, "/games", "othello.elf", 132684, "129.6 KB", true
  },
  {
    "chess", "CHESS", "Cờ Vua",
    "Chess with puzzles", "Cờ vua và câu đố cờ",
    StoreCategory::Games, "/games", "chess.elf", 284204, "277.5 KB", true
  },
  {
    "go", "GO", "Cờ Vây",
    "The board game Go", "Cờ vây",
    StoreCategory::Games, "/games", "go.elf", 255900, "249.9 KB", true
  },
  {
    "theintercept", "THE INTERCEPT", "The Intercept",
    "Decode the intercepted message", "Giải mã thông điệp",
    StoreCategory::Games, "/games", "theintercept.elf", 96312, "94.1 KB", true
  },
  {
    "solitaire", "SOLITAIRE", "Xếp Bài",
    "Klondike solitaire", "Xếp bài Solitaire",
    StoreCategory::Games, "/games", "solitaire.elf", 152532, "149.0 KB", true
  },
  {
    "hearts", "HEARTS", "Bài Hearts",
    "Card game Hearts", "Đánh bài Hearts",
    StoreCategory::Games, "/games", "hearts.elf", 140072, "136.8 KB", true
  },
  {
    "murdle", "MURDLE", "Murdle",
    "Logic murder mysteries", "Giải đố phá án",
    StoreCategory::Games, "/games", "murdle.elf", 283664, "277.0 KB", true
  },
  {
    "carnivoro", "CARNIVORO", "Carnivoro",
    "Party deduction game", "Trò chơi suy luận",
    StoreCategory::Games, "/games", "carnivoro.elf", 166112, "162.2 KB", true
  },
  {
    "pet", "PET", "Thú Cưng",
    "Virtual pet that grows as you read", "Thú cưng lớn lên theo việc đọc sách",
    StoreCategory::Apps, "/apps", "pet.elf", 419120, "409.3 KB", true
  },
  {
    "readingquest", "READING QUEST", "Hành Trình Đọc",
    "Gamified reading adventure", "Phiêu lưu nhập vai từ việc đọc sách",
    StoreCategory::Apps, "/apps", "readingquest.elf", 388588, "379.5 KB", true
  },
  {
    "habits", "HABITS", "Thói Quen",
    "Daily habit tracker", "Theo dõi thói quen hàng ngày",
    StoreCategory::Apps, "/apps", "habits.elf", 105536, "103.1 KB", true
  },
  {
    "pomodoro", "POMODORO", "Pomodoro",
    "Focus timer", "Đồng hồ tập trung Pomodoro",
    StoreCategory::Apps, "/apps", "pomodoro.elf", 103124, "100.7 KB", true
  },
  {
    "calculator", "CALCULATOR", "Máy Tính",
    "Pocket calculator", "Máy tính bỏ túi",
    StoreCategory::Apps, "/apps", "calculator.elf", 133336, "130.2 KB", true
  },
  {
    "focus", "FOCUS", "Tập Trung",
    "Projects, tasks and focus timer", "Dự án, công việc và đồng hồ tập trung",
    StoreCategory::Apps, "/apps", "focus.elf", 157312, "153.6 KB", true
  },
  {
    "instapaper", "INSTAPAPER", "Instapaper",
    "Read saved web articles", "Đọc bài viết đã lưu từ Instapaper",
    StoreCategory::Apps, "/apps", "instapaper.elf", 155580, "151.9 KB", true
  },
  {
    "connections", "CONNECTIONS", "Connections",
    "Find groups of 4 words", "Tìm nhóm 4 từ liên kết",
    StoreCategory::Games, "/games", "connections.elf", 119980, "117.2 KB", true
  },
  {
    "knucklebones", "KNUCKLEBONES", "Cờ Xương",
    "Dice placement duel", "Đấu xúc xắc chiến thuật",
    StoreCategory::Games, "/games", "knucklebones.elf", 114204, "111.5 KB", true
  },
  {
    "yahtzee", "YAHTZEE", "Yahtzee",
    "Classic dice score sheet", "Đổ xúc xắc tính điểm Yahtzee",
    StoreCategory::Games, "/games", "yahtzee.elf", 119936, "117.1 KB", true
  },
  {
    "seasalt", "SEA SALT", "Sea Salt & Paper",
    "Origami card combo game", "Trò chơi thẻ bài Origami",
    StoreCategory::Games, "/games", "seasalt.elf", 162992, "159.2 KB", true
  },
  {
    "toybattle", "TOY BATTLE", "Toy Battle",
    "Mini tactics board game", "Chiến thuật bàn cờ thu nhỏ",
    StoreCategory::Games, "/games", "toybattle.elf", 202684, "197.9 KB", true
  },
  {
    "trivia", "TRIVIA", "Đố Vui",
    "Trivia questions quiz", "Câu hỏi đố vui kiến thức",
    StoreCategory::Games, "/games", "trivia.elf", 112712, "110.1 KB", true
  },
  {
    "underhand", "UNDERHAND", "Underhand",
    "Cultist card management game", "Quản lý thẻ bài giáo phái bí ẩn",
    StoreCategory::Games, "/games", "underhand.elf", 415196, "405.5 KB", true
  },
  {
    "wavelength", "WAVELENGTH", "Wavelength",
    "Telepathic spectrum guessing game", "Đoán phổ bước sóng thần giao",
    StoreCategory::Games, "/games", "wavelength.elf", 89040, "87.0 KB", true
  },
  {
    "forehead", "FOREHEAD", "Đoán Chữ",
    "Party word guessing game", "Trò chơi đoán từ trên trán",
    StoreCategory::Games, "/games", "forehead.elf", 158744, "155.0 KB", true
  },
  {
    "insider", "INSIDER", "Kẻ Nội Gián",
    "Secret word deduction party game", "Trò chơi suy luận kẻ nội gián",
    StoreCategory::Games, "/games", "insider.elf", 100652, "98.3 KB", true
  },
  {
    "xkcd", "XKCD", "Truyện XKCD",
    "XKCD offline webcomics reader", "Đọc truyện tranh XKCD offline",
    StoreCategory::Apps, "/apps", "xkcd.elf", 114632, "111.9 KB", true
  },
  {
    "hackernews", "HACKER NEWS", "Hacker News",
    "Tech news & discussion reader", "Đọc tin tức công nghệ Hacker News",
    StoreCategory::Apps, "/apps", "hackernews.elf", 140484, "137.2 KB", true
  },
  {
    "wikipedia", "WIKIPEDIA", "Wikipedia",
    "Offline Wikipedia article reader", "Đọc bách khoa toàn thư Wikipedia",
    StoreCategory::Apps, "/apps", "wikipedia.elf", 144844, "141.4 KB", true
  },
  {
    "dice", "DICE", "Xúc Xắc",
    "3D Dice roller physics", "Đổ xúc xắc 3D mô phỏng",
    StoreCategory::Games, "/games", "dice.elf", 3152, "3.1 KB"
  },
  {
    "tictactoe", "TIC-TAC-TOE", "Cờ Caro",
    "Classic 3x3 tic-tac-toe", "Trò chơi cờ ca-rô đối kháng kinh điển",
    StoreCategory::Games, "/games", "tictactoe.elf", 130436, "127.4 KB", true
  },
  {
    "dungeon", "D&DIAGRAMS", "Mê Cung D&D",
    "64 dungeon logic puzzles", "64 câu đố hầm ngục logic D&D",
    StoreCategory::Games, "/games", "dungeon.elf", 157444, "153.8 KB", true
  },
  {
    "oneclue", "ONE CLUE", "Ô Chữ Manh Mối",
    "One-clue crossword puzzle", "Ô chữ giải đố qua một manh mối",
    StoreCategory::Games, "/games", "oneclue.elf", 142244, "138.9 KB", true
  },
  {
    "lightsout", "LIGHTS OUT", "Tắt Đèn",
    "5x5 light toggle logic puzzle", "Giải đố lật ô tắt hết đèn",
    StoreCategory::Games, "/games", "lightsout.elf", 133520, "130.4 KB", true
  },
  {
    "suguru", "SUGURU", "Suguru",
    "800 regional number logic puzzles", "800 câu đố điền số vùng logic",
    StoreCategory::Games, "/games", "suguru.elf", 230240, "224.8 KB", true
  },
  {
    "sokoban", "SOKOBAN", "Sokoban",
    "Push boxes onto goals (385 levels)", "Đẩy hộp vào đích 385 màn",
    StoreCategory::Games, "/games", "sokoban.elf", 220936, "215.8 KB", true
  },
  {
    "queens", "QUEENS", "Đặt Hậu",
    "1200 queens and star battle puzzles", "1200 câu đố đặt hậu và ngôi sao",
    StoreCategory::Games, "/games", "queens.elf", 265924, "259.7 KB", true
  },
  {
    "symboldice", "SYMBOL DICE", "Xúc Xắc Ký Hiệu",
    "Storytelling symbol dice", "Đổ xúc xắc biểu tượng phiêu lưu kỳ bí",
    StoreCategory::Games, "/games", "symboldice.elf", 130164, "127.1 KB", true
  },
  {
    "pegsolitaire", "PEG SOLITAIRE", "Nhảy Chốt",
    "Classic jump peg solitaire", "Nhảy chốt bàn cờ đơn cổ điển",
    StoreCategory::Games, "/games", "pegsolitaire.elf", 133836, "130.7 KB", true
  },
  {
    "textadv", "TEXT ADVENTURES", "Phiêu Lưu Chữ",
    "Interactive fiction with Zork I-III", "Game phiêu lưu nhập vai chữ Zork I, II, III",
    StoreCategory::Games, "/games", "textadv.elf", 461616, "450.8 KB", true
  },
  {
    "wordladder", "WORD LADDER", "Thang Từ",
    "Step-by-step word ladder puzzle", "Biến đổi từ từng bước một chữ cái",
    StoreCategory::Games, "/games", "wordladder.elf", 276516, "270.0 KB", true
  },
  {
    "diceroller", "DICE ROLLER", "Đổ Xúc Xắc RPG",
    "RPG polyhedral dice roller", "Đổ xúc xắc nhập vai d4 đến d100",
    StoreCategory::Games, "/games", "diceroller.elf", 129592, "126.6 KB", true
  },
  {
    "nonogram", "NONOGRAM", "Nonogram",
    "600 picture logic puzzles", "Tranh giải đố số ô vuông 600 màn",
    StoreCategory::Games, "/games", "nonogram.elf", 167792, "163.9 KB", true
  },
  {
    "kakuro", "KAKURO", "Kakuro",
    "600 cross sums number puzzles", "600 câu đố điền số tổng chéo",
    StoreCategory::Games, "/games", "kakuro.elf", 186976, "182.6 KB", true
  },
  {
    "bingocaller", "BINGO CALLER", "Quay Số Bingo",
    "Electronic bingo number caller", "Máy quay số gọi Bingo tự động",
    StoreCategory::Games, "/games", "bingocaller.elf", 132556, "129.4 KB", true
  },
  {
    "islandmaps", "ISLAND MAPS", "Bản Đồ Đảo",
    "Forbidden Island map generator", "Trình tạo bản đồ đảo phiêu lưu",
    StoreCategory::Games, "/games", "islandmaps.elf", 133704, "130.6 KB", true
  },
  {
    "thinkalike", "THINK ALIKE", "Đồng Điệu",
    "Party word association game", "Trò chơi liên tưởng từ nhóm",
    StoreCategory::Games, "/games", "thinkalike.elf", 135996, "132.8 KB", true
  },
  {
    "authoritytracker", "AUTHORITY TRACKER", "Theo Dõi Điểm",
    "Card game health and authority tracker", "Bộ đếm điểm máu thẻ bài",
    StoreCategory::Games, "/games", "authoritytracker.elf", 129636, "126.6 KB", true
  },
  {
    "gridgambit", "GRID GAMBIT", "Grid Gambit",
    "Tactical 4x4 chess puzzle", "Câu đố chiến thuật cờ vua 4x4",
    StoreCategory::Games, "/games", "gridgambit.elf", 142700, "139.4 KB", true
  },
  {
    "chesspuzzles", "CHESS PUZZLES", "Thế Cờ Vua",
    "2398 Lichess tactics puzzles", "2398 câu đố chiến thuật cờ Lichess",
    StoreCategory::Games, "/games", "chesspuzzles.elf", 453976, "443.3 KB", true
  },
  {
    "polyominopanic", "POLYOMINO PANIC", "Polyomino Panic",
    "Pentomino packing puzzle", "Ghép hình các khối Pentomino",
    StoreCategory::Games, "/games", "polyominopanic.elf", 133008, "129.9 KB", true
  },
  {
    "crowncrest", "CROWN & CREST", "Vương Miện & Huy Hiệu",
    "Dice push-your-luck game", "Đổ xúc xắc tích điểm may rủi",
    StoreCategory::Games, "/games", "crowncrest.elf", 230924, "225.5 KB", true
  },
  {
    "lastpiece", "LAST PIECE", "Quân Cờ Cuối",
    "Chess endgame tactical puzzle", "Câu đố tàn cuộc cờ vua độc đáo",
    StoreCategory::Games, "/games", "lastpiece.elf", 160036, "156.3 KB", true
  },
  {
    "lang-vi", "VIETNAMESE", "Tiếng Việt",
    "Vietnamese interface language", "Gói ngôn ngữ giao diện tiếng Việt",
    StoreCategory::Languages, "/languages", "vi.cplang", 426183, "416.2 KB", true, true
  },
  {
    "lang-ar", "ARABIC", "Tiếng Ả Rập",
    "Arabic interface language", "Gói ngôn ngữ giao diện tiếng Ả Rập",
    StoreCategory::Languages, "/languages", "ar.cplang", 451688, "441.1 KB", true, true
  },
  {
    "lang-az", "AZERBAIJANI", "Tiếng Azerbaijan",
    "Azerbaijani interface language", "Gói ngôn ngữ giao diện tiếng Azerbaijan",
    StoreCategory::Languages, "/languages", "az.cplang", 380222, "371.3 KB", true, true
  },
  {
    "lang-be", "BELARUSIAN", "Tiếng Belarus",
    "Belarusian interface language", "Gói ngôn ngữ giao diện tiếng Belarus",
    StoreCategory::Languages, "/languages", "be.cplang", 529651, "517.2 KB", true, true
  },
  {
    "lang-bs", "BOSNIAN", "Tiếng Bosnia",
    "Bosnian interface language", "Gói ngôn ngữ giao diện tiếng Bosnia",
    StoreCategory::Languages, "/languages", "bs.cplang", 339419, "331.5 KB", true, true
  },
  {
    "lang-bg", "BULGARIAN", "Tiếng Bulgaria",
    "Bulgarian interface language", "Gói ngôn ngữ giao diện tiếng Bulgaria",
    StoreCategory::Languages, "/languages", "bg.cplang", 549365, "536.5 KB", true, true
  },
  {
    "lang-ca", "CATALAN", "Tiếng Catalunya",
    "Catalan interface language", "Gói ngôn ngữ giao diện tiếng Catalunya",
    StoreCategory::Languages, "/languages", "ca.cplang", 360493, "352.0 KB", true, true
  },
  {
    "lang-zh-cn", "CHINESE (SIMPLIFIED)", "Tiếng Trung (Giản thể)",
    "Simplified Chinese interface language", "Gói ngôn ngữ giao diện tiếng Trung giản thể",
    StoreCategory::Languages, "/languages", "zh-CN.cplang", 321127, "313.6 KB", true, true
  },
  {
    "lang-zh-tw", "CHINESE (TRADITIONAL)", "Tiếng Trung (Phồn thể)",
    "Traditional Chinese interface language", "Gói ngôn ngữ giao diện tiếng Trung phồn thể",
    StoreCategory::Languages, "/languages", "zh-TW.cplang", 321269, "313.7 KB", true, true
  },
  {
    "lang-cs", "CZECH", "Tiếng Séc",
    "Czech interface language", "Gói ngôn ngữ giao diện tiếng Séc",
    StoreCategory::Languages, "/languages", "cs.cplang", 353808, "345.5 KB", true, true
  },
  {
    "lang-da", "DANISH", "Tiếng Đan Mạch",
    "Danish interface language", "Gói ngôn ngữ giao diện tiếng Đan Mạch",
    StoreCategory::Languages, "/languages", "da.cplang", 338742, "330.8 KB", true, true
  },
  {
    "lang-nl", "DUTCH", "Tiếng Hà Lan",
    "Dutch interface language", "Gói ngôn ngữ giao diện tiếng Hà Lan",
    StoreCategory::Languages, "/languages", "nl.cplang", 358783, "350.4 KB", true, true
  },
  {
    "lang-fi", "FINNISH", "Tiếng Phần Lan",
    "Finnish interface language", "Gói ngôn ngữ giao diện tiếng Phần Lan",
    StoreCategory::Languages, "/languages", "fi.cplang", 348486, "340.3 KB", true, true
  },
  {
    "lang-fr", "FRENCH", "Tiếng Pháp",
    "French interface language", "Gói ngôn ngữ giao diện tiếng Pháp",
    StoreCategory::Languages, "/languages", "fr.cplang", 378109, "369.2 KB", true, true
  },
  {
    "lang-de", "GERMAN", "Tiếng Đức",
    "German interface language", "Gói ngôn ngữ giao diện tiếng Đức",
    StoreCategory::Languages, "/languages", "de.cplang", 386277, "377.2 KB", true, true
  },
  {
    "lang-he", "HEBREW", "Tiếng Do Thái",
    "Hebrew interface language", "Gói ngôn ngữ giao diện tiếng Do Thái",
    StoreCategory::Languages, "/languages", "he.cplang", 420468, "410.6 KB", true, true
  },
  {
    "lang-hu", "HUNGARIAN", "Tiếng Hungary",
    "Hungarian interface language", "Gói ngôn ngữ giao diện tiếng Hungary",
    StoreCategory::Languages, "/languages", "hu.cplang", 377507, "368.7 KB", true, true
  },
  {
    "lang-id", "INDONESIAN", "Tiếng Indonesia",
    "Indonesian interface language", "Gói ngôn ngữ giao diện tiếng Indonesia",
    StoreCategory::Languages, "/languages", "id.cplang", 356430, "348.1 KB", true, true
  },
  {
    "lang-it", "ITALIAN", "Tiếng Ý",
    "Italian interface language", "Gói ngôn ngữ giao diện tiếng Ý",
    StoreCategory::Languages, "/languages", "it.cplang", 365909, "357.3 KB", true, true
  },
  {
    "lang-ja", "JAPANESE", "Tiếng Nhật",
    "Japanese interface language", "Gói ngôn ngữ giao diện tiếng Nhật",
    StoreCategory::Languages, "/languages", "ja.cplang", 427931, "417.9 KB", true, true
  },
  {
    "lang-kk", "KAZAKH", "Tiếng Kazakh",
    "Kazakh interface language", "Gói ngôn ngữ giao diện tiếng Kazakh",
    StoreCategory::Languages, "/languages", "kk.cplang", 542400, "529.7 KB", true, true
  },
  {
    "lang-lt", "LITHUANIAN", "Tiếng Litva",
    "Lithuanian interface language", "Gói ngôn ngữ giao diện tiếng Litva",
    StoreCategory::Languages, "/languages", "lt.cplang", 363230, "354.7 KB", true, true
  },
  {
    "lang-nb", "NORWEGIAN", "Tiếng Na Uy",
    "Norwegian interface language", "Gói ngôn ngữ giao diện tiếng Na Uy",
    StoreCategory::Languages, "/languages", "nb.cplang", 336543, "328.7 KB", true, true
  },
  {
    "lang-ook", "ORANGUTAN", "Tiếng Đười ươi (Ook)",
    "Ook interface language", "Gói ngôn ngữ giao diện tiếng Đười ươi",
    StoreCategory::Languages, "/languages", "ook.cplang", 9115, "8.9 KB", true, true
  },
  {
    "lang-fa", "PERSIAN", "Tiếng Ba Tư",
    "Persian interface language", "Gói ngôn ngữ giao diện tiếng Ba Tư",
    StoreCategory::Languages, "/languages", "fa.cplang", 492027, "480.5 KB", true, true
  },
  {
    "lang-pl", "POLISH", "Tiếng Ba Lan",
    "Polish interface language", "Gói ngôn ngữ giao diện tiếng Ba Lan",
    StoreCategory::Languages, "/languages", "pl.cplang", 357768, "349.4 KB", true, true
  },
  {
    "lang-pt-br", "PORTUGUESE (BRAZIL)", "Tiếng Bồ Đào Nha (Brazil)",
    "Brazilian Portuguese interface language", "Gói ngôn ngữ tiếng Bồ Đào Nha (Brazil)",
    StoreCategory::Languages, "/languages", "pt-BR.cplang", 357305, "348.9 KB", true, true
  },
  {
    "lang-pt-pt", "PORTUGUESE (PORTUGAL)", "Tiếng Bồ Đào Nha (Bồ Đào Nha)",
    "European Portuguese interface language", "Gói ngôn ngữ tiếng Bồ Đào Nha (Bồ Đào Nha)",
    StoreCategory::Languages, "/languages", "pt-PT.cplang", 357953, "349.6 KB", true, true
  },
  {
    "lang-ro", "ROMANIAN", "Tiếng Romania",
    "Romanian interface language", "Gói ngôn ngữ giao diện tiếng Romania",
    StoreCategory::Languages, "/languages", "ro.cplang", 367327, "358.7 KB", true, true
  },
  {
    "lang-ru", "RUSSIAN", "Tiếng Nga",
    "Russian interface language", "Gói ngôn ngữ giao diện tiếng Nga",
    StoreCategory::Languages, "/languages", "ru.cplang", 534618, "522.1 KB", true, true
  },
  {
    "lang-sk", "SLOVAK", "Tiếng Slovakia",
    "Slovak interface language", "Gói ngôn ngữ giao diện tiếng Slovakia",
    StoreCategory::Languages, "/languages", "sk.cplang", 355711, "347.4 KB", true, true
  },
  {
    "lang-sl", "SLOVENIAN", "Tiếng Slovenia",
    "Slovenian interface language", "Gói ngôn ngữ giao diện tiếng Slovenia",
    StoreCategory::Languages, "/languages", "sl.cplang", 336906, "329.0 KB", true, true
  },
  {
    "lang-es", "SPANISH", "Tiếng Tây Ban Nha",
    "Spanish interface language", "Gói ngôn ngữ giao diện tiếng Tây Ban Nha",
    StoreCategory::Languages, "/languages", "es.cplang", 372320, "363.6 KB", true, true
  },
  {
    "lang-sv", "SWEDISH", "Tiếng Thụy Điển",
    "Swedish interface language", "Gói ngôn ngữ giao diện tiếng Thụy Điển",
    StoreCategory::Languages, "/languages", "sv.cplang", 346114, "338.0 KB", true, true
  },
  {
    "lang-th", "THAI", "Tiếng Thái",
    "Thai interface language", "Gói ngôn ngữ giao diện tiếng Thái",
    StoreCategory::Languages, "/languages", "th.cplang", 682918, "666.9 KB", true, true
  },
  {
    "lang-tr", "TURKISH", "Tiếng Thổ Nhĩ Kỳ",
    "Turkish interface language", "Gói ngôn ngữ giao diện tiếng Thổ Nhĩ Kỳ",
    StoreCategory::Languages, "/languages", "tr.cplang", 364786, "356.2 KB", true, true
  },
  {
    "lang-uk", "UKRAINIAN", "Tiếng Ukraina",
    "Ukrainian interface language", "Gói ngôn ngữ giao diện tiếng Ukraina",
    StoreCategory::Languages, "/languages", "uk.cplang", 523105, "510.8 KB", true, true
  },
  {
    "lang-ca-valencia", "VALENCIAN", "Tiếng Valencia",
    "Valencian interface language", "Gói ngôn ngữ giao diện tiếng Valencia",
    StoreCategory::Languages, "/languages", "ca-valencia.cplang", 360471, "352.0 KB", true, true
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
