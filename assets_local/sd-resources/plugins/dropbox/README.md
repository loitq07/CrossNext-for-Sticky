# Dropbox Plugin cho CrossNext (reTerminal Sticky)

Plugin này cho phép duyệt thư mục và tải sách trực tiếp từ Dropbox về máy đọc sách của bạn.

## 1. Tính năng
- **Duyệt cây thư mục trên Dropbox:** Chạm vào thư mục để mở, phím Back để quay lại thư mục cha.
- **Tự động lọc sách:** Chỉ hiện file sách (.epub, .pdf, .txt, .mobi, .cbz) và thư mục, tự động bỏ qua các file không liên quan.
- **Phân trang bằng cursor:** Hỗ trợ xem tiếp các trang sau một cách mượt mà.
- **Tự động làm mới token:** Hỗ trợ refresh token để không bị gián đoạn khi đọc sách lâu dài.
- **Tải trực tiếp vào thẻ SD:** Lưu tại thư mục `/books/Dropbox/`.

## 2. Cách lấy Access Token / Refresh Token từ Dropbox (Làm 1 lần)
1. Đăng nhập vào [Dropbox App Console](https://www.dropbox.com/developers/apps).
2. Bấm **Create app**:
   - Chọn **Scoped access**.
   - Chọn **Full Dropbox** (hoặc **App folder** nếu chỉ muốn chia sẻ 1 thư mục riêng).
   - Đặt tên cho app (ví dụ: `MyStickyReader-YourName`).
3. Vào tab **Permissions**:
   - Tích chọn quyền: `files.metadata.read` và `files.content.read`.
   - Bấm **Submit** ở cuối trang để lưu quyền.
4. Vào tab **Settings**:
   - Bạn sẽ thấy **App key** và **App secret**.
   - Trong mục **OAuth 2**, bạn có thể bấm nút **Generate access token** để lấy token dùng ngay.
   - Để có token không bao giờ hết hạn, bạn có thể tạo Refresh Token qua OAuth2 hoặc dùng App key/secret.

## 3. Cấu hình trên máy Sticky
Điền token vào file `/plugins/dropbox/token.json`:
```json
{
  "access_token": "YOUR_DROPBOX_ACCESS_TOKEN",
  "refresh_token": "YOUR_REFRESH_TOKEN_IF_ANY"
}
```
*(Nếu dùng refresh token, điền thêm `client_id` (App key) và `client_secret` (App secret) vào `/plugins/dropbox/config.json`).*

## 4. Sử dụng trên thiết bị
1. Bật Wi-Fi trên máy Sticky.
2. Từ màn hình chính, vào biểu tượng **Plugins / Catalogs** > chọn **Dropbox**.
3. Danh sách thư mục và file sách trên Dropbox sẽ hiện ra ngay lập tức.
4. Chọn cuốn sách bạn muốn đọc để tải về!
