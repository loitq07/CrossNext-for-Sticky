# Wallabag Plugin cho CrossNext (reTerminal Sticky)

Plugin này tích hợp dịch vụ lưu bài viết để đọc sau (Read-It-Later) **Wallabag** vào máy đọc sách của bạn. Mỗi bài viết sẽ được tải về dưới dạng một cuốn sách **EPUB hoàn chỉnh** (bao gồm đầy đủ hình ảnh, dàn trang và định dạng đẹp mắt).

## 1. Tính năng nổi bật
- **Phân loại danh mục bài viết:**
  - **Chưa đọc (Unread):** Danh sách bài viết mới lưu chờ đọc.
  - **Yêu thích (Starred):** Các bài viết hay đã đánh dấu sao.
  - **Lưu trữ (Archive):** Các bài viết đã đọc xong.
- **Tải chuẩn EPUB:** Tự động gọi API export của Wallabag để đóng gói bài viết thành file `.epub` chuẩn cho E-reader.
- **Tự động đăng nhập & làm mới Token (Silent OAuth 2.0):** Tự động mint access token và refresh token từ thông tin cấu hình, người dùng không cần đăng nhập lại thủ công.
- **Hỗ trợ cả Cloud và Self-Hosted:** Tương thích với `app.wallabag.it` cũng như bất kỳ máy chủ Wallabag tự lưu trữ (Docker/VPS) nào.
- **Lưu trữ gọn gàng:** Sách tải về được lưu tại thư mục `/books/Wallabag/`.

## 2. Cách lấy Client ID và Client Secret từ Wallabag (Làm 1 lần)
1. Đăng nhập vào tài khoản Wallabag của bạn trên trình duyệt (ví dụ: `https://app.wallabag.it` hoặc trang web Wallabag cá nhân của bạn).
2. Vào mục **API clients management** (hoặc truy cập trực tiếp đường dẫn `/developer`).
3. Bấm nút **Create a new client**:
   - Name: Đặt tên bất kỳ (ví dụ: `StickyReader`).
4. Bấm **Save**. Wallabag sẽ cung cấp cho bạn:
   - **Client ID**
   - **Client Secret**

## 3. Cấu hình trên máy Sticky
Mở file `/plugins/wallabag/config.json` và điền thông tin tài khoản của bạn:
```json
{
  "url": "https://app.wallabag.it",
  "client_id": "YOUR_CLIENT_ID",
  "client_secret": "YOUR_CLIENT_SECRET",
  "username": "your_wallabag_username",
  "password": "your_wallabag_password"
}
```
> [!NOTE]
> * Phần `"url"` không để dấu gạch chéo `/` ở cuối (ví dụ: `https://app.wallabag.it` hoặc `https://wallabag.domaincuaban.com`).
> * Bạn cũng có thể thiết lập các thông tin này qua giao diện Web của máy Sticky tại `http://<ip-thiết-bị>/settings`.

## 4. Sử dụng trên thiết bị
1. Bật Wi-Fi trên máy Sticky.
2. Từ màn hình chính, vào biểu tượng **Plugins / Catalogs** > chọn **Wallabag**.
3. Chọn danh mục: **Chưa đọc**, **Yêu thích** hoặc **Lưu trữ**.
4. Chạm vào bài viết bạn muốn đọc, máy sẽ tự động tải file EPUB về và mở ra đọc ngay lập tức!
