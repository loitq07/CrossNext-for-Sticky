# CrossNext for Seeed reTerminal Sticky v2.0.5

Bản phát hành chính thức nâng cấp giao diện toàn diện cho **23 trò chơi Pocket Play** trên Seeed reTerminal Sticky: đồng bộ hoàn toàn theo hệ thống thiết kế CrossNext Design System (CDS), mang lại trải nghiệm liền mạch, sắc nét và hiện đại trên màn hình E-Ink.

## Trước khi cập nhật / Before you update

1. **Sao lưu thẻ nhớ**, đặc biệt là thư mục ẩn `/.crosspoint/` (tiến độ đọc, cài đặt, dữ liệu app).
   **Back up your microSD**, especially the hidden `/.crosspoint/` folder (progress, settings, app saves).
2. **Chép tài nguyên thẻ nhớ** *trước* khi cập nhật firmware: tải `CrossNext-Sticky-Resources.zip` đính kèm trong release này, giải nén và chép thư mục `apps` và `fonts` vào gốc thẻ nhớ (thay thế thư mục cũ).
   **Copy microSD resources** *before* updating firmware: download `CrossNext-Sticky-Resources.zip` attached to this release, extract and copy `apps` and `fonts` to the microSD root.
3. **Sau khi cập nhật**, vào Home và chọn **Cập nhật ngay** (cần Wi-Fi): thiết bị sẽ tự động tải các module ứng dụng/trò chơi native tương ứng cho firmware mới **và gói Tiếng Việt của Sticky**.
   **After updating**, accept the app update on Home (needs Wi-Fi): it fetches native apps/games for the new firmware **and the Sticky Vietnamese pack**.

> **Lưu ý cập nhật OTA / OTA note:** sau khi bấm Cập nhật, màn hình có thể **đứng ở 0% khoảng 1 phút** (máy đang xoá vùng nhớ flash cũ) và nút bấm tạm không phản hồi. Đừng tắt hay reset máy — tiến trình sẽ chạy tiếp, toàn bộ mất khoảng 2–3 phút.
> After tapping Update, the screen may **sit at 0% for about a minute** while the old slot is erased, and buttons won't respond. Don't power off or reset — progress resumes on its own; the whole update takes about 2–3 minutes.

---

## Tiếng Việt

### Tính năng mới & Cải tiến trong v2.0.5:
- **Đồng bộ chuẩn thiết kế CrossNext CDS cho toàn bộ 23 game Pocket Play**:
  - **Thanh Header chuẩn CDS**: Nâng chiều cao chuẩn 72px (56px trong chế độ xoay ngang), nền đen tuyền sang trọng với đường kẻ phân cách 2px.
  - **Nút Back dạng mũi tên chevron (`←`)**: Thay thế nút chữ nhật "BACK" cũ bằng mũi tên chevron chuẩn nét 4px của CrossNext, tích hợp vùng chạm cảm ứng 80x72px (`HeaderBackTapTarget`).
  - **Cụm trạng thái thông minh**: Tích hợp icon pin, phần trăm pin thực tế và đồng hồ hệ thống (`HH:MM`) trực tiếp trên thanh header của tất cả trò chơi.
  - **Các nút bấm (Buttons & Pills) bo góc chuẩn CDS**: Bo góc mềm mại `r = 6px` (`kRadiusControl`), viền nét 2px (`kStrokeBorder`), tối ưu hoá căn chỉnh chữ (optical ink-centering) và hiển thị tương phản cao chống lưu ảnh (ghosting) trên E-Ink.
  - **Danh sách hàng (Row items) & Nút vô hiệu hoá (Dimmed pills)**: Chuẩn hoá viền hairline 1px, dither 50% chống bóng mờ.
- **Trọn bộ 23 tựa game mới từ kho Pocket Play trên App Store**:
  - Sokoban, Text Adventures (Zork I/II/III), Suguru, Queens, D&Diagrams, Nonogram, Kakuro, Chess Puzzles, Tic-Tac-Toe, Peg Solitaire, Lights Out, Word Ladder, One Clue, Dice Roller, Bingo Caller, Symbol Dice, Island Maps, Think Alike, Authority Tracker, Polyomino Panic, Crown & Crest, Last Piece, Grid Gambit.

---

## English

### What's New & Improvements in v2.0.5:
- **CrossNext CDS Design System synchronization for all 23 Pocket Play games**:
  - **Standard CDS Header**: Unified 72px height (56px in landscape), solid black band with 2px rule divider.
  - **Native Chevron Back Arrow (`←`)**: Replaced legacy rectangular "BACK" button with authentic 4px-thick chevron arrow and 80x72px touch hit target (`HeaderBackTapTarget`).
  - **Status Cluster**: Dynamic battery glyph, live battery percentage, and real-time clock (`HH:MM`) integrated into the game header bar.
  - **Rounded Controls (Buttons & Pills)**: Standardized corner radius `r = 6px` (`kRadiusControl`) and 2px stroke border (`kStrokeBorder`) with crisp font fallback and optical ink-centering.
  - **Dimmed & Row Elements**: 1px hairline border with 50% dithering pattern for e-ink ghosting resistance.
- **23 Pocket Play games on App Store**: Full puzzle and adventure collection with native `.sav` autosaves.
