# CrossNext for Seeed reTerminal Sticky v2.0.3

Bản phát hành chính thức đưa **CrossNext 2.0** lên Seeed reTerminal Sticky, mang lại toàn bộ hệ thống đọc sách, thư viện, quản lý ứng dụng và kết nối Bluetooth mới nhất cùng các tinh chỉnh dành riêng cho phần cứng Sticky.

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

### Tính năng mới & Cải tiến trong v2.0.3:
- **Dòng kẻ chấm dưới chữ trong sách (Dotted Line Guides)**:
  - Bật/tắt trong thanh cài đặt chữ khi đọc sách (hoặc **Cài đặt → Đọc → Cài đặt chữ → Kiểu chữ → Dòng kẻ chấm dưới chữ**).
  - Vẽ dòng chấm mảnh trang vở dưới mỗi dòng chữ, giúp mắt dễ bám dòng khi đọc các đoạn văn dài hoặc đọc sách font chữ nhỏ.
  - Tự động canh theo lề cột và khoảng cách dòng; lưu riêng theo cài đặt hiển thị toàn cục.

### Toàn bộ nâng cấp từ nền tảng CrossNext 2.0:
- **Engine EPUB mới**: Bố cục nhanh hơn, tái sử dụng chương, độ rộng từ đã tính; hỗ trợ cài đặt chữ riêng cho từng cuốn sách.
- **Remote Bluetooth**: Ghép remote lật trang BLE, gán phím ngắn/giữ riêng, hỗ trợ 2 shortcut, ô Bluetooth trong Quick settings.
- **Thư viện sách mới**: Yêu thích, bộ sưu tập, menu giữ sách gộp Đánh dấu đã đọc/chưa đọc, Xoá khỏi Gần đây, Xoá, Dựng lại thư viện.
- **Thống kê đọc sách**: Theo dõi tiến độ đọc theo ngày, tháng, năm và lịch sử từng cuốn; ứng dụng FOCUS hỗ trợ phiên tập trung.
- **Cửa hàng ứng dụng (App Store) tối ưu**:
  - Tách riêng 5 tab: **Tất cả**, **Trò chơi**, **Ứng dụng**, **Ngôn ngữ**, **Tài nguyên**.
  - Tải trực tiếp gói tiếng Việt và gói tài nguyên đồ họa thẻ nhớ (PET, Reading Quest, Intercept) qua Wi-Fi với các nút Tải lại / Xoá tiện lợi.
  - Tên tab hiển thị gọn gàng trên 1 dòng với dấu `...` khi nhãn dài.
- **Quản lý phông chữ đọc sách độc lập**: Tải phông chữ qua Wi-Fi trong **Cài đặt → Đọc → Quản lý phông chữ**, tự động nhận diện cả `Noto Serif` và `NotoSerif`.
- **Trò chơi & Ứng dụng kệ ngoài**: Đăng ký 14 game và app mới vào kệ Home có icon và tên tiếng Việt; bàn chơi Hearts thiết kế dạng la bàn với điểm số từng ván.
- **Tối ưu phần cứng Sticky**: Màn hình IC SSD2677 không bị lật gương; dịch tay hơn 3.100 chuỗi tiếng Việt chuẩn xác; loại bỏ các thiết lập phần cứng không có trên Sticky.

---

## English

### What's New in v2.0.3:
- **Dotted Line Guides in Reader**:
  - Toggle in the reader text sheet or under **Settings → Reading → Text settings → Style → Dotted Line Guides**.
  - Draws a subtle notebook-style dotted rule under each text line, making it easier to track lines during long reading sessions.
  - Anchors cleanly to the column margins and line baselines without modifying page layout.

### Complete CrossNext 2.0 Highlights:
- **New EPUB Engine**: Faster layout reuse, word width caching, and per-book text settings.
- **Bluetooth Remotes**: Pair BLE page-turners, map short and long presses, shortcuts, and Bluetooth Quick settings tile.
- **Redesigned Library**: Favorites, collections, unified long-press book menu (Mark read/unread, Remove from recents, Rebuild library).
- **Reading Statistics & Focus**: Daily, monthly, and yearly stats with per-book tracking; FOCUS app for dedicated reading sessions.
- **Enhanced App Store**:
  - 5 dedicated tabs: **All**, **Games**, **Apps**, **Languages**, and **Resources**.
  - Direct Wi-Fi installation and management of microSD resource packs (PET, Reading Quest, Intercept) and language packs.
  - Clean single-line tab bars with automatic truncation.
- **Independent Font Downloads**: Download fonts via Wi-Fi under **Settings → Reading → Manage fonts**, with automatic fallback for bundled and downloaded Noto Serif.
- **Shelf Games & Apps**: 14 new games/apps integrated onto the shelf with native icons and Vietnamese labels; CrossPlay-style Hearts board.
- **Sticky Hardware Optimizations**: SSD2677 display support without mirroring; complete handcrafted Vietnamese localization (3,100+ strings); pruned settings for unpopulated hardware.
