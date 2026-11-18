# CrossNext for Seeed reTerminal Sticky v2.0.0 (beta)

> **Bản thử nghiệm (prerelease).** Chỉ máy bật **Cài đặt → Hệ thống → Nhận bản thử nghiệm (Beta)** mới được đề nghị cập nhật. Có thể còn lỗi — hãy sao lưu thẻ nhớ trước.
> **Beta build (prerelease).** Only devices with **Settings → System → Beta updates** on are offered it. Expect rough edges — back up your microSD first.

## Trước khi cập nhật / Before you update

1. **Sao lưu thẻ nhớ**, kể cả thư mục ẩn `/.crosspoint/` (tiến độ đọc, cài đặt, dữ liệu app).
   **Back up the microSD**, including the hidden `/.crosspoint/` folder (progress, settings, app saves).
2. **Cài gói Resources của CrossNext 2.0** *trước* khi cập nhật firmware: tải `CrossNext-Resources-NEW.zip` từ [CrossNext 2.0.3](https://github.com/manuelx900/CrossNext/releases/tag/2.0.3), xoá các thư mục tài nguyên cũ (`apps`, `fonts`, `languages`, `images`, `dictionaries`) rồi giải nén vào gốc thẻ nhớ. Giữ lại sách và `/.crosspoint/`. Font đọc sách (Noto Serif/Noto Sans…) và hình PET/Reading Quest giờ nằm trên thẻ nhớ; Ubuntu vẫn là font dự phòng tích hợp.
   **Install the CrossNext 2.0 Resources** *before* the firmware: download `CrossNext-Resources-NEW.zip` from [CrossNext 2.0.3](https://github.com/manuelx900/CrossNext/releases/tag/2.0.3), remove the old bundled folders (`apps`, `fonts`, `languages`, `images`, `dictionaries`) and extract it at the card root. Keep your books and `/.crosspoint/`. Reading fonts and the PET/Reading Quest artwork now live on the card; Ubuntu remains the built-in fallback font.
3. **Sau khi cập nhật**, vào Home và chọn **Cập nhật ngay** (cần Wi-Fi): máy tải lại game/app native cho firmware mới **và gói Tiếng Việt của Sticky** (ghi đè bản tiếng Việt trong gói Resources).
   **After updating**, accept the app update on Home (needs Wi-Fi): it fetches the native games/apps for the new firmware **and the Sticky Vietnamese pack** (replacing the one in the Resources pack).

> **Lưu ý OTA:** màn hình có thể đứng ở 0% khoảng 1 phút trong lúc xoá vùng nhớ cũ — đừng tắt máy.
> **OTA note:** the screen may sit at 0% for about a minute while the old slot is erased — don't power off.
>
> **PET và Reading Quest bắt đầu lại từ đầu**: hai tính năng được làm lại hoàn toàn trong CrossNext 2.0, dữ liệu cũ không tương thích.
> **PET and Reading Quest start over**: both were redesigned in CrossNext 2.0 and old saves are not compatible.

## Tiếng Việt

Bản Sticky chuyển lên nền **CrossNext 2.0.3** (dựa trên CrossPoint 2.0.0), vẫn giữ các tính năng riêng của Sticky: driver màn hình mới tự nhận diện, font Baemin Daniel và tiếng Việt đầy đủ dấu, App Store với native module, gói tiếng Việt trên thẻ nhớ, Read Later, highlight trên màn hình ngủ, âm phím, Plugin Hub và plugin cloud.

- **Engine EPUB mới** bên cạnh **Classic (1.4)** (Cài đặt → Đọc → EPUB engine), tái sử dụng chương, độ rộng từ và bố cục đã tính. **Cài đặt chữ riêng cho từng sách**.
- **Remote Bluetooth**: ghép remote lật trang BLE, gán riêng bấm ngắn/giữ, hai phím tắt, ô Bluetooth trong Quick settings. Thay cho chế độ bàn phím BLE cũ của Sticky (remote đã ghép được giữ nguyên).
- **Thư viện**: yêu thích, bộ sưu tập, menu giữ sách gộp Đánh dấu đã đọc/chưa đọc, Xoá khỏi Gần đây, Xoá, Dựng lại thư viện, Yêu thích, Thêm vào bộ sưu tập.
- **Thống kê đọc** theo ngày, tháng, năm và lịch sử từng cuốn; **Focus** (phiên tập trung) cài từ App Store.
- **Plugin**: trang chi tiết, kiểm tra tương thích, giữ để gỡ; duyệt thư mục cloud (Dropbox/Google Drive) vẫn hoạt động.
- **Wordle** nhiều ngôn ngữ, bàn phím mới, icon game và Home mới.
- **App Store** có thêm Connections, Trivia, Wikipedia, Hacker News, xkcd, Knucklebones, Yahtzee và các game khác dưới dạng native module.
- Voice Notes vẫn tạm ẩn trong lúc sửa lỗi.

## English

The Sticky port moves onto **CrossNext 2.0.3** (based on CrossPoint 2.0.0) and keeps every Sticky feature: the auto-detected new display driver, Baemin Daniel and full Vietnamese typography, the App Store with native modules, the microSD Vietnamese pack, Read Later, highlights on the sleep screen, key sounds, Plugin Hub and cloud plugins.

- **New EPUB engine** alongside **Classic (1.4)** (Settings → Reading → EPUB engine), reusing parsed chapters, word widths and layouts. **Per-book text settings**.
- **Bluetooth remotes**: pair a BLE page turner, map short presses and holds separately, two shortcuts, a Bluetooth tile in Quick settings. Replaces the Sticky's earlier BLE keyboard mode (a paired remote is carried over).
- **Library**: favourites, collections, and one book menu with Mark read/unread, Remove from recents, Delete, Rebuild library, Favourite and Add to collection.
- **Reading statistics** by day, month and year with per-book history; **Focus** sessions from the App Store.
- **Plugins**: item details, compatibility checks, hold to uninstall; cloud folder browsing (Dropbox/Google Drive) still works.
- Multilingual **Wordle**, new keyboards, new game and Home icons.
- The **App Store** adds Connections, Trivia, Wikipedia, Hacker News, xkcd, Knucklebones, Yahtzee and more as native modules.
- Voice Notes stays hidden while it is being fixed.

## Files

- `firmware-sticky.bin`: OTA update asset for the built-in updater.
- `CrossNext-Sticky-v2.0.0-app.bin`: the same application image.
- `CrossNext-Sticky-v2.0.0-factory.bin`: full factory image for USB installation.
- `CrossNext-Sticky-v2.0.0.zip`: firmware bundle, checksums, manifest and license notices.
- `vi.cplang`: Vietnamese interface language pack for this firmware (the App Store installs it over Wi-Fi; offline, copy it to `/languages/`).
- `SHA256SUMS.txt`: SHA-256 checksums for the release assets.

## Validation

Built for `gh_release_sticky` (ESP32-S3, 16MB Flash, 8MB PSRAM).
