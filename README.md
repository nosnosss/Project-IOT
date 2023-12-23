- Tạo database mysql:

  CREATE DATABASE IF NOT EXISTS smart_lock;
  
  USE smart_lock;
  
  CREATE TABLE IF NOT EXISTS access_logs (
    id INT AUTO_INCREMENT PRIMARY KEY,
    card_id VARCHAR(255) NOT NULL,
    access_time DATETIME NOT NULL
  );

- Import các thư viện:
express: Framework Node.js cho việc xây dựng ứng dụng web.
body-parser: Middleware để xử lý dữ liệu được gửi đến server từ phía client.
mysql: Thư viện để kết nối và tương tác với MySQL database.
moment-timezone: Thư viện để làm việc với thời gian và múi giờ.
- Cấu hình Express:
Tạo một ứng dụng Express (app) và thiết lập cổng lắng nghe là 3000.
Sử dụng body-parser để xử lý dữ liệu từ phía client.
Đặt view engine là EJS cho việc render trang HTML.
- Cấu hình múi giờ:
Sử dụng thư viện moment-timezone để đặt múi giờ mặc định là 'Asia/Ho_Chi_Minh'.
- Kết nối đến MySQL Database:
Sử dụng mysql để tạo kết nối đến cơ sở dữ liệu MySQL.
- Endpoint /record (Ghi dữ liệu):
Lắng nghe phương thức POST từ phía client.
Lấy dữ liệu từ yêu cầu (req) như cardID.
Thực hiện một truy vấn SQL để chèn dữ liệu vào bảng access_logs.
Trả về trạng thái thành công hoặc lỗi.
- Endpoint /history (Hiển thị lịch sử):
Lắng nghe phương thức GET từ phía client.
Thực hiện một truy vấn SQL để truy vấn tất cả dữ liệu từ bảng access_logs và sắp xếp theo thời gian giảm dần.
Sử dụng res.render để hiển thị trang HTML (sử dụng EJS) và truyền dữ liệu lịch sử vào.
Kết thúc và lắng nghe trên cổng 3000:
Mở kết nối đến cơ sở dữ liệu MySQL.
Bắt đầu lắng nghe trên cổng 3000 và log thông báo khi server đã khởi chạy.
Lưu ý rằng để chạy ứng dụng này, bạn cần đảm bảo rằng MySQL server đã được cài đặt và cấu hình đúng, và bạn cũng cần tạo một cơ sở dữ liệu có tên là 'smart_lock'. Đồng thời, có thể cần cài đặt các thư viện thông qua npm bằng cách chạy lệnh npm install trong thư mục chứa mã nguồn của bạn để cài đặt các dependencies.

...
