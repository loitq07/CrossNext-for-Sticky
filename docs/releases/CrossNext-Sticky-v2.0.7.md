# CrossNext for Seeed reTerminal Sticky v2.0.7 (beta)

> **Bản thử nghiệm (prerelease).** Chỉ máy bật **Cài đặt → Hệ thống → Nhận bản thử nghiệm (Beta)** mới được đề nghị cập nhật. Có thể còn lỗi — hãy sao lưu thẻ nhớ trước.
> **Beta build (prerelease).** Only devices with **Settings → System → Beta updates** on are offered it. Expect rough edges — back up your microSD first.

## Trước khi cập nhật / Before you update

1. **Sao lưu thẻ nhớ**, đặc biệt là thư mục ẩn `/.crosspoint/` (tiến độ đọc, cài đặt, dữ liệu app).
   **Back up your microSD**, especially the hidden `/.crosspoint/` folder (progress, settings, app saves).
2. **Nếu nâng cấp từ bản cũ hơn v2.0.5**, tải `CrossNext-Sticky-Resources.zip` đính kèm, giải nén và chép `apps` + `fonts` vào gốc thẻ nhớ trước khi cập nhật (người dùng đang ở v2.0.5 / v2.0.6 giữ nguyên được).
   **If upgrading from older than v2.0.5**, extract `CrossNext-Sticky-Resources.zip` and copy `apps/` and `fonts/` to the microSD root first.
3. **Sau khi cập nhật**, vào Home và chọn **Cập nhật ngay** (cần Wi-Fi): thiết bị sẽ tự động tải các module ứng dụng/trò chơi native tương ứng cho firmware mới **và gói Tiếng Việt của Sticky**.
   **After updating**, accept the app update on Home (needs Wi-Fi): it fetches native apps/games for the new firmware **and the Sticky Vietnamese pack**.

> **Lưu ý cập nhật OTA / OTA note:** sau khi bấm Cập nhật, màn hình có thể **đứng ở 0% khoảng 1 phút** (máy đang xoá vùng nhớ flash cũ) và nút bấm tạm không phản hồi. Đừng tắt hay reset máy — tiến trình sẽ chạy tiếp, toàn bộ mất khoảng 2–3 phút.
> After tapping Update, the screen may **sit at 0% for about a minute** while the old slot is erased, and buttons won't respond. Don't power off or reset — progress resumes on its own; the whole update takes about 2–3 minutes.

---

## Tiếng Việt

### Có gì mới so với v2.0.6 (và v2.0.5):
- **Tinh chỉnh bảng Quick Settings cho Sticky**:
  - **Loại bỏ `Tilt Page Turn`** khỏi Quick Settings và trình tuỳ chỉnh trên Sticky (không có phần cứng/tính năng lật trang bằng nghiêng máy).
  - **Sắp xếp lại 2 hàng (8 nút cân đối) & bổ sung tính năng Chụp ảnh màn hình (`Screenshot`)**:
    - **Hàng 1 (4 nút toggle)**: **Wi-Fi → Bluetooth → Key Sound → Night Mode**
    - **Hàng 2 (4 nút tác vụ)**: **Refresh → Screenshot → Rotate → Sleep**
    - **Chụp ảnh màn hình (`Screenshot`)**: Chạm **Screenshot** trên Quick Settings để tự động đóng bảng, vẽ lại trọn vẹn trang hiện tại rồi lưu ảnh `.bmp` vào `/screenshots/` trên thẻ nhớ kèm viền báo chụp.
  - **Đồng bộ icon Night Mode**: Dùng đúng icon **Trắng/Đen (`Contrast`)** đồng bộ với mục **Chế độ tối** trong **Cài đặt** thay cho icon mặt trăng của **Ngủ (Sleep)**.
  - **Wi-Fi / Bluetooth**: Nhấn nhanh để bật/tắt trực tiếp (Wi-Fi tự kết nối mạng đã lưu gần nhất và tự tắt BLE nếu đang bật); **nhấn giữ** để mở trang chọn mạng Wi-Fi hoặc cài đặt Bluetooth.
- **Sửa lỗi đen trang sách khi kéo cạnh trên lúc đọc EPUB**:
  - `Ssd1677Driver::cleanupGrayscaleBuffers()` trước đây chỉ khôi phục `RED RAM` mà để lại lớp LSB (`0x00`) của bộ khử răng cưa chữ (AA) trong `BW RAM`, khiến thao tác cập nhật cửa sổ nhỏ (`displayWindow`) khi hiện chỉ báo vuốt cạnh làm toàn bộ trang đọc còn lại chuyển sang nền đen. Nay đã khôi phục đồng bộ cả `BW RAM` và `RED RAM`, đồng thời vô hiệu `supportsFastWindow()` khi đang bật Night Mode (`inverted`) hoặc chưa xong pass grayscale.
- **Kế thừa từ v2.0.6**:
  - **Chỉ báo vuốt cạnh màn hình kiểu Boox Picco**: Tai nổi bo tròn kèm icon Back / Home / Menu / Quick Settings dùng `fast window` trên SSD1677.
  - **Sửa lỗi App Store External Apps SDK (`dice.elf`) & Book Preparer Sticky**.

---

## English

### What's New vs v2.0.6 (and v2.0.5):
- **Sticky Quick Settings refinement**:
  - **Removed `Tilt Page Turn`** from Quick Settings and slot customizer on Sticky.
  - **Reordered 2-row 8-tile layout & new Screenshot quick action**:
    - **Row 1**: **Wi-Fi → Bluetooth → Key Sound → Night Mode**
    - **Row 2**: **Refresh → Screenshot → Rotate → Sleep**
    - **Screenshot**: Tapping **Screenshot** dismisses Quick Settings, lets the underlying page repaint cleanly, and captures it as a `.bmp` under `/screenshots/` with the frame confirmation border.
  - **Night Mode icon fix**: Uses the **Black/White `Contrast` icon** synced with **Settings**, replacing the moon icon reserved for **Sleep**.
  - **Quick toggles**: Short-tap toggles Wi-Fi (auto-connecting the last saved network and cleanly releasing BLE if active), Bluetooth, Key Sound, or Night Mode; **long-press** Wi-Fi/Bluetooth opens their full settings screens.
- **Fixed black EPUB reading page on edge swipe**:
  - `Ssd1677Driver::cleanupGrayscaleBuffers()` previously restored only `RED RAM` and left the anti-aliasing LSB plane (`0x00`) in `BW RAM`, causing a partial `displayWindow()` edge indicator over a grayscale book page to invert the rest of the page solid black. Now restores both `BW RAM` and `RED RAM` cleanly, and guards `FreeInkDisplay::supportsFastWindow()` against inverted (`Night Mode`) and unfinished grayscale states.
- **Included from v2.0.6**:
  - **Boox Picco–style edge swipe visual indicator** with SSD1677 fast-window partial rendering.
  - **App Store External Apps SDK fix (`-nostartfiles` / `dice.elf`) & Book Preparer Sticky branding**.
