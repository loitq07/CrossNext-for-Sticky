# Đọc sau trên Sticky

Mở **Đọc sau** từ màn hình chính để xem bài viết Wallabag đã tải. Danh sách này
hoạt động offline, sắp bài mới tải lên trước và hiển thị tiến độ nếu bài đã được
đọc. Chạm một bài để mở bằng trình đọc EPUB; nhấn Back để quay lại Đọc sau.

Chọn **Wallabag · Duyệt và tải bài** để kết nối Wi-Fi, duyệt danh sách Wallabag
và tải bài. Cài plugin vào `/plugins/wallabag` và điền tài khoản trong
`config.json` theo README của plugin. Đây là thao tác duyệt và tải từng bài;
chưa có tải toàn bộ hàng đợi hoặc đồng bộ trạng thái đọc/archive về Wallabag.
Khi rời màn hình Wallabag, danh sách offline tự làm mới. Có thể dùng
**Làm mới bài đã tải** sau khi chép bài từ máy tính.

Bài viết lưu tại `/books/Wallabag/`, bao gồm bài tải từ mục Plugins. Firmware
giữ thư mục này cố định cho Wallabag, kể cả khi config của plugin có `dest_dir`
khác. EPUB có sẵn ở thư mục này được nhận ngay, không di chuyển hoặc xóa file.
Vị trí đọc và cache EPUB vẫn dùng đường dẫn cũ.

Các bài trong `/books/Wallabag/` không được thêm vào thư viện sách hay sách gần
đây. Bản nâng cấp loại các mục này khỏi lịch sử đã lưu và xây lại chỉ mục thư
viện theo chính sách mới. Các file vẫn truy cập được qua trình duyệt file.

Màn hình Đọc sau liệt kê tối đa 512 EPUB, với tối đa bốn cấp thư mục con; nếu
vượt giới hạn hoặc gặp lỗi đọc thẻ, màn hình có thông báo. Tên hiển thị lấy từ
tên file EPUB. Đổi tên hoặc chuyển bài ra ngoài thư mục này sẽ làm thay đổi cách
phân loại và có thể làm mất liên kết với vị trí đọc cũ.

## Integration seams

`src/apps_local/readlater/ReadLaterActivity` owns the offline list. Home adds a
stable appended menu ID, HMC5 preserves HMC1–HMC4 labels and shortcuts, and the
cover-grid tab buffer follows the shared visible-entry limit of eight. BOOX
adds a seventh quick action alongside its existing Settings button.

`ContentVisibility::readLaterContent` is shared by LibraryBuilder,
LibraryCardCheck, RecentBooksStore and LibrarySync. Visibility policy version 2
invalidates old indexes. PluginCatalogActivity can open a named plugin directly
and returns to its caller; Wallabag downloads always use the owned article
directory. ReaderUtils routes Back from that directory to ReadLaterActivity.

Validation: `host-tests/read-later/run.sh`, Home UI host tests, reading-suite
core/store host tests, and the `gh_release_sticky` firmware build. Live sign-in
and Wallabag downloads require a configured account and device.
