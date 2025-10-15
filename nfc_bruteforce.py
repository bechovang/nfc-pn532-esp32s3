#!/usr/bin/env python3
"""
NFC MIFARE Classic Brute-Force Tool v3.1 (Streaming + Key Selection)
Script điều khiển cho ESP32-S3 Worker chuyên dụng.
"""

import serial
import time
import sys
import json
import threading
import re
from datetime import datetime
from pathlib import Path

# --- Các hàm helper để xử lý wordlist ---

def parse_sectioned_wordlist(path):
    """
    Đọc file wordlist và phân loại key vào các mục nếu có.
    Mục được định nghĩa bởi các dòng header như '#### TÊN MỤC ####'.
    """
    sections = {}
    current_section = 'DEFAULT'
    header_re = re.compile(r"^\s*#+\s*(.+?)\s*#+\s*$")

    try:
        with open(path, 'r', encoding='utf-8', errors='ignore') as f:
            for line in f:
                line = line.strip()
                if not line:
                    continue
                
                match = header_re.match(line)
                if match:
                    current_section = match.group(1).strip().upper()
                    if current_section not in sections:
                        sections[current_section] = []
                    continue
                
                if re.fullmatch(r"[0-9A-Fa-f]{12}", line):
                    if current_section not in sections:
                        sections[current_section] = []
                    sections[current_section].append(line.upper())
    except Exception as e:
        print(f"✗ Lỗi khi phân tích wordlist: {e}")
        return {}
        
    return {k: v for k, v in sections.items() if v} # Chỉ trả về các mục có key

def get_keys_from_user_selection(wordlist_path):
    """
    Hiển thị menu cho người dùng chọn các bộ key và trả về danh sách key cuối cùng.
    """
    sections = parse_sectioned_wordlist(wordlist_path)
    
    # Nếu không có mục nào, hoặc chỉ có 1 mục, dùng tất cả key
    if not sections or len(sections) == 1:
        print(f"Đang đọc tất cả các key từ '{Path(wordlist_path).name}'...")
        all_keys = []
        for section_keys in sections.values():
            all_keys.extend(section_keys)
        # Nếu sections trống, đọc lại file theo cách thông thường
        if not all_keys:
             try:
                with open(wordlist_path, 'r', encoding='utf-8', errors='ignore') as f:
                    all_keys = [line.strip().upper() for line in f if re.fullmatch(r"[0-9A-Fa-f]{12}", line.strip())]
             except Exception:
                 return []
        unique_keys = list(dict.fromkeys(all_keys))
        print(f"✓ Đã load {len(unique_keys)} keys duy nhất.")
        return unique_keys

    # Nếu có nhiều mục, hiển thị menu
    print("\n" + "─"*50)
    print("Các bộ key được tìm thấy trong wordlist:")
    
    section_names = list(sections.keys())
    all_keys = [key for key_list in sections.values() for key in key_list]
    total_unique_keys = len(list(dict.fromkeys(all_keys)))

    print(f"  0) ALL ({total_unique_keys} keys duy nhất)")
    for i, name in enumerate(section_names, 1):
        print(f"  {i}) {name} ({len(sections[name])} keys)")
    
    choice_str = input("\nChọn bộ key để quét (ví dụ: '0' cho tất cả, hoặc '1,3' cho mục 1 và 3): ").strip()
    
    selected_keys = []
    try:
        choices = [int(c.strip()) for c in choice_str.split(',')]
        if 0 in choices or not choice_str:
            selected_keys.extend(all_keys)
        else:
            for i in choices:
                if 1 <= i <= len(section_names):
                    selected_keys.extend(sections[section_names[i-1]])
    except ValueError:
        print("Lựa chọn không hợp lệ, mặc định dùng TẤT CẢ keys.")
        selected_keys.extend(all_keys)
        
    final_keys = list(dict.fromkeys(selected_keys)) # Loại bỏ trùng lặp
    print(f"✓ Đã chọn {len(final_keys)} keys để brute-force.")
    print("─"*50)
    return final_keys


