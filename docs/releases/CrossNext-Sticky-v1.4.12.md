# CrossNext for Seeed reTerminal Sticky v1.4.12

## Tiếng Việt

- **What's New tích hợp trên thiết bị & Web UI**:
  - Tự động hiển thị màn hình "Có gì mới" khi khởi động sau khi nâng cấp firmware.
  - Mục "Có gì mới" được tích hợp trong Cài đặt → Hệ thống và trong mục Giới thiệu (About) để xem lại bất cứ lúc nào mà không cần truy cập GitHub.
  - Nút và modal "Có gì mới" ngay trên giao diện Web truyền file (Settings, Files, Fonts).
- **Cửa hàng ứng dụng (App Store)**: Hỗ trợ Native Modules cho phép duyệt, tải và cài đặt game, ứng dụng trực tiếp trên Sticky.
- **Voice Notes (Ghi âm giọng nói)**: Ứng dụng ghi âm micro PDM tích hợp, chuẩn hóa âm thanh và hỗ trợ phiên âm AI (Groq).
- **Instapaper & Máy tính cầm tay**: Đọc báo offline từ Instapaper và máy tính tiện lợi với giao diện pixel đồng bộ.
- **Giao diện Web truyền file nâng cấp**: Thiết kế dạng tab hiện đại, xem trước ảnh thumbnail trực tiếp, hỗ trợ tải file inline.
- **Tối ưu hóa & Sửa lỗi**: Khắc phục triệt để lỗi đơ SPI khi tải tệp, tự động nhận diện màn hình Sticky mới, tối ưu cảm ứng và font chữ.

## English

- **Built-in What's New on Device & Web Portal**:
  - Automatically raises a "What's New" screen on first boot after firmware upgrade.
  - Accessible anytime in Settings → System → What's New and About screen without visiting GitHub.
  - Interactive "What's New" button and modal on the Web file manager portal.
- **Native App Store**: Native Modules support for browsing, installing, and managing on-device games and apps.
- **Voice Notes**: Onboard PDM microphone audio recording with normalization and Groq transcription support.
- **Instapaper & Calculator**: Offline saved article reader with Instapaper and quick on-device calculator.
- **Modern Web File Portal**: Tabbed settings interface, live image thumbnail previews, and inline download support.
- **Hardware & System Stability**: Resolves SPI freeze during file downloads, adds auto-detection for new Sticky display revisions, and provides instant touch feedback.

## Files

- `firmware-sticky.bin`: OTA update asset for the built-in updater.
- `CrossNext-Sticky-v1.4.12-app.bin`: the same application image.
- `CrossNext-Sticky-v1.4.12-factory.bin`: full factory image for USB installation.
- `CrossNext-Sticky-v1.4.12.zip`: firmware bundle, checksums, manifest and license notices.
- `SHA256SUMS.txt`: SHA-256 checksums for the release assets.

## Validation

Built for `gh_release_sticky` (ESP32-S3, 16MB Flash, 8MB PSRAM). All embedded web assets pass node syntax tests.
