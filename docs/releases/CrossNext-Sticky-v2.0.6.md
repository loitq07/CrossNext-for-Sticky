# CrossNext for Seeed reTerminal Sticky v2.0.6 (beta)

> **Bản thử nghiệm (prerelease).** Chỉ máy bật **Cài đặt → Hệ thống → Nhận bản thử nghiệm (Beta)** mới được đề nghị cập nhật. Có thể còn lỗi — hãy sao lưu thẻ nhớ trước.
> **Beta build (prerelease).** Only devices with **Settings → System → Beta updates** on are offered it. Expect rough edges — back up your microSD first.

## Trước khi cập nhật / Before you update

1. **Sao lưu thẻ nhớ**, đặc biệt là thư mục ẩn `/.crosspoint/` (tiến độ đọc, cài đặt, dữ liệu app).
   **Back up your microSD**, especially the hidden `/.crosspoint/` folder (progress, settings, app saves).
2. **Nếu nâng cấp từ bản cũ hơn v2.0.5**, tải `CrossNext-Sticky-Resources.zip` đính kèm, giải nén và chép `apps` + `fonts` vào gốc thẻ nhớ trước khi cập nhật (người dùng đang ở v2.0.5 giữ nguyên được).
   **If upgrading from older than v2.0.5**, extract `CrossNext-Sticky-Resources.zip` and copy `apps/` and `fonts/` to the microSD root first.
3. **Sau khi cập nhật**, vào Home và chọn **Cập nhật ngay** (cần Wi-Fi): thiết bị sẽ tự động tải các module ứng dụng/trò chơi native tương ứng cho firmware mới **và gói Tiếng Việt của Sticky**.
   **After updating**, accept the app update on Home (needs Wi-Fi): it fetches native apps/games for the new firmware **and the Sticky Vietnamese pack**.

> **Lưu ý cập nhật OTA / OTA note:** sau khi bấm Cập nhật, màn hình có thể **đứng ở 0% khoảng 1 phút** (máy đang xoá vùng nhớ flash cũ) và nút bấm tạm không phản hồi. Đừng tắt hay reset máy — tiến trình sẽ chạy tiếp, toàn bộ mất khoảng 2–3 phút.
> After tapping Update, the screen may **sit at 0% for about a minute** while the old slot is erased, and buttons won't respond. Don't power off or reset — progress resumes on its own; the whole update takes about 2–3 minutes.

---

## Tiếng Việt

### Có gì mới so với v2.0.5:
- **Panel Quick Settings 2×4 với các nút toggle trực tiếp trên Sticky**:
  - Vuốt từ cạnh trên xuống để mở Quick Settings trên Sticky (mặc dù không có đèn nền), hiển thị **2 hàng × 4 nút (8 chức năng)**.
  - **Wi-Fi**: nhấn nhanh để bật (tự kết nối mạng đã lưu gần nhất, tự tắt BLE nếu đang bật để tránh xung đột radio) hoặc tắt ngay; **nhấn giữ** (hoặc chạm icon Wi-Fi trên thanh trạng thái) để mở danh sách mạng Wi-Fi.
  - **Bluetooth**: nhấn nhanh để bật/tắt BLE remote; **nhấn giữ** (hoặc chạm banner Bluetooth) để mở Cài đặt Bluetooth.
  - **Âm phím bấm (`Key Sound`)**: bật/tắt nhanh âm còi buzzer khi thao tác, kèm tiếng click xác nhận khi bật.
  - **Nghiêng để lật trang (`Tilt`)**: bật/tắt nhanh cảm biến nghiêng lật trang ngay trên panel.
  - Đầy đủ các nút **Chế độ tối (Dark)**, **Làm mới (Refresh)**, **Xoay màn hình (Rotate)**, **Ngủ (Sleep)** và hỗ trợ tuỳ chỉnh 4 ô hàng đầu trong **Cài đặt → Hiển thị → Tuỳ chỉnh Quick Settings** (tự ẩn các mục không có phần cứng như Đèn nền/Khoá Home).
- **Chỉ báo trực quan khi vuốt cạnh màn hình (kiểu Boox Picco)**:
  - Khi chạm và kéo từ các cạnh trái / phải / dưới / trên, màn hình hiện tai nổi bo tròn cùng icon **Back**, **Home**, **Menu**, hoặc **Quick Settings** chạy theo vùng `fast window` của màn hình E-Ink SSD1677 và tự xoá sạch khi kết thúc thao tác.
- **Sửa lỗi App Store & External Apps SDK**:
  - Truyền `-nostartfiles` khi biên dịch ứng dụng ngoài và cập nhật lại `dice.elf` trong catalog để mở game ổn định.
- **Book Preparer Desktop**:
  - Đổi nhãn chân trang sidebar từ X4 Pro sang **Sticky**.

---

## English

### What's New vs v2.0.5:
- **Multi-toggle 2×4 Quick Settings panel for Sticky**:
  - Swipe down from the top edge on touch devices without a frontlight (Sticky) to open an **8-tile (2×4) Quick Settings sheet**.
  - **Wi-Fi**: short tap toggles Wi-Fi on (auto-connecting to the last saved credential and cleanly releasing BLE if active) or off; **long-press** (or tap the header Wi-Fi icon) opens network selection.
  - **Bluetooth**: short tap toggles BLE remote on/off; **long-press** (or tap the Bluetooth status banner) opens Bluetooth settings.
  - **Key Sound**: new quick toggle for buzzer click feedback with audible confirmation when enabled.
  - **Tilt Page Turn**: new quick toggle for IMU tilt-to-turn-page.
  - Includes **Dark Mode**, **Full Refresh**, **Rotate**, and **Sleep**, plus hardware-filtered slot customization under **Settings → Display → Quick Settings**.
- **Boox Picco–style edge swipe indicator**:
  - Live floating rounded pill with **Back / Home / Menu / Quick Settings** glyph rendered smoothly via SSD1677 fast-window partial updates while swiping from screen edges.
- **App Store & External Apps SDK fix**:
  - Added `-nostartfiles` to `tools_local/external_apps_sdk/build_app.py` and updated `dice.elf` size in `AppStoreCatalog.h`.
- **Book Preparer Desktop**:
  - Rebranded sidebar footer from X4 Pro to **Sticky**.
