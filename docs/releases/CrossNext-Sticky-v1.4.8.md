# CrossNext for Seeed reTerminal Sticky v1.4.8

## Tiếng Việt

- Thêm **Highlight từ sách** trong **Cài đặt → Màn hình → Màn hình ngủ**:
  hiển thị trích dẫn đã lưu, tên sách và tác giả.
- **Chu kỳ đổi highlight**: chọn 1–240 phút, mặc định 5 phút; chọn 0 để tắt
  tự đổi. Máy thức ngắn để đổi highlight rồi trở lại ngủ sâu.
- Dùng 512 trích dẫn hoàn chỉnh gần nhất trong `/My Clippings.txt`, bỏ qua
  mục đã ẩn, nội dung rỗng, bản ghi chưa hoàn chỉnh và bookmark. Tránh lặp
  lại ngay mục vừa hiển thị khi có lựa chọn khác.
- Khi đổi tự động, giữ nguyên trạng thái khôi phục sách/ứng dụng, không bật
  đèn và không kết nối lại bàn phím Bluetooth. Chỉ một highlight hợp lệ thì
  không hẹn giờ thức định kỳ.
- Có nhãn tiếng Việt, tiếng Anh, tiếng Tây Ban Nha và hướng dẫn khi chưa có
  highlight. Chế độ Quick Resume sau thời gian chờ vẫn được ưu tiên nếu bật.

## English

- Add **Book highlights** under **Settings → Display → Sleep screen**, with
  the saved passage, book title and author.
- Set **Change highlight every** to 1–240 minutes (default 5), or 0 to keep
  one highlight. Scheduled changes briefly wake the device and return to deep sleep.
- Use the latest 512 complete records in `/My Clippings.txt`, honor hidden
  quotes, skip empty/incomplete/bookmark records and avoid immediate repeats.
- Preserve book/app resume state, keep the frontlight off and skip Bluetooth
  keyboard reconnection during scheduled changes. One valid highlight needs
  no periodic wake. Quick Resume after timeout still takes precedence when enabled.

## Files

- `firmware-sticky.bin`: OTA update asset for the built-in updater.
- `CrossNext-Sticky-v1.4.8-app.bin`: the same application image.
- `CrossNext-Sticky-v1.4.8-factory.bin`: full factory image for USB installation.
- `CrossNext-Sticky-v1.4.8.zip`: firmware bundle, checksums, manifest and license notices.
- `SHA256SUMS.txt`: SHA-256 checksums for the release assets.

## Validation

Built for `gh_release_sticky`. Highlight rendering/selection/storage tests,
streaming index tests, Quotes store tests, wake policy tests, Wallpapers tests
and Vietnamese localization checks passed. Timer wake behavior, physical refresh
quality and sleep current have not been verified on a device.
