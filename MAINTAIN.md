# Hướng dẫn Dành cho Người Bảo trì & Đóng góp

Chào mừng bạn! Cảm ơn bạn đã quan tâm đến việc phát triển dự án này. Tài liệu này nhằm mục đích giải thích kiến trúc, các quyết định thiết kế, và quy trình để đóng góp.

## 🏛️ Kiến trúc Tổng quan

Dự án này là một hệ thống độc lập, không cần script Python bên ngoài. Toàn bộ logic được xử lý trên ESP32 và giao tiếp với người dùng thông qua Arduino Serial Monitor.

- **Ngôn ngữ:** C++ (trên nền tảng Arduino).
- **Vai trò:** ESP32-S3 đóng vai trò là "bộ não" và "giao diện người dùng", điều khiển trực tiếp module PN532 và hiển thị kết quả ra Serial.
- **Thư viện chính:**
  - `Wire.h`: Giao tiếp I2C.
  - `Adafruit_PN532.h`: Cung cấp các hàm API cấp cao để tương tác với thẻ MIFARE.

## 🏗️ Cấu trúc Code

Code trong file `.ino` chính được chia thành các khu vực logic rõ ràng:

1.  **`#pragma region Robust Functions & Helpers`**:
    - **Mục đích:** Đây là nền tảng của sự ổn định. Các hàm `robust*` bọc các lệnh giao tiếp NFC gốc của thư viện Adafruit trong một vòng lặp thử lại.
    - **Lý do thiết kế:** Giao tiếp RF/NFC vốn không ổn định. Việc thử lại tự động giúp chương trình vượt qua các lỗi tạm thời mà không làm phiền người dùng.
    - **Khi bảo trì:** Nếu thư viện Adafruit được cập nhật với các hàm mới, hãy xem xét tạo các phiên bản `robust` tương ứng cho chúng.

2.  **`#pragma region Menu Functions`**:
    - **Mục đích:** Chứa code triển khai cho mỗi chức năng trong menu.
    - **Thiết kế:** Mỗi hàm nên độc lập, thực hiện một nhiệm vụ duy nhất (ví dụ: `cloneCopy`, `formatCard`). Logic phức tạp nên được chứa hoàn toàn bên trong hàm.
    - **Tối ưu hóa:** Chú ý đến hiệu suất, đặc biệt là trong các vòng lặp lớn (như trong `clonePasteData` và `formatCard`). Logic hiện tại đã được tối ưu để chỉ xác thực mỗi sector một lần duy nhất trước khi thực hiện nhiều thao tác ghi.

3.  **`#pragma region Global Variables`** (Khu vực khai báo biến toàn cục):
    - `clonedData[64][16]`: Được khai báo là `static` để tránh các vấn đề về cấp phát bộ nhớ khi khởi động (Stack Overflow), một nguyên nhân phổ biến gây ra boot loop.
    - **Khi bảo trì:** Hạn chế thêm biến toàn cục. Nếu cần, hãy xem xét khai báo chúng là `static`.

4.  **`setup()` và `loop()`**:
    - `setup()`: Khởi tạo phần cứng và thư viện. Chứa các bước sửa lỗi quan trọng như khởi tạo `Wire` một cách tường minh để tăng độ ổn định I2C.
    - `loop()`: Đơn giản, chỉ lắng nghe một ký tự lệnh từ Serial và gọi hàm chức năng tương ứng.

## 🔧 Quy trình Gỡ lỗi

1.  **Boot Loop (Khởi động lại liên tục):**
    - **Triệu chứng:** Log Serial chỉ hiển thị các thông báo khởi động của ESP-ROM.
    - **Nguyên nhân & Giải pháp:**
      1.  **Nguồn yếu:** Đổi cáp/cổng USB.
      2.  **Stack Overflow:** Kiểm tra các biến toàn cục và cục bộ có kích thước lớn. Thêm từ khóa `static` cho các biến toàn cục lớn.

2.  **Chương trình bị Treo/Đơ sau khi Khởi động:**
    - **Triệu chứng:** Chương trình in ra vài dòng rồi dừng hẳn.
    - **Nguyên nhân & Giải pháp:**
      1.  **Lỗi I2C:** Log có thể báo `I2C transaction unexpected nack detected`. Đây là dấu hiệu mạnh mẽ của việc thiếu điện trở pull-up.
      2.  **Giải pháp Phần cứng:** Thêm 2 điện trở 4.7kΩ cho SDA và SCL.
      3.  **Giải pháp Phần mềm:** Đảm bảo `Wire.begin()` được gọi một cách tường minh trong `setup()` trước `nfc.begin()`.

3.  **Thao tác Thẻ Thất bại Ngẫu nhiên:**
    - **Triệu chứng:** Cùng một thao tác lúc thành công, lúc thất bại.
    - **Nguyên nhân & Giải pháp:** Vấn đề về RF. Các hàm `robust*` đã được thiết kế để giảm thiểu điều này. Nếu vẫn xảy ra, có thể cần tăng số lần `retries` trong các hàm đó.

## 🌱 Hướng Phát triển trong Tương lai

- **Hỗ trợ Key B:** Menu hiện tại chỉ sử dụng Key A. Có thể thêm tùy chọn để người dùng nhập và sử dụng Key B để xác thực.
- **Quản lý Key:** Thêm chức năng cho phép người dùng thay đổi Key A, Key B và Access Bits trong Sector Trailer.
- **Hỗ trợ các loại Thẻ khác:** Mở rộng để hỗ trợ MIFARE Ultralight hoặc NTAG2xx (nếu phần cứng PN532 hỗ trợ).
- **Giao diện Người dùng Tốt hơn:** Có thể sử dụng một màn hình LCD nhỏ (như OLED I2C) để hiển thị thông tin thay vì chỉ dùng Serial Monitor.

## 🤝 Quy trình Đóng góp

1.  **Fork a Repository:** Tạo một bản sao của dự án về tài khoản của bạn.
2.  **Create a Branch:** Tạo một nhánh mới cho tính năng hoặc bản sửa lỗi của bạn (ví dụ: `feature/add-keyb-support` hoặc `fix/format-card-logic`).
3.  **Commit Changes:** Thực hiện các thay đổi của bạn và commit với các thông điệp rõ ràng.
4.  **Test Thoroughly:** Đảm bảo các thay đổi của bạn không phá vỡ các chức năng hiện có.
5.  **Submit a Pull Request:** Gửi một Pull Request về nhánh `main` của dự án gốc, kèm theo mô tả chi tiết về những gì bạn đã thay đổi và tại sao.

Cảm ơn bạn đã giúp dự án này trở nên tốt hơn!