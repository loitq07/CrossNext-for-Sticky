# CrossNext for Seeed reTerminal Sticky v2.0.9

Bản phát hành chính thức (**Public Release**) bổ sung **Tùy biến Quick Settings (tối đa 3 dòng / 12 mục)** tương tự Home Menu: cho phép chuyển bớt các mục ít dùng từ Home Menu (`Reading Stats`, `File Transfer`, `Plugins`, `Read Later`, `Files`, `Recent Books`, `Games`, `Apps`, `App Store`, `Settings`, `USB Drive`), ghim tối đa **4 lối tắt `+ Add Game` / `+ Add App`** lên bảng Quick Settings, và sửa nhãn hiển thị gọn đẹp.

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

### Có gì mới ở v2.0.9:
- **Tùy biến Quick Settings toàn diện (tối đa 3 hàng ~ 12 mục)**:
  - Vào **Cài đặt → Hiển thị → Quick Settings** (`Settings → Display → Quick Settings`) để **Ẩn / Hiện mục**, **Sắp xếp thứ tự**, **`+ Add Game`** và **`+ Add App`** tương tự Home Menu.
  - Hỗ trợ đưa các mục từ Home Menu lên bảng kéo xuống: **Reading Stats**, **File Transfer**, **Plugins** (hoặc **OPDS Browser**), **Read Later**, **Browse Files**, **Recent Books**, **Games**, **Apps**, **App Store**, **Settings**, **USB Drive** cùng tối đa **4 lối tắt Game/App ghim trực tiếp**.
  - Cấu hình được lưu riêng an toàn tại `/.crosspoint/quick-menu-{a,b}.bin`; giữ mặc định 8 nút (2 hàng) cho Sticky và tự mở rộng 3 hàng khi bật từ 9–12 mục.
- **Sửa nhãn hiển thị Quick Settings**:
  - Ô **Bluetooth** hiển thị gọn **BLUETOOTH / Bluetooth** (sửa lỗi `BluetoothOff`).
  - Ô **Screenshot** hiển thị gọn **Screenshot** (thay cho `TAKE SCREENSHOT`).
- **Kế thừa đầy đủ từ v2.0.8**:
  - Đủ 38 ngôn ngữ trên App Store, tai nổi chỉ báo vuốt cạnh, sửa lỗi đen trang đọc sách EPUB khi bật khử răng cưa chữ (AA).

---

## English

### What's New in v2.0.9:
- **Home-Menu–Style Quick Settings Customization (up to 3 rows / 12 tiles)**:
  - Open **Settings → Display → Quick Settings** to **Show / Hide Items**, **Reorder** (`Row X · Slot Y`), **`+ Add Game`**, and **`+ Add App`** just like Home Menu customization.
  - Move less-used Home Menu items onto Quick Settings: **Reading Stats**, **File Transfer**, **Plugins** (or **OPDS Browser**), **Read Later**, **Browse Files**, **Recent Books**, **Games**, **Apps**, **App Store**, **Settings**, **USB Drive**, plus up to **4 pinned Game/App shortcuts**.
  - Stored safely via ping-pong persistence (`/.crosspoint/quick-menu-{a,b}.bin`); keeps the 8-tile (2-row) Sticky default and dynamically expands up to 3 rows (12 tiles) in both portrait and landscape orientations.
- **Clean Quick Settings captions**:
  - Fixed **Bluetooth** caption (`BLUETOOTH` / `Bluetooth`, removing the `Off` suffix).
  - Fixed **Screenshot** caption (`Screenshot` in English instead of `TAKE SCREENSHOT`).
- **Includes everything from v2.0.8**:
  - All 38 interface languages on App Store, Boox-style edge swipe indicator, and fixed AA EPUB black-page rendering.
