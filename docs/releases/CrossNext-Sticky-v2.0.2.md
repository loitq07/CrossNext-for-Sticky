# CrossNext for Seeed reTerminal Sticky v2.0.2 (beta)

> **Bản thử nghiệm (prerelease).** Chỉ máy bật **Cài đặt → Hệ thống → Nhận bản thử nghiệm (Beta)** mới được đề nghị cập nhật. Có thể còn lỗi — hãy sao lưu thẻ nhớ trước.
> **Beta build (prerelease).** Only devices with **Settings → System → Beta updates** on are offered it. Expect rough edges — back up your microSD first.

> **Lưu ý OTA:** màn hình có thể đứng ở 0% khoảng 1 phút trong lúc xoá vùng nhớ cũ — đừng tắt máy.
> **OTA note:** the screen may sit at 0% for about a minute while the old slot is erased — don't power off.

Sau khi cập nhật, vào Home và chọn **Cập nhật ngay** (cần Wi-Fi) để tải lại game/app native cho firmware mới.
After updating, accept the app update on Home (needs Wi-Fi) to fetch the native games/apps for the new firmware.

## Tiếng Việt

- **Sửa game tải từ App Store không mở được / không hiện tên** (Xúc Xắc, Đố Vui, Toy Battle…): tên game trước đây có thể trỏ vào vùng nhớ đã giải phóng nên hiện rỗng hoặc sai và không mở được.
- **Đăng ký 11 game và 3 app còn thiếu trong kệ**, có icon và tên tiếng Việt: Connections, Cờ Xương (Knucklebones), Yahtzee, Sea Salt, Toy Battle, Đố Vui, Underhand, Wavelength, Forehead, Kẻ Nội Gián (Insider), Xúc Xắc; XKCD, Hacker News, Wikipedia.
- **Hearts theo kiểu CrossPlay**: cột 4 người chơi theo thứ tự lượt, điểm ăn trong ván (`+n`) và tổng tạm tính, bàn chơi dạng la bàn với lá đang thắng được khung đậm, `ROUND n` và `LƯỢT x/13`, ghi chú "Hearts broken / Queen seen", pha chuyền bài hiện 3 lá to và nút Pass. Giữ nguyên độ khó, giới hạn điểm và làm nổi bài hợp lệ của CrossNext.
- Sửa lỗi ghi lệch chỉ số khi chỉnh phông cho plugin không có khoá phông.

## English

- **Fixed App Store games that would not open or showed no name** (Dice, Trivia, Toy Battle…): the shelf kept dangling title pointers after its list reallocated, so names came out blank or garbled and the launch path did not match.
- **11 games and 3 apps that were store-only are now on the shelf**, with icons and Vietnamese names: Connections, Knucklebones, Yahtzee, Sea Salt, Toy Battle, Trivia, Underhand, Wavelength, Forehead, Insider, Dice; XKCD, Hacker News, Wikipedia.
- **CrossPlay-style Hearts**: a four-seat rail in turn order with per-round points (`+n`) and projected totals, a compass-style table with a heavy frame on the winning card, `ROUND n` and `TRICK x/13`, "Hearts broken / Queen seen" status, and a larger pass-cards panel with a Pass button. CrossNext's difficulty, score limit and legal-card highlight are kept.
- Fixed an out-of-range write when styling the font of plugins that have no font key.
