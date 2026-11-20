# CrossNext for Seeed reTerminal Sticky v2.0.4

Bản phát hành chính thức nâng cấp kho ứng dụng **App Store** trên Seeed reTerminal Sticky: tích hợp thêm **23 trò chơi giải đố & nhập vai mới** từ bộ sưu tập Pocket Play của Seeed Playground, cùng các cải tiến toàn diện về trải nghiệm đọc sách và hệ thống CrossNext 2.0.

## Trước khi cập nhật / Before you update

1. **Sao lưu thẻ nhớ**, đặc biệt là thư mục ẩn `/.crosspoint/` (tiến độ đọc, cài đặt, dữ liệu app).
   **Back up your microSD**, especially the hidden `/.crosspoint/` folder (progress, settings, app saves).
2. **Chép tài nguyên thẻ nhớ** *trước* khi cập nhật firmware: tải `CrossNext-Sticky-Resources.zip` đính kèm trong release này, giải nén và chép thư mục `apps` và `fonts` vào gốc thẻ nhớ (thay thế thư mục cũ). Gói chứa tài nguyên đồ họa cho PET, Reading Quest, The Intercept và các phông chữ đọc sách (Noto Serif…); từ bản 2.0 chúng nằm trên thẻ nhớ thay vì nạp cứng vào firmware. **Không** chép thư mục `languages` từ gói CrossNext gốc (giữ sách và thư mục `/.crosspoint/`).
   **Copy microSD resources** *before* updating firmware: download `CrossNext-Sticky-Resources.zip` attached to this release, extract and copy `apps` and `fonts` to the microSD root (replace existing folders). It contains artwork for PET, Reading Quest, The Intercept and reader fonts (Noto Serif…), which live on microSD starting in 2.0.
3. **Sau khi cập nhật**, vào Home và chọn **Cập nhật ngay** (cần Wi-Fi): thiết bị sẽ tự động tải các module ứng dụng/trò chơi native tương ứng cho firmware mới **và gói Tiếng Việt của Sticky**.
   **After updating**, accept the app update on Home (needs Wi-Fi): it fetches native apps/games for the new firmware **and the Sticky Vietnamese pack**.

> **Lưu ý cập nhật OTA / OTA note:** sau khi bấm Cập nhật, màn hình có thể **đứng ở 0% khoảng 1 phút** (máy đang xoá vùng nhớ flash cũ) và nút bấm tạm không phản hồi. Đừng tắt hay reset máy — tiến trình sẽ chạy tiếp, toàn bộ mất khoảng 2–3 phút.
> After tapping Update, the screen may **sit at 0% for about a minute** while the old slot is erased, and buttons won't respond. Don't power off or reset — progress resumes on its own; the whole update takes about 2–3 minutes.
>
> **PET và Reading Quest bắt đầu lại từ đầu**: hai tính năng được làm lại hoàn toàn trong CrossNext 2.0, dữ liệu cũ không tương thích.
> **PET and Reading Quest start over**: both were redesigned in CrossNext 2.0 and old saves are not compatible.

---

## Tiếng Việt

### Tính năng mới & Cải tiến trong v2.0.4:
- **Bổ sung 23 tựa game mới từ kho Pocket Play trên App Store**:
  - Hỗ trợ tải trực tiếp qua Wi-Fi từ App Store hoặc chép thủ công vào thẻ nhớ microSD (`/games/*.elf`).
  - Danh sách 23 game mới:
    1. **Sokoban**: Đẩy hộp vào đích với 385 màn giải đố kinh điển.
    2. **Text Adventures (Z-Machine)**: Game phiêu lưu chữ tương tác chạy trọn bộ huyền thoại Zork I, II, III.
    3. **Suguru**: 800 câu đố điền số theo vùng logic hấp dẫn.
    4. **Queens (Star Battle)**: 1200 câu đố đặt quân hậu và ngôi sao.
    5. **D&Diagrams**: 64 mê cung hầm ngục logic phong cách D&D.
    6. **Nonogram & Kakuro**: Hàng trăm màn tranh giải đố số và điền tổng chéo logic.
    7. **Chess Puzzles & Last Piece & Grid Gambit**: Câu đố chiến thuật cờ vua Lichess và cờ tàn.
    8. **Tic-Tac-Toe, Peg Solitaire, Lights Out, Word Ladder, One Clue, Dice Roller, Bingo Caller, Symbol Dice, Island Maps, Think Alike, Authority Tracker, Polyomino Panic, Crown & Crest**.
  - Tự động lưu tiến trình game vào `/.pocketplay/<game>.sav` trên thẻ nhớ SD.

### Toàn bộ nâng cấp từ nền tảng CrossNext 2.0:
- **Dòng kẻ chấm dưới chữ trong sách (Dotted Line Guides)**: Bật/tắt trong thanh cài đặt chữ khi đọc sách (hoặc **Cài đặt → Đọc → Cài đặt chữ → Kiểu chữ → Dòng kẻ chấm dưới chữ**).
- **Engine EPUB mới**: Bố cục nhanh hơn, tái sử dụng chương, độ rộng từ đã tính; hỗ trợ cài đặt chữ riêng cho từng cuốn sách.
- **Remote Bluetooth**: Ghép remote lật trang BLE, gán phím ngắn/giữ riêng, hỗ trợ 2 shortcut, ô Bluetooth trong Quick settings.
- **Thư viện sách mới**: Yêu thích, bộ sưu tập, menu giữ sách gộp Đánh dấu đã đọc/chưa đọc, Xoá khỏi Gần đây, Xoá, Dựng lại thư viện.
- **Thống kê đọc sách**: Theo dõi tiến độ đọc theo ngày, tháng, năm và lịch sử từng cuốn; ứng dụng FOCUS hỗ trợ phiên tập trung.
- **Cửa hàng ứng dụng (App Store) tối ưu**:
  - Tách riêng 5 tab: **Tất cả**, **Trò chơi**, **Ứng dụng**, **Ngôn ngữ**, **Tài nguyên**.
  - Tải trực tiếp gói tiếng Việt và gói tài nguyên đồ họa thẻ nhớ qua Wi-Fi.
- **Tối ưu phần cứng Sticky**: Màn hình IC SSD2677 không bị lật gương; dịch tay hơn 3.100 chuỗi tiếng Việt chuẩn xác.

---

## English

### What's New in v2.0.4:
- **23 New Games from Pocket Play on the App Store**:
  - Available for one-tap Wi-Fi download in the App Store or direct placement on microSD (`/games/*.elf`).
  - Includes: Sokoban (385 levels), Text Adventures (Z-Machine running Zork I-III), Suguru (800 puzzles), Queens (1200 Star Battle puzzles), D&Diagrams (64 dungeon logic puzzles), Nonogram, Kakuro, Chess Puzzles (2398 Lichess tactics), Last Piece, Grid Gambit, Tic-Tac-Toe, Peg Solitaire, Lights Out, Word Ladder, One Clue, Dice Roller, Bingo Caller, Symbol Dice, Island Maps, Think Alike, Authority Tracker, Polyomino Panic, Crown & Crest.
  - Automatic game state persistence under `/.pocketplay/<game>.sav` on microSD.

### Complete CrossNext 2.0 Highlights:
- **Dotted Line Guides in Reader**: Toggle in reader style settings for notebook-like reading guidance.
- **New EPUB Engine**: Faster layout reuse, word width caching, and per-book text settings.
- **Bluetooth Remotes**: Pair BLE page-turners, map short and long presses, shortcuts, and Bluetooth Quick settings tile.
- **Redesigned Library**: Favorites, collections, unified long-press book menu.
- **Reading Statistics & Focus**: Daily, monthly, and yearly stats with per-book tracking; FOCUS app.
- **Enhanced App Store**: 5 dedicated tabs (**All**, **Games**, **Apps**, **Languages**, **Resources**), clean single-line tabs.
- **Sticky Hardware Optimizations**: Native SSD2677 display support without mirroring; complete handcrafted Vietnamese localization.