class NFCBruteForce:
    def __init__(self, port, baudrate=230400):
        self.port = port
        self.baudrate = baudrate
        self.ser = None
        self.uid = None
        self.sectors = {}
        self.total_keys_tested = 0
        self.session_start_time = None
        
        self.stop_event = threading.Event()
        self.found_key_data = {}

    def connect(self):
        try:
            self.ser = serial.Serial(self.port, self.baudrate, timeout=0.1)
            print(f"Đang chờ ESP32 worker sẵn sàng trên {self.port}...")
            time.sleep(2)
            return True
        except Exception as e:
            print(f"✗ Lỗi kết nối: {e}")
            return False

    def read_line_from_serial(self):
        try:
            return self.ser.readline().decode('utf-8', errors='ignore').strip()
        except (serial.SerialException, TypeError):
            self.stop_event.set()
            return None

    def send_command(self, cmd):
        try:
            self.ser.write(f"{cmd}\n".encode('utf-8'))
        except (serial.SerialException, TypeError):
            self.stop_event.set()

    def initialize_session(self):
        print("\n" + "="*50)
        print("Đang chờ thẻ NFC... (Đặt thẻ lên đầu đọc)")
        start_time = time.time()
        while time.time() - start_time < 30:
            line = self.read_line_from_serial()
            if not line: continue
            
            if "STATUS:" in line: print(f"  [ESP32]: {line}")
            if line.startswith("READY:UID:"):
                self.uid = line.split(":")[2]
                print(f"✓ Phát hiện thẻ! UID: {self.uid}")
                self.ser.reset_input_buffer()
                return True

        print("✗ Timeout! Không phát hiện thẻ nào.")
        return False

    def _stream_reader(self):
        while not self.stop_event.is_set():
            line = self.read_line_from_serial()
            if line and line.startswith("OK:"):
                try:
                    parts = line.split(":")
                    sector = int(parts[1])
                    key = parts[2]
                    self.found_key_data[sector] = line
                    print(f"\n  ✓✓✓ TÌM THẤY KEY cho Sector {sector}: {key} ✓✓✓")
                except (IndexError, ValueError):
                    pass # Bỏ qua format lỗi

    def _stream_writer_and_progress(self, keys_to_try):
        total_keys = len(keys_to_try)
        start_time = time.time()
        
        for i, (sector, key) in enumerate(keys_to_try):
            if self.stop_event.is_set() or len(self.found_key_data) == 16:
                break
            
            if sector not in self.found_key_data:
                self.send_command(f"KEY:{sector}:{key}")

            if i % 100 == 0:
                elapsed = time.time() - start_time
                rate = (i + 1) / elapsed if elapsed > 0 else 0
                self.total_keys_tested = i + 1
                found_count = len(self.found_key_data)
                print(f"  Đã thử {i+1:>7}/{total_keys} | {rate:,.0f} keys/s | Đã tìm thấy: {found_count}/16", end='\r')
        
        try: self.ser.flush()
        except: pass
        print(f"\n  Hoàn thành gửi keys.")
        self.stop_event.set()

    def run_brute_force(self, keys):
        self.session_start_time = time.time()
        tasks = [(sector, key) for sector in range(16) for key in keys]
        
        print(f"Bắt đầu brute-force với {len(keys)} keys cho mỗi sector chưa tìm thấy...")
        
        self.stop_event.clear()
        self.found_key_data.clear()

        reader = threading.Thread(target=self._stream_reader)
        writer = threading.Thread(target=self._stream_writer_and_progress, args=(tasks,))
        
        reader.start()
        writer.start()
        writer.join()
        time.sleep(1)
        self.stop_event.set()
        reader.join()
        
        for sector, data_line in self.found_key_data.items():
            parts = data_line.split(":")
            self.sectors[sector] = {"key": parts[2], "blocks": parts[3:7]}

    def end_session(self):
        self.send_command("STOP")
        time.sleep(0.5)
        self.show_results()
        self.save_results()

    def show_results(self):
        # Code này giữ nguyên, không cần thay đổi
        elapsed = time.time() - self.session_start_time
        rate = self.total_keys_tested / elapsed if elapsed > 0 else 0
        print("\n" + "="*50 + "\nKẾT QUẢ BRUTE-FORCE\n" + "="*50)
        print(f"UID: {self.uid}")
        print(f"Thời gian chạy: {elapsed:.2f}s (~{elapsed/60:.1f} phút)")
        print(f"Tổng số key đã thử: {self.total_keys_tested:,}")
        print(f"Tốc độ trung bình: {rate:,.0f} keys/s")
        print(f"\nSectors tìm được: {len(self.sectors)}/16")
        print("─"*50)
        for sector in range(16):
            if sector in self.sectors:
                print(f"  Sector {sector:2d}: ✓ {self.sectors[sector]['key']}")
            else:
                print(f"  Sector {sector:2d}: ✗ Không tìm được")
        print("="*50 + "\n")

    def save_results(self):
        # Code này giữ nguyên, không cần thay đổi
        if not self.uid: return
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"nfc_dump_{self.uid}_{timestamp}.json"
        data = {"uid": self.uid, "timestamp": datetime.now().isoformat(), "sectors": self.sectors}
        try:
            with open(filename, 'w') as f: json.dump(data, f, indent=2)
            print(f"✓ Đã lưu kết quả vào: {filename}")
        except Exception as e:
            print(f"✗ Lỗi lưu file: {e}")
            
    def close(self):
        if self.ser and self.ser.is_open:
            self.ser.close()
            print("✓ Đã ngắt kết nối")

def main():
    print("╔════════════════════════════════════════════════╗")
    print("║   NFC MIFARE Classic High-Speed Brute-Force    ║")
    print("║              v3.1 (Key Selection)              ║")
    print("╚════════════════════════════════════════════════╝")
    
    port = input("Nhập COM port của ESP32 (VD: COM9): ").strip()
    wordlist_path = input("Đường dẫn tới file wordlist (Enter để dùng 'wordlist.txt'): ").strip()
    
    if not port:
        print("✗ Cần phải có COM port!"); return
    if not wordlist_path:
        wordlist_path = "wordlist.txt"
    if not Path(wordlist_path).exists():
        print(f"✗ Không tìm thấy file wordlist tại: {wordlist_path}"); return
        
    keys = get_keys_from_user_selection(wordlist_path)
    if not keys:
        print("✗ Wordlist trống hoặc bị lỗi. Dừng chương trình."); return

    input("Nhấn Enter để bắt đầu kết nối và quét thẻ...")

    bruteforcer = NFCBruteForce(port)
    try:
        if not bruteforcer.connect(): return
        if not bruteforcer.initialize_session(): return
        bruteforcer.run_brute_force(keys)
        bruteforcer.end_session()
    except KeyboardInterrupt:
        print("\n\n⚠ Đã dừng bởi người dùng.")
        bruteforcer.stop_event.set()
    except Exception as e:
        print(f"\n✗ Đã xảy ra lỗi nghiêm trọng: {e}")
    finally:
        bruteforcer.close()

if __name__ == "__main__":
    main()