# CrossNext for Seeed reTerminal Sticky v1.4.13

> **Lưu ý khi cập nhật OTA / OTA update note:** sau khi bấm Cập nhật, màn hình có thể **đứng ở 0% khoảng 1 phút** (máy đang xoá vùng nhớ cũ) và nút bấm tạm không phản hồi. Đừng tắt hay reset máy — thanh tiến trình sẽ chạy tiếp, toàn bộ mất khoảng 2–3 phút.
> After tapping Update, the screen may **sit at 0% for about a minute** while the old slot is erased, and buttons won't respond. Don't power off or reset — progress resumes on its own; the whole update takes about 2–3 minutes.
>
> **Tiếng Việt giờ là gói ngôn ngữ trên thẻ nhớ.** Lần đầu khởi động sau khi cập nhật, giao diện có thể hiện **tiếng Anh**. Vào Home, máy sẽ đề nghị cập nhật ứng dụng — hãy chọn **Cập nhật ngay** (cần Wi-Fi): máy tải game/app native cho firmware mới **và gói Tiếng Việt**, rồi tự chuyển lại sang tiếng Việt. Nếu lỡ chọn "Để sau": mở **Cửa hàng ứng dụng → Tiếng Việt**.
> **Vietnamese is now a microSD language pack.** The first boot after updating may show the interface in **English**. On Home, accept the app update (needs Wi-Fi): it fetches the native games/apps for the new firmware **and the Vietnamese pack**, then switches back to Vietnamese. Skipped it? Open **App Store → Vietnamese**.

## Tiếng Việt

- **Tiếng Việt trên thẻ nhớ**: firmware chỉ còn tích hợp tiếng Anh; tiếng Việt là gói `/languages/vi.cplang` (định dạng gói ngôn ngữ của CrossNext 2.0), nạp một lần vào PSRAM. Firmware nhẹ hơn 59,5 KB so với v1.4.12 dù có thêm tính năng mới. Gói được tải qua App Store và tự cập nhật theo mỗi bản firmware. Cài đặt → Hệ thống → Ngôn ngữ hiển thị các gói tìm thấy trên thẻ.
- **Chọn cách mở cài đặt sách** (Cài đặt → Điều khiển → Mở cài đặt sách): vuốt từ dưới lên (mặc định), chạm giữa trang, chạm mép dưới hoặc chạm mép trên. Chạm không mở khi đang có thanh công cụ, popup hay khi chạm vào link; nút Confirm vẫn mở như cũ.
- **Voice Notes không còn mất tiếng**: bản ghi được thu đủ (trước đây phần lớn âm thanh bị tràn bộ đệm), nên phiên âm chính xác hơn; hiển thị đúng thời lượng ghi tối đa (60, 30 hoặc 15 giây tuỳ bộ nhớ).
- **Nhận bản thử nghiệm (Beta)** trong Cài đặt → Hệ thống: bật để được cập nhật cả bản prerelease trước khi phát hành chính thức. Mặc định tắt.
- **Mở nhanh hơn**: Cài đặt không còn quét font và từ điển trên thẻ trước khi hiện danh mục; thư mục Trò chơi/Ứng dụng hiện ra trước khi ghi trạng thái xuống thẻ (từ CrossNext 2.0.1).
- **Trang web truyền tệp**: icon hiển thị cả khi không có Internet (khi kết nối hotspot của Sticky); kéo thả tệp hoặc cả thư mục vào bất kỳ đâu trên trang để tải lên; giao diện mặc định tiếng Anh, chuyển được sang tiếng Việt.
- **Sửa lỗi**: Go đọc ngoài mảng khi kiểm tra ô chéo (từ CrossNext 2.0.1).

## English

- **Vietnamese on the microSD**: English is the only built-in interface language; Vietnamese is `/languages/vi.cplang` (CrossNext 2.0 language-pack format), loaded once into PSRAM. The image is 59.5 KB smaller than v1.4.12 despite the new features. The App Store fetches the pack and refreshes it with every firmware build. Settings → System → Language lists the packs found on the card.
- **Choose how book settings open** (Settings → Controls → Open book settings): swipe up from the bottom (default), tap the page centre, tap the bottom edge or tap the top edge. A tap never opens it over a toolbar, a popup or a link; Confirm still opens it.
- **Voice Notes keep every word**: recordings no longer overflow the audio buffer, so transcriptions are complete, and the real recording limit (60, 30 or 15 s, depending on memory) is shown.
- **Beta updates** in Settings → System: opt in to prerelease builds before they ship to everyone. Off by default.
- **Faster menus**: Settings no longer scans SD fonts and dictionaries before showing its categories; Games/Apps folders appear before their state is written to the card (from CrossNext 2.0.1).
- **Web file portal**: icons work without Internet (on the Sticky's own hotspot); drop files or whole folders anywhere on the page to upload them; English by default with a Vietnamese switch.
- **Fix**: Go no longer reads past its neighbour table when checking diagonals (from CrossNext 2.0.1).

## Files

- `firmware-sticky.bin`: OTA update asset for the built-in updater.
- `CrossNext-Sticky-v1.4.13-app.bin`: the same application image.
- `CrossNext-Sticky-v1.4.13-factory.bin`: full factory image for USB installation.
- `CrossNext-Sticky-v1.4.13.zip`: firmware bundle, checksums, manifest and license notices.
- `vi.cplang`: Vietnamese interface language pack for this firmware. The App Store installs it over Wi-Fi (from `appstore:modules/<abi>/languages/`); offline, copy it to `/languages/` on the microSD and restart.
- `SHA256SUMS.txt`: SHA-256 checksums for the release assets.

## Validation

Built for `gh_release_sticky` (ESP32-S3, 16MB Flash, 8MB PSRAM). Host checks: Beta OTA release parser (104 checks), Go diagonal neighbours, `vi.cplang` header/CRC and key mapping (3,917 of 3,917 translations map to firmware keys), Phosphor icon subset.
