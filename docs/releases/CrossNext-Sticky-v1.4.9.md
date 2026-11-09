# CrossNext for Seeed reTerminal Sticky v1.4.9

## Tiếng Việt

- **Bộ nhận diện Typography Baemin Daniel & Tiếng Việt**:
  - Tích hợp trọn bộ font chữ Baemin Daniel Bold (`baemin_10`, `baemin_12`, `baemin_14`, `baemin_18`) với đầy đủ dấu tiếng Việt chuẩn mực cho toàn bộ Header, Section, Button, Thẻ thông tin và Cài đặt nhanh.
  - Bộ số Baemin Daniel Bold (`Numerals.h`) đồng bộ phong cách CrossNext gốc cho các trang Thống kê đọc sách (`0 PHÚT`, `1 PHIÊN ĐỌC`, `45 PHÚT`), số đếm trang, đồng hồ v.v.
- **Tinh chỉnh giao diện & Sửa lỗi hiển thị**:
  - Mở rộng khung phần trăm pin từ 58px lên 76px, khắc phục triệt để lỗi pin 100% bị cắt ngắn thành `10...`.
  - Phụ đề danh sách trong Cài đặt và Trích dẫn giữ định dạng chữ thường tự nhiên (mixed-case), tránh tràn chữ và ép dòng.
  - Nút huy hiệu (Badge) trang chủ tự động điều chỉnh hiển thị `"TIẾP TỤC"` không bị chấm lửng.
- **Tối ưu cảm ứng & Khung viền (Bezel) Sticky**:
  - Cấu hình chính xác khoảng đệm viền bezel cho Seeed reTerminal Sticky (`BoardConfig.h`).
  - Hỗ trợ thao tác cảm ứng trong Giá sách (Shelf) và Tiện ích đọc sách (Reading Tools).
  - Tùy chọn lật trang chỉ bằng vuốt (Swipe-only) không bị chạm nhầm ở mép màn hình.

## English

- **Baemin Daniel Typography & Vietnamese Font Design System**:
  - Integrated complete Baemin Daniel Bold font family (`baemin_10`, `baemin_12`, `baemin_14`, `baemin_18`) with full Vietnamese diacritics across Headers, Sections, Buttons, Cards, and Quick Settings.
  - Dedicated Baemin Daniel Bold Numerals (`Numerals.h`) matching authentic CrossNext design for reading statistics (`0 MIN`, `1 SESSION`, `45 MIN`), counters, and clock.
- **UI Overflow & Layout Improvements**:
  - Expanded battery percentage box from 58px to 76px, preventing `100%` truncation to `10...`.
  - Natural mixed-case labels for menu subtitles in Settings and Quotes lists, eliminating cramped ALLCAPS text.
  - Smart resume badge resizing on Home screen ("TIẾP TỤC" fallback without truncation ellipsis).
- **Sticky Bezel Insets & Touch Enhancements**:
  - Declared precise bezel insets for Seeed reTerminal Sticky in `BoardConfig.h`.
  - Touch support for Shelf and Reading Tools activities.
  - Swipe-only page turn option with edge-tap bypass prevention in reader.

## Files

- `firmware-sticky.bin`: OTA update asset for the built-in updater.
- `CrossNext-Sticky-v1.4.9-app.bin`: the same application image.
- `CrossNext-Sticky-v1.4.9-factory.bin`: full factory image for USB installation.
- `CrossNext-Sticky-v1.4.9.zip`: firmware bundle, checksums, manifest and license notices.
- `SHA256SUMS.txt`: SHA-256 checksums for the release assets.

## Validation

Built for `gh_release_sticky`. Typography engine, numeral glyph tests, touch gestures, and OTA flashing on hardware verified via Wi-Fi Developer Mode.
