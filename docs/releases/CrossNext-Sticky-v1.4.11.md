# CrossNext for Seeed reTerminal Sticky v1.4.11

## Tiếng Việt

- **Plugin Hub**: vào Cài đặt → Hệ thống → Plugins, chọn "Cài đặt Plugin Hub" (hiện khi chưa có Hub trên thẻ SD) để duyệt, cài và cập nhật plugin cộng đồng. Danh sách plugin tải từ xa và có thông báo "plugin bên thứ ba" trước khi mở.
- `/api/status` báo thêm `firmwareFamily`, `pluginApi`, `pluginCapabilities` để plugin trên trình duyệt kiểm tra tương thích.
- Thư mục plugin cài dở (có tệp `.installing`) không còn bị coi là plugin.
- **Sửa lỗi crash** (`ESP_ERROR_CHECK`, "abort() was called") khi bật Hotspot lúc Chế độ nhà phát triển đang nối Wi-Fi.
- Câu trích ở màn hình ngủ: chọn được phông chữ (kể cả phông trên thẻ SD) và căn lề.
- Hỗ trợ bàn phím Bluetooth (BLE HID host) ở mức build, cập nhật phần cài đặt Bluetooth; phản hồi rung trong thư mục kệ sách.

## English

- **Plugin Hub**: Settings → System → Plugins offers "Install Plugin Hub" while the Hub is not on the SD card; afterwards browse, install and update community plugins. Plugin lists load remotely and show a one-time third-party notice first.
- `/api/status` now reports `firmwareFamily`, `pluginApi` and `pluginCapabilities` for browser-plugin compatibility checks.
- A partly installed plugin folder (`.installing` marker) is no longer treated as a plugin.
- **Crash fix** (`ESP_ERROR_CHECK` abort) when starting the hotspot while Developer Mode was still joining Wi-Fi.
- Sleep-screen book highlights: choose the font family (including SD fonts) and text alignment.
- Bluetooth keyboard (BLE HID host) build support and Bluetooth settings updates; haptic feedback in shelf folders.

## Not yet covered

The Plugin Hub list does not yet filter by `compatibility`, and item detail pages (`details_url`) are not implemented; the firmware does not advertise `catalog.compatibility` or `catalog.item_details`.

## Files

- `firmware-sticky.bin`: OTA update asset for the built-in updater.
- `CrossNext-Sticky-v1.4.11-app.bin`: the same application image.
- `CrossNext-Sticky-v1.4.11-factory.bin`: full factory image for USB installation.
- `CrossNext-Sticky-v1.4.11.zip`: firmware bundle, checksums, manifest and license notices.
- `SHA256SUMS.txt`: SHA-256 checksums for the release assets.

## Validation

Built for `gh_release_sticky` (ESP32-S3, 16MB Flash, 8MB PSRAM). The build compiles cleanly; the Plugin Hub install flow and the hotspot fix have not yet been exercised on a device.
