# CrossNext for Seeed reTerminal Sticky v2.0.8

Bản phát hành chính thức (**Public Release**) tổng hợp toàn bộ nâng cấp từ v2.0.6–v2.0.7 và bổ sung **đầy đủ 38 gói ngôn ngữ giao diện trên App Store** (giống CrossNext gốc).

## Trước khi cập nhật / Before you update

1. **Sao lưu thẻ nhớ**, đặc biệt là thư mục ẩn `/.crosspoint/` (tiến độ đọc, cài đặt, dữ liệu app).
   **Back up your microSD**, especially the hidden `/.crosspoint/` folder (progress, settings, app saves).
2. **Nếu nâng cấp từ bản cũ hơn v2.0.5** (hoặc muốn cài thêm ngôn ngữ chữ tượng hình/Ả Rập/Do Thái/Thái), tải `CrossNext-Sticky-Resources.zip` đính kèm, giải nén và chép `apps/`, `fonts/`, `languages/` vào gốc thẻ nhớ (giữ nguyên các file `*.cplang` đã tải từ App Store).
   **If upgrading from older than v2.0.5** (or if you need non-Latin UI fallback fonts/keyboards), extract `CrossNext-Sticky-Resources.zip` and copy `apps/`, `fonts/`, and `languages/` to the microSD root (keep existing `*.cplang` packs).
3. **Sau khi cập nhật**, vào Home và chọn **Cập nhật ngay** (cần Wi-Fi): thiết bị sẽ tự động tải các module ứng dụng/trò chơi native tương ứng cho firmware mới **và gói ngôn ngữ bạn đang dùng**.
   **After updating**, accept the app update on Home (needs Wi-Fi): it fetches native apps/games for the new firmware **and your active language pack**.

> **Lưu ý cập nhật OTA / OTA note:** sau khi bấm Cập nhật, màn hình có thể **đứng ở 0% khoảng 1 phút** (máy đang xoá vùng nhớ flash cũ) và nút bấm tạm không phản hồi. Đừng tắt hay reset máy — tiến trình sẽ chạy tiếp, toàn bộ mất khoảng 2–3 phút.
> After tapping Update, the screen may **sit at 0% for about a minute** while the old slot is erased, and buttons won't respond. Don't power off or reset — progress resumes on its own; the whole update takes about 2–3 minutes.

---

## Tiếng Việt

### Có gì mới so với v2.0.5:
- **Đầy đủ 38 ngôn ngữ giao diện trên App Store**:
  - Tab **Ngôn ngữ** trong **App Store** nay hiển thị toàn bộ 38 gói ngôn ngữ giống firmware CrossNext gốc (bao gồm Tiếng Việt, Tiếng Ukraina, Tiếng Tây Ban Nha, Pháp, Đức, Nhật, Trung, Hàn/Thái/Ả Rập/Nga...).
  - Sửa menu thao tác cho gói ngôn ngữ đã cài trong App Store (**Tải lại / Xoá / Huỷ**) thay vì hiện nút Mở app.
  - `CrossNext-Sticky-Resources.zip` đính kèm sẵn `languages/fonts/` (font giao diện bổ sung cho chữ non-Latin) và `languages/keyboards/`.
- **Bảng Quick Settings mới cho Sticky & Chụp ảnh màn hình (`Screenshot`)**:
  - Loại bỏ `Tilt Page Turn`, sắp xếp lại 2 hàng 8 nút cân đối:
    - **Hàng 1**: **Wi-Fi → Bluetooth → Key Sound → Night Mode**
    - **Hàng 2**: **Refresh → Screenshot → Rotate → Sleep**
  - **Chụp ảnh màn hình**: Chạm **Screenshot** trên Quick Settings để tự động đóng bảng, vẽ lại trọn vẹn trang hiện tại rồi lưu ảnh `.bmp` vào `/screenshots/` kèm viền báo chụp (sửa lỗi deadlock/đơ máy khi chụp ở bản beta v2.0.7).
  - **Đồng bộ icon Night Mode** (icon Trắng/Đen `Contrast`) và hỗ trợ **nhấn giữ** Wi-Fi/Bluetooth để mở cài đặt chi tiết.
- **Chỉ báo vuốt cạnh màn hình & Sửa lỗi đen trang sách EPUB**:
  - Thêm tai nổi chỉ báo vuốt cạnh kiểu Boox Picco dùng `fast window` trên SSD1677.
  - Sửa `Ssd1677Driver::cleanupGrayscaleBuffers()` và `FreeInkDisplay::supportsFastWindow()` để trang EPUB đang bật khử răng cưa chữ (AA) không bị đảo đen khi vuốt cạnh trên.
- **Sửa lỗi App Store External Apps SDK (`dice.elf`) & Book Preparer Sticky**.

---

## English

### What's New vs v2.0.5:
- **All 38 interface languages on App Store**:
  - The **Languages** tab in **App Store** now lists all 38 CrossNext interface language packs (`vi`, `uk`, `es`, `fr`, `de`, `ja`, `zh-CN`, `zh-TW`, `ru`, `pl`, `ar`, `th`, etc.), compiled per firmware build and downloaded straight to `/languages/`.
  - Fixed installed language-pack actions in App Store to offer **Download again / Delete / Cancel** instead of Open.
  - `CrossNext-Sticky-Resources.zip` now includes `languages/fonts/` (non-Latin UI fallback fonts) and `languages/keyboards/`.
- **Sticky Quick Settings refinement & Screenshot**:
  - Removed `Tilt Page Turn` and organized an 8-tile layout:
    - **Row 1**: **Wi-Fi → Bluetooth → Key Sound → Night Mode**
    - **Row 2**: **Refresh → Screenshot → Rotate → Sleep**
  - **Screenshot**: Tapping **Screenshot** dismisses Quick Settings, lets the page repaint cleanly, and saves a `.bmp` under `/screenshots/` with the confirmation border (fixes the v2.0.7 beta freeze/deadlock when taking a screenshot).
  - **Night Mode Contrast icon** synced with Settings; short-tap toggles Wi-Fi/Bluetooth and **long-press** opens full Wi-Fi/Bluetooth settings.
- **Edge swipe indicator & fixed black EPUB reading page**:
  - Added Boox Picco–style edge swipe indicator using SSD1677 fast-window partial rendering.
  - Fixed `Ssd1677Driver::cleanupGrayscaleBuffers()` and guarded `FreeInkDisplay::supportsFastWindow()` so anti-aliased EPUB pages no longer invert black on top-edge swipes.
- **App Store External Apps SDK fix (`-nostartfiles` / `dice.elf`) & Book Preparer Sticky branding**.
