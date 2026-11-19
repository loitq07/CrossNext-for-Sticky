# CrossNext for Seeed reTerminal Sticky v2.0.1 (beta)

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

## Tiếng Việt

Bản cập nhật v2.0.1 bổ sung các hoàn thiện giao diện và tính năng cho CrossNext 2.0 trên Sticky:

- **Tách riêng tab Ngôn ngữ & Tài nguyên trong App Store**:
  - Gói ngôn ngữ tiếng Việt (`vi.cplang`) chuyển sang tab **Ngôn ngữ** riêng biệt.
  - Các gói tài nguyên thẻ nhớ (ảnh/dữ liệu cho PET, Reading Quest, The Intercept) nằm trong tab **Tài nguyên** riêng, với thao tác tải về/cập nhật/xoá trực tiếp trên thẻ nhớ.
  - Tab **Tất cả** chỉ tập trung hiển thị Trò chơi và Ứng dụng.
- **Quản lý phông chữ đọc sách độc lập**:
  - Phông chữ Noto Serif được quản lý trực tiếp qua **Cài đặt → Đọc → Quản lý phông chữ → Tải về** (tải từ kho phông chữ CrossPoint), không gây lẫn lộn trong App Store.
  - Hệ thống tự động nhận diện cả phông chữ tải về và phông có sẵn làm phông đọc sách mặc định.
- **Tên tab giao diện gọn gàng**:
  - Tất cả thanh tab (App Store, Đọc sách, v.v.) hiển thị tên tab trên 1 dòng duy nhất, tự động thu gọn với dấu chấm lửng `...` khi nhãn dài để giữ giao diện luôn cân đối và sắc nét.

## English

Update v2.0.1 brings UI polish and system refinements for CrossNext 2.0 on Sticky:

- **Dedicated Languages & Resources tabs in App Store**:
  - The Vietnamese language pack (`vi.cplang`) now has its own **Languages** tab.
  - MicroSD resource packs (artwork/assets for PET, Reading Quest, The Intercept) live in their own **Resources** tab with download, reinstall and delete actions.
  - The **All** tab cleanly focuses on games and applications.
- **Streamlined reader font management**:
  - Reader fonts (including Noto Serif) are downloaded and managed under **Settings → Reading → Manage fonts → Download** (via the CrossPoint font catalog).
  - The font system seamlessly supports downloaded and bundled font variations as reader defaults.
- **Clean single-line tab bars**:
  - Tab bars across the interface constrain labels to a single line, automatically ellipsizing (`...`) overflowing titles to keep layout proportions balanced.
