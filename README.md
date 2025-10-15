# ESP32-S3 & PN532: Bộ Công Cụ Quản Lý & Sao Chép Thẻ MIFARE Classic Nâng Cao

![Project Banner](https://i.imgur.com/your-image-url.png) <!-- Bạn nên tạo một ảnh bìa cho dự án và thay link vào đây -->

Một dự án DIY mạnh mẽ biến một bo mạch ESP32-S3 và module PN532 thành một trạm làm việc chuyên dụng để đọc, ghi, và sao chép thẻ MIFARE Classic 1K. Dự án này được thiết kế để hoạt động ổn định và hiệu quả, tích hợp các cơ chế tự sửa lỗi và quy trình sao chép hoàn chỉnh.

## 🌟 Tính năng chính

- **Đọc/Ghi Block Đơn lẻ:** Tương tác trực tiếp với từng block dữ liệu trên thẻ.
- **Dump Thẻ Tốc độ cao:** Đọc toàn bộ 64 block của thẻ một cách nhanh chóng bằng key đã biết.
- **Định dạng/Xóa trắng Thẻ:** Đưa thẻ về trạng thái "sạch", sẵn sàng cho mục đích sử dụng mới.
- **Sao chép Thẻ 2 bước (Copy/Paste):**
  - **Copy:** Đọc toàn bộ dữ liệu (bao gồm cả Block 0 chứa UID) từ thẻ gốc và lưu vào bộ nhớ của ESP32.
  - **Paste Data:** Ghi dữ liệu (block 1-63) lên thẻ đích.
- **Sao chép UID (Yêu cầu Magic Card):**
  - **Paste UID:** Ghi đè Block 0 của thẻ đích, tạo ra một bản sao 1:1 hoàn hảo.
- **Cơ chế Hoạt động Bền bỉ:** Tự động thử lại các thao tác (đọc thẻ, xác thực, ghi) khi gặp lỗi giao tiếp tạm thời, tăng độ ổn định.

## 🛠️ Phần cứng Yêu cầu

1.  **Bo mạch ESP32-S3:** Bất kỳ bo mạch phát triển nào sử dụng chip ESP32-S3.
2.  **Module NFC PN532:** Loại có thể hoạt động ở chế độ I2C (phổ biến nhất là bo mạch màu đỏ).
3.  **Thẻ MIFARE Classic 1K:** Để thử nghiệm các chức năng đọc/ghi.
4.  **Thẻ Magic Card Gen1/Gen2 (UID Changeable):** **Bắt buộc** phải có để thực hiện sao chép UID (chức năng "Paste UID").
5.  **Dây cắm (Jumper Wires):** 4 sợi để kết nối.
6.  **(Tùy chọn nhưng khuyến khích) 2x Điện trở 4.7kΩ:** Dùng làm điện trở kéo lên (pull-up) cho bus I2C để đảm bảo tín hiệu ổn định nhất.

## 🔌 Sơ đồ Nối dây (I2C)

**Quan trọng:** Đảm bảo các công tắc gạt (DIP switch) trên module PN532 được đặt ở chế độ I2C. Thường là `SW1=ON`, `SW2=OFF`.

```
      ESP32-S3                    PN532 Module
      +-------+                   +-----------+
      |  GND  |-------------------|    GND    |
      | 3.3V  |-------------------|    VCC    |
      | GPIO 8|-------------------|    SDA    |
      | GPIO 9|-------------------|    SCL    |
      +-------+                   +-----------+
```

**Để Tăng Độ Ổn Định (Khắc phục lỗi I2C):**
- Nối một điện trở 4.7kΩ giữa chân **3.3V** và **SDA (GPIO 8)**.
- Nối một điện trở 4.7kΩ khác giữa chân **3.3V** và **SCL (GPIO 9)**.

## ⚙️ Cài đặt & Thiết lập

### Bước 1: Cài đặt Firmware cho ESP32-S3

1.  **Mở Arduino IDE:** Đảm bảo bạn đã cài đặt board ESP32 trong Board Manager.
2.  **Cài đặt Thư viện:** Vào `Tools > Manage Libraries...` và cài đặt thư viện sau:
    - `Adafruit PN532` (của Adafruit)
3.  **Mở File Code:** Mở file `.ino` của dự án này.
4.  **Chọn Board:** Vào `Tools > Board` và chọn "ESP32S3 Dev Module" hoặc bo mạch tương ứng.
5.  **Nạp Code:** Kết nối ESP32-S3 với máy tính, chọn đúng cổng COM và nhấn nút "Upload".

### Bước 2: Chuẩn bị Môi trường Python trên Máy tính

1.  **Cài đặt Python:** Đảm bảo bạn đã cài đặt Python 3.6+ trên máy tính.
2.  **Cài đặt Thư viện:** Mở Terminal hoặc Command Prompt và chạy lệnh sau:
    ```bash
    pip install pyserial
    ```
3.  **Chuẩn bị File Wordlist (Tùy chọn):** Nếu bạn cần dò key cho các thẻ khác, hãy chuẩn bị một file `.txt` chứa danh sách các key, mỗi key trên một dòng.

## 🚀 Hướng dẫn Sử dụng

Chương trình được thiết kế để hoạt động qua Menu trên Arduino Serial Monitor.

1.  **Kết nối:** Cắm ESP32 đã nạp code vào máy tính.
2.  **Mở Serial Monitor:** Mở Arduino Serial Monitor và đặt baud rate là **115200**.
3.  **Tương tác:** Menu điều khiển sẽ hiện ra. Nhập số tương ứng với chức năng bạn muốn và làm theo hướng dẫn.

### Quy trình Sao chép Thẻ Hoàn chỉnh (Yêu cầu Magic Card)

1.  **[5] Sao chép (Copy):** Đặt thẻ gốc (thẻ cần sao chép) lên đầu đọc. Chương trình sẽ đọc toàn bộ dữ liệu và UID, sau đó lưu vào bộ nhớ.
2.  **[6] Dán Dữ liệu (Paste Data):** Tháo thẻ gốc ra, đặt Magic Card (thẻ đích) lên đầu đọc. Chương trình sẽ ghi dữ liệu từ block 1-63.
3.  **[7] Dán UID (Paste UID):** **Giữ nguyên Magic Card trên đầu đọc.** Chọn chức năng này để ghi đè Block 0 với UID của thẻ gốc.

Sau 3 bước trên, bạn sẽ có một bản sao 1:1 hoàn hảo.

## ⚠️ Cảnh báo & Lưu ý Quan trọng

- **Pháp lý:** Việc sao chép thẻ NFC có thể bị giới hạn bởi pháp luật. Chỉ sử dụng công cụ này trên những thẻ bạn sở hữu hoặc được phép sao chép.
- **Nguồn điện:** Nếu ESP32 liên tục khởi động lại (boot loop), hãy thử đổi dây cáp USB, đổi cổng USB trên máy tính, hoặc cấp nguồn ngoài ổn định hơn.
- **Không phải mọi thẻ đều có thể sao chép:** Các loại thẻ bảo mật cao hơn (MIFARE DESFire, iCLASS,...) không thể sao chép bằng phương pháp này.

## 📜 Giấy phép

Dự án này được phát hành dưới Giấy phép MIT. Xem file `LICENSE` để biết thêm chi tiết.