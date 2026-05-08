# Brickout_FPGA
# Brickout_FPGA
[![Brickout FPGA Demo](https://img.youtube.com/vi/NMJlXC7nWPQ/0.jpg)](https://youtu.be/NMJlXC7nWPQ)

**Brickout_FPGA** là phiên bản trò chơi Breakout được triển khai trên nền tảng FPGA (bo mạch Arty A7-35T). Dự án sử dụng phần mềm Vivado để cấu hình phần cứng và Vitis/C để lập trình phần mềm điều khiển, hiển thị hình ảnh trên màn hình OLED SSD1306 và nhận tín hiệu từ nút nhấn vật lý.

## Mục tiêu
- Triển khai trò chơi Breakout trên FPGA với đồ họa cơ bản.
- Tích hợp logic trò chơi, hiển thị và điều khiển người chơi.
- Thử nghiệm và đánh giá hiệu năng hệ thống trong môi trường thực hành.

## Thiết bị và công cụ
| Hạng mục | Chi tiết |
|----------|---------|
| Bo mạch FPGA | Arty A7-35T |
| Phần mềm thiết kế | Vivado (2020.2) |
| Màn hình hiển thị | OLED SSD1306 giao tiếp I²C |
| Thiết bị nhập liệu | Nút nhấn vật lý trên bo mạch |
| Ngôn ngữ lập trình | C cho hệ thống nhúng trên FPGA |

## Cách chạy
1. Mở Vivado, nạp cấu hình phần cứng lên Arty A7-35T.
2. Mở Vitis, biên dịch và nạp phần mềm điều khiển.
3. Kết nối màn hình OLED và nút nhấn theo sơ đồ phần cứng.
4. Khởi chạy trò chơi và điều khiển bằng nút nhấn.

## Tính năng
- Giới hạn FPS để đảm bảo tốc độ bóng và thanh đỡ ổn định.
- Đồng bộ logic và hiển thị để tránh trễ hoặc lỗi hiển thị.
- Quản lý trạng thái trò chơi trực tiếp trên FPGA.

## Lưu ý
- Dự án được triển khai cho **Arty A7-35T**, có thể cần chỉnh sửa nếu sử dụng bo mạch khác.
- Đảm bảo môi trường thực hành ổn định khi chạy trò chơi.

## Liên kết
Toàn bộ mã nguồn dự án được lưu trữ tại: [Brickout_FPGA trên GitHub](https://github.com/dathavan1523/Brickout_FPGA/tree/main)

