# CrossNext for Seeed reTerminal Sticky v2.0.0 (beta)

> **Bản thử nghiệm (prerelease).** Chỉ máy bật **Cài đặt → Hệ thống → Nhận bản thử nghiệm (Beta)** mới được đề nghị cập nhật. Có thể còn lỗi — hãy sao lưu thẻ nhớ trước.
> **Beta build (prerelease).** Only devices with **Settings → System → Beta updates** on are offered it. Expect rough edges — back up your microSD first.

## Trước khi cập nhật / Before you update

1. **Sao lưu thẻ nhớ**, kể cả thư mục ẩn `/.crosspoint/` (tiến độ đọc, cài đặt, dữ liệu app).
   **Back up the microSD**, including the hidden `/.crosspoint/` folder (progress, settings, app saves).
2. **Chép tài nguyên thẻ nhớ** *trước* khi cập nhật firmware: tải `CrossNext-Sticky-Resources.zip` trong release này, giải nén rồi chép thư mục `apps` và `fonts` vào gốc thẻ nhớ (thay thế, không trộn với thư mục cũ). Gói có hình cho PET, Reading Quest, The Intercept và các font đọc sách (Noto Serif…); từ bản 2.0 chúng nằm trên thẻ nhớ thay vì trong firmware. **Không** chép thư mục `languages` từ gói CrossNext gốc: gói đó ghi đè gói tiếng Việt của Sticky bằng bản không khớp firmware. Giữ lại sách và `/.crosspoint/`.
   **Copy the microSD resources** *before* the firmware: download `CrossNext-Sticky-Resources.zip` from this release, extract it and copy its `apps` and `fonts` folders to the card root (replace, do not merge, old ones). It holds the PET, Reading Quest and The Intercept artwork and the reader fonts (Noto Serif…), which since 2.0 live on the card instead of in the firmware. Do **not** copy `languages` from the upstream CrossNext package: it replaces the Sticky Vietnamese pack with one that does not match this firmware. Keep your books and `/.crosspoint/`.
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

**Sửa riêng cho Sticky trong bản beta này**
- Màn hình bản phần cứng mới (IC SSD2677) không còn bị lật gương trái–phải.
- Dịch lại toàn bộ giao diện tiếng Việt bằng tay (hơn 3.100 chuỗi), bỏ các câu dịch máy sai nghĩa.
- Trang Truyền tệp trên web không còn bị dồn sang trái; mục **Quản lý phông chữ** (tải font qua Wi-Fi) trở lại trong Cài đặt → Đọc.
- Bluetooth bật được khi đang dùng Wi-Fi; sửa nhãn pin, trang Cài đặt → Hệ thống bị tràn, vuốt nhầm về Home và vị trí nút Back.
- Bỏ các tuỳ chọn phần cứng Sticky không có (vuốt cạnh chỉnh đèn, đèn nền, nút mặt trước…); sửa header bị vẽ đôi trong Plugin Hub; highlight trên màn hình ngủ không còn mất chữ khi thẻ nhớ chưa có Noto Serif.

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

**Sticky-specific fixes in this beta**
- The new hardware revision's display (SSD2677) is no longer mirrored left-to-right.
- The whole Vietnamese interface was re-translated by hand (3,100+ strings), replacing machine translations that read wrong.
- The web File Transfer page no longer collapses to the left; **Manage fonts** (download fonts over Wi-Fi) is back under Settings → Reading.
- Bluetooth turns on while Wi-Fi is in use; fixed the battery label, the overflowing Settings → System page, accidental swipes to Home and the Back button position.
- Removed options for hardware the Sticky lacks (edge-swipe light, frontlight, front buttons…); fixed the doubled Plugin Hub header; sleep-screen highlights no longer lose their text when the card has no Noto Serif.

## Files

- `firmware-sticky.bin`: OTA update asset for the built-in updater.
- `CrossNext-Sticky-v2.0.0-app.bin`: the same application image.
- `CrossNext-Sticky-v2.0.0-factory.bin`: full factory image for USB installation.
- `CrossNext-Sticky-v2.0.0.zip`: firmware bundle, checksums, manifest and license notices.
- `CrossNext-Sticky-Resources.zip`: microSD resources — PET, Reading Quest and The Intercept artwork plus reader fonts. Copy `apps/` and `fonts/` to the card root.
- `vi.cplang`: Vietnamese interface language pack for this firmware (the App Store installs it over Wi-Fi; offline, copy it to `/languages/`).
- `SHA256SUMS.txt`: SHA-256 checksums for the release assets.

## Validation

Built for `gh_release_sticky` (ESP32-S3, 16MB Flash, 8MB PSRAM).
