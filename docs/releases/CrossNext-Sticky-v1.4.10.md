# CrossNext for Seeed reTerminal Sticky v1.4.10

## Tiếng Việt

- **Khắc phục lỗi hiển thị dấu tiếng Việt trên font Jersey 25**:
  - Tách tầng hoàn toàn giữa dấu mũ/trăng (`^`, `˘`) và các dấu thanh (sắc, huyền, hỏi, ngã) với khoảng đệm 1px, không còn bị đè nét hay dính dấu vào nhau.
- **Bổ sung trọn bộ Tiếng Việt cho toàn bộ font giao diện**:
  - Tạo và nhúng 141 glyph tiếng Việt chuẩn mực cho tất cả các bộ font: **Jersey 25** (`toybox_10/14/20/30`), **Noto Sans** (`ui_prose_10/14/bold_14`, `book_text_10/bold_10`), **Noto Serif** (`reading_serif_11/14/bold_11/bold_14`) và **Classic**.
  - Không còn tình trạng giao diện bị nhảy font lộn xộn hoặc fallback chữ xấu khi chuyển đổi font.
  - Tích hợp font dự phòng hệ thống an toàn `kSystemFallbackFontId` trong `ToyboxFonts.cpp`.
- **Khôi phục phím cứng & Tinh chỉnh điều hướng Kệ sách (Bookshelf)**:
  - Khôi phục chuẩn hành vi phím Power theo cấu hình người dùng (`shortPwrBtn`).
  - Sửa dứt điểm lỗi ấn nút khi ở giao diện Kệ sách tự động mở danh sách game khi ngăn kéo kệ sách đang đóng.

## English

- **Pixel Diacritics Fix on Jersey 25**:
  - Implemented stacked tiered diacritics with 1px clearance between circumflex/breve marks (`^`, `˘`) and tone marks (acute, grave, hook, tilde) for crisp pixel-perfect legibility.
- **Comprehensive Vietnamese Typography Across All UI Fonts**:
  - Generated and embedded complete 141-glyph Vietnamese character sets across all fonts: **Jersey 25**, **Noto Sans**, **Noto Serif**, and **Classic**.
  - Resolved UI font mismatch and jagged fallback behavior when toggling UI fonts.
  - Added robust system fallback font mechanism (`kSystemFallbackFontId`) in `ToyboxFonts.cpp`.
- **Hardware Button & Navigation Fixes**:
  - Restored original power button short-press behavior (`shortPwrBtn`).
  - Fixed inadvertent game drawer activation on Bookshelf home layout when shelf drawer is closed.

## Files

- `firmware-sticky.bin`: OTA update asset for the built-in updater.
- `CrossNext-Sticky-v1.4.10-app.bin`: the same application image.
- `CrossNext-Sticky-v1.4.10-factory.bin`: full factory image for USB installation.
- `CrossNext-Sticky-v1.4.10.zip`: firmware bundle, checksums, manifest and license notices.
- `SHA256SUMS.txt`: SHA-256 checksums for the release assets.

## Validation

Built for `gh_release_sticky` (ESP32-S3, 16MB Flash, 8MB PSRAM). Verified font generation, layout integrity, and hardware button input handling.
