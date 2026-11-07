# Google Drive Plugin cho CrossNext (reTerminal Sticky)

Plugin này cho phép duyệt thư mục và tải sách trực tiếp từ Google Drive về máy đọc sách của bạn.

## 1. Tính năng
- **Đăng nhập bằng mã QR (OAuth 2.0 Device Flow):** Quét mã QR trên màn hình E-ink bằng điện thoại để đăng nhập.
- **Tự động làm mới phiên (Silent Refresh):** Không cần đăng nhập lại khi token hết hạn.
- **Duyệt thư mục phân cấp:** Bấm vào thư mục để xem nội dung bên trong, bấm Back để quay lại.
- **Tự động lọc sách:** Chỉ hiển thị các file sách và tài liệu (.epub, .pdf, .txt, .mobi, .cbz).
- **Tải trực tiếp vào thẻ SD:** Sách tải về được lưu tại thư mục `/books/GoogleDrive/`.

## 2. Cách thiết lập Google Cloud (Làm 1 lần duy nhất - Miễn phí)
1. Truy cập [Google Cloud Console](https://console.cloud.google.com/).
2. Tạo một Dự án mới (New Project), đặt tên ví dụ: `CrossNext Reader`.
3. Vào **APIs & Services** > **Library**, tìm **Google Drive API** và bấm **Enable**.
4. Vào **APIs & Services** > **OAuth consent screen**:
   - Chọn **External**, bấm **Create**.
   - Điền tên ứng dụng (App name) và email của bạn, lưu lại.
   - Thêm Scope: `https://www.googleapis.com/auth/drive.readonly`.
   - Trong mục **Test users**, thêm địa chỉ Gmail của bạn.
5. Vào **APIs & Services** > **Credentials**:
   - Bấm **Create Credentials** > **OAuth client ID**.
   - Application type: Chọn **TVs and Limited Input devices**.
   - Đặt tên và bấm **Create**.
   - Bạn sẽ nhận được **Client ID** và **Client Secret**.

## 3. Cấu hình trên máy Sticky
1. Mở file `config.json` trong thư mục plugin này (`/plugins/google-drive/config.json`).
2. Điền Client ID và Client Secret vào:
```json
{
  "client_id": "YOUR_CLIENT_ID.apps.googleusercontent.com",
  "client_secret": "YOUR_CLIENT_SECRET"
}
```
*(Bạn cũng có thể cấu hình qua Web UI của thiết bị tại `http://<ip-thiết-bị>/settings`).*

## 4. Sử dụng trên thiết bị
1. Bật Wi-Fi trên máy Sticky.
2. Từ màn hình chính, vào biểu tượng **Plugins / Catalogs** > chọn **Google Drive**.
3. Màn hình sẽ hiện **Mã số xác thực** và **Mã QR**:
   - Dùng điện thoại quét mã QR.
   - Nhập mã số hiển thị trên màn hình E-ink.
   - Bấm Cho phép (Allow).
4. Máy Sticky sẽ tự động tải danh sách thư mục và file sách trong Google Drive của bạn!
