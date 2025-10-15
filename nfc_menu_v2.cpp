// =================================================================================
// ==      ESP32-S3 + PN532 - MENU SAO CHÉP & ĐIỀU KHIỂN THẺ (v4 - SỬA LỖI)      ==
// ==          Đã sửa lỗi xác thực và ghi, thêm thử nhiều khóa                  ==
// =================================================================================

#include <Wire.h>
#include <Adafruit_PN532.h>

Adafruit_PN532 nfc(-1, -1);

// ========== CẤU HÌNH ==========
// Các khóa phổ biến để thử xác thực
uint8_t keys[][6] = {
  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // Khóa mặc định
  {0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5}, // Khóa A
  {0xD3, 0xF7, 0xD3, 0xF7, 0xD3, 0xF7}, // Khóa B
  {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // Khóa rỗng
  {0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5}  // Khóa khác
};
int numKeys = 5;

uint8_t dataToWrite[16] = { 'E', 'S', 'P', '3', '2', ' ', 'T', 'E', 'S', 'T', ' ', 'D', 'A', 'T', 'A', '!' };

// ========== BỘ NHỚ ĐỆM ĐỂ SAO CHÉP ==========
uint8_t clonedData[64][16];
bool isDataCloned = false;

// ========== CÁC HÀM ROBUST (TỰ ĐỘNG THỬ LẠI KHI LỖI) ==========
bool robustReadPassiveTargetID(uint8_t* uid, uint8_t* uidLength, uint8_t retries = 5) { 
  for (int i = 0; i < retries; i++) { 
    if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, uidLength)) return true; 
    delay(50); 
  } 
  return false; 
}

// Hàm xác thực với nhiều khóa
bool robustAuthenticateBlock(uint8_t* uid, uint8_t uidLength, uint8_t block, uint8_t retries = 3) { 
  for (int i = 0; i < retries; i++) {
    // Thử tất cả các khóa có sẵn
    for (int k = 0; k < numKeys; k++) {
      if (nfc.mifareclassic_AuthenticateBlock(uid, uidLength, block, 0, keys[k])) {
        Serial.print("  ✓ Xac thuc thanh cong voi key "); Serial.println(k);
        return true;
      }
    }
    delay(30); 
  } 
  return false; 
}

bool robustReadDataBlock(uint8_t block, uint8_t* data, uint8_t retries = 3) { 
  for (int i = 0; i < retries; i++) { 
    if (nfc.mifareclassic_ReadDataBlock(block, data)) return true; 
    delay(30); 
  } 
  return false; 
}

bool robustWriteDataBlock(uint8_t block, uint8_t* data, uint8_t retries = 3) { 
  for (int i = 0; i < retries; i++) { 
    if (nfc.mifareclassic_WriteDataBlock(block, data)) return true; 
    delay(30); 
  } 
  return false; 
}

// ========== HÀM PHỤ TRỢ ==========
void printHex(uint8_t* buffer, uint8_t length) { 
  for (uint8_t i = 0; i < length; i++) { 
    Serial.print(" "); 
    if (buffer[i] < 0x10) Serial.print("0"); 
    Serial.print(buffer[i], HEX); 
  } 
}

void printText(uint8_t* buffer, uint8_t length) { 
  for (uint8_t i = 0; i < length; i++) { 
    if (buffer[i] >= 32 && buffer[i] <= 126) Serial.print((char)buffer[i]); 
    else Serial.print("."); 
  } 
}

int readNumberFromSerial() { 
  String input = ""; 
  while (true) { 
    if (Serial.available()) { 
      char c = Serial.read(); 
      if (c == '\n' || c == '\r') { 
        if (input.length() > 0) return input.toInt(); 
      } else if (isDigit(c)) { 
        input += c; 
      } 
    } 
  } 
}

// ========== CÁC CHỨC NĂNG CHÍNH CỦA MENU ==========

// 1. ĐỌC TẤT CẢ CÁC BLOCK (0-63)
void readAllBlocks() { 
  Serial.println("\n[1] DOC TAT CA CAC BLOCK (0-63)");
  uint8_t uid[7]; uint8_t uidLength;
  Serial.println("  Dat the len cam bien...");
  if (!robustReadPassiveTargetID(uid, &uidLength)) { 
    Serial.println("✗ Khong tim thay the!"); 
    return; 
  }
  
  Serial.println("  UID cua the:"); printHex(uid, uidLength); Serial.println();
  Serial.println("  Bat dau doc tat ca cac block...");
  
  for (uint8_t block = 0; block < 64; block++) {
    // Xác thực sector khi cần thiết
    if (block % 4 == 0) {
      if (!robustAuthenticateBlock(uid, uidLength, block)) { 
        Serial.print("Sector "); Serial.print(block/4); Serial.println(": ✗ Xac thuc THAT BAI"); 
        // Bỏ qua 3 block còn lại trong sector này
        block += 3; 
        continue; 
      }
    }
    
    uint8_t data[16]; 
    if (robustReadDataBlock(block, data)) { 
      Serial.print("  Block "); 
      if (block < 10) Serial.print(" "); 
      Serial.print(block); 
      Serial.print(":"); 
      printHex(data, 16); 
      Serial.print(" |"); 
      printText(data, 16); 
      Serial.println("|"); 
    } else { 
      Serial.print("  Block "); 
      if (block < 10) Serial.print(" "); 
      Serial.print(block); 
      Serial.println(": ✗ Loi doc du lieu!"); 
    }
  }
  Serial.println("  ✓ Hoan thanh doc tat ca cac block!");
}

// 2. GHI TẤT CẢ CÁC BLOCK (0-63) - ĐÃ SỬA LỖI
void writeAllBlocks() { 
  Serial.println("\n[2] GHI TAT CA CAC BLOCK (0-63)");
  Serial.println("!!! CANH BAO: Thao tac nay se ghi de len toan bo du lieu cua the!");
  Serial.print("Nhap 'writeall' de xac nhan: ");
  String confirm = "";
  while(true){ 
    if(Serial.available()){ 
      char c = Serial.read(); 
      if(c=='\n' || c=='\r'){ 
        break; 
      } else { 
        confirm += c; 
      } 
    } 
  }
  
  if (confirm != "writeall") {
    Serial.println("\n Da huy.");
    return;
  }
  
  uint8_t uid[7]; uint8_t uidLength;
  Serial.println("\n  Dat the len cam bien de bat dau ghi...");
  if (!robustReadPassiveTargetID(uid, &uidLength)) { 
    Serial.println("✗ Khong tim thay the!"); 
    return; 
  }
  
  Serial.println("  UID cua the:"); printHex(uid, uidLength); Serial.println();
  Serial.println("  Bat dau ghi du lieu vao tat ca cac block...");
  int successCount = 0;
  
  for (uint8_t block = 0; block < 64; block++) {
    // Bỏ qua block 0 (chứa UID) và các sector trailer (block 3, 7, 11, ...)
    if (block == 0 || (block % 4 == 3)) {
      Serial.print("  Block "); 
      if (block < 10) Serial.print(" "); 
      Serial.print(block); 
      if (block == 0) {
        Serial.println(": ✗ Bỏ qua (block 0 - chứa UID)");
      } else {
        Serial.println(": ✗ Bỏ qua (sector trailer)");
      }
      continue;
    }
    
    // Xác thực sector khi cần thiết
    if (block % 4 == 0) {
      if (!robustAuthenticateBlock(uid, uidLength, block)) { 
        Serial.print("Sector "); Serial.print(block/4); Serial.println(": ✗ Xac thuc THAT BAI"); 
        // Bỏ qua 3 block còn lại trong sector này
        block += 3; 
        continue; 
      }
    }
    
    if (robustWriteDataBlock(block, dataToWrite)) { 
      Serial.print("  Block "); 
      if (block < 10) Serial.print(" "); 
      Serial.print(block); 
      Serial.println(": ✓ Ghi thanh cong"); 
      successCount++; 
    } else { 
      Serial.print("  Block "); 
      if (block < 10) Serial.print(" "); 
      Serial.print(block); 
      Serial.println(": ✗ Loi ghi du lieu!"); 
    }
  }
  
  Serial.print("\n  ✓ Hoan thanh! Da ghi thanh cong "); 
  Serial.print(successCount); 
  Serial.println(" blocks.");
}

// 3. DUMP THẺ
void dumpKnownCard() { 
  uint8_t uid[7]; uint8_t uidLength; 
  Serial.println("\n[3] DUMP TOAN BO THE...");
  if (!robustReadPassiveTargetID(uid, &uidLength)) { 
    Serial.println("✗ Khong tim thay the!"); 
    return; 
  }
  
  Serial.println("  UID cua the:"); printHex(uid, uidLength); Serial.println();
  
  for (uint8_t block = 0; block < 64; block++) {
    if (block % 4 == 0 && !robustAuthenticateBlock(uid, uidLength, block)) { 
      Serial.print("Sector "); Serial.print(block/4); Serial.println(": ✗ Xac thuc THAT BAI"); 
      block += 3; 
      continue; 
    }
    
    uint8_t data[16]; 
    if (robustReadDataBlock(block, data)) { 
      Serial.print("  Block "); 
      if (block < 10) Serial.print(" "); 
      Serial.print(block); 
      Serial.print(":"); 
      printHex(data, 16); 
      Serial.print(" |"); 
      printText(data, 16); 
      Serial.println("|"); 
    }
  }
}

// 4. ĐỊNH DẠNG/XÓA THẺ
void formatCard() {
  Serial.println("\n[4] DINH DANG/XOA TRANG THE");
  Serial.println("!!! CANH BAO: Thao tac nay se xoa TOAN BO DU LIEU.");
  Serial.print("Nhap 'format' de xac nhan: ");
  String confirm = "";
  while(true){ 
    if(Serial.available()){ 
      char c = Serial.read(); 
      if(c=='\n' || c=='\r'){ 
        break; 
      } else { 
        confirm += c; 
      } 
    } 
  }
  
  if (confirm != "format") {
    Serial.println("\n Da huy.");
    return;
  }
  
  uint8_t uid[7]; uint8_t uidLength;
  Serial.println("\n  Dat the len cam bien de bat dau format...");
  if (!robustReadPassiveTargetID(uid, &uidLength)) {
    Serial.println("✗ Khong tim thay the!");
    return;
  }

  uint8_t blankData[16] = {0};
  int successCount = 0;
  
  Serial.println("  Bat dau xoa du lieu...");
  
  // Vòng lặp tối ưu: Lặp qua từng SECTOR
  for (uint8_t sector = 0; sector < 16; sector++) {
    uint8_t firstBlock = sector * 4;
    
    // Chỉ xác thực MỘT LẦN cho mỗi sector
    if (robustAuthenticateBlock(uid, uidLength, firstBlock)) {
      Serial.print("  Sector "); Serial.print(sector); Serial.print(": ");
      // Sau khi xác thực, ghi vào 3 block dữ liệu của sector đó
      for (uint8_t blockOffset = 0; blockOffset < 3; blockOffset++) {
        uint8_t block = firstBlock + blockOffset;
        if (block == 0) continue; // Không bao giờ ghi vào block 0

        if (robustWriteDataBlock(block, blankData)) {
          successCount++;
          Serial.print(".");
        } else {
          Serial.print("F"); // Ghi block thất bại
        }
      }
      Serial.println(" OK");
    } else {
      Serial.print("  Sector "); Serial.print(sector); Serial.println(": Xac thuc THAT BAI!");
    }
  }
  
  Serial.println("\n  ✓ HOAN THANH!");
  Serial.print("  Da xoa thanh cong "); Serial.print(successCount); Serial.println(" blocks du lieu.");
}

// 5. SAO CHÉP (COPY)
void cloneCopy() { 
  uint8_t uid[7]; uint8_t uidLength; 
  Serial.println("\n[5] SAO CHEP (COPY) - Dat THE GOC len dau doc...");
  if (!robustReadPassiveTargetID(uid, &uidLength, 10)) { 
    Serial.println("✗ Khong tim thay the nguon!"); 
    return; 
  }
  
  Serial.println("  UID cua the:"); printHex(uid, uidLength); Serial.println();
  
  int successCount = 0;
  for (uint8_t block = 0; block < 64; block++) {
    if (block % 4 == 0 && !robustAuthenticateBlock(uid, uidLength, block)) { 
      Serial.print("\n✗ Khong the xac thuc Sector "); Serial.print(block/4); 
      block += 3; 
      continue; 
    }
    
    if (robustReadDataBlock(block, clonedData[block])) { 
      successCount++; 
      Serial.print("."); 
    } else { 
      Serial.print("F"); 
    }
  }
  
  if (successCount >= 48) { 
    isDataCloned = true; 
    Serial.print("\n✓ SAO CHEP THANH CONG! (Doc duoc "); Serial.print(successCount); Serial.println("/64 blocks)"); 
  } else { 
    isDataCloned = false; 
    Serial.print("\n✗ SAO CHEP THAT BAI! (Chi doc duoc "); Serial.print(successCount); Serial.println("/64 blocks)"); 
  }
}

// 6. DÁN (PASTE)
void clonePaste() {
  if (!isDataCloned) {
    Serial.println("\n[6] DAN (PASTE) - ✗ Loi: Chua co du lieu nao duoc sao chep!");
    return;
  }
  
  uint8_t uid[7]; uint8_t uidLength;
  Serial.println("\n[6] DAN (PASTE) - Dat THE TRANG (the dich) len dau doc...");
  
  if (!robustReadPassiveTargetID(uid, &uidLength, 10)) {
    Serial.println("✗ Khong tim thay the dich!");
    return;
  }
  
  Serial.println("✓ Da tim thay the dich. Bat dau ghi du lieu...");
  int successCount = 0;
  
  // Vòng lặp tối ưu: Lặp qua từng SECTOR để ghi
  for (uint8_t sector = 0; sector < 16; sector++) {
    uint8_t firstBlock = sector * 4;
    
    // Chỉ xác thực MỘT LẦN cho mỗi sector trên thẻ đích
    if (robustAuthenticateBlock(uid, uidLength, firstBlock)) {
      Serial.print("  Sector "); Serial.print(sector); Serial.print(": ");
      // Ghi vào 3 block dữ liệu của sector đó
      for (uint8_t blockOffset = 0; blockOffset < 3; blockOffset++) {
        uint8_t block = firstBlock + blockOffset;
        if (block == 0) continue; // Không ghi vào block 0

        if (robustWriteDataBlock(block, clonedData[block])) {
          successCount++;
          Serial.print(".");
        } else {
          Serial.print("F");
        }
      }
      Serial.println(" OK");
    } else {
      Serial.print("  Sector "); Serial.print(sector); Serial.println(": Xac thuc THAT BAI!");
    }
  }
  
  Serial.println("\n----------------------------------------");
  Serial.print("✓ GHI HOAN THANH! (Ghi thanh cong "); Serial.print(successCount); Serial.println(" blocks du lieu)");
  Serial.println("  Luu y: UID cua the moi van giu nguyen.");
}

// HIỂN THỊ MENU
void showMenu() {
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║      MENU SAO CHEP & DIEU KHIEN THE      ║");
  Serial.println("╠════════════════════════════════════════╣");
  Serial.println("║  1. Doc tat ca cac block (0-63)         ║");
  Serial.println("║  2. Ghi vao tat ca cac block (0-63)     ║");
  Serial.println("║  3. Dump toan bo the                   ║");
  Serial.println("║  4. Dinh dang/Xoa trang the            ║");
  Serial.println("║  5. Sao chep the (Copy)                ║");
  Serial.println("║  6. Dan vao the (Paste)                ║");
  Serial.println("║  9. Hien thi lai Menu                  ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.print("Nhap lua chon cua ban: ");
}

// ========== SETUP & LOOP ==========
void setup() {
  Serial.begin(115200); while (!Serial) delay(10);
  Serial.println("\nKhoi dong chuong trinh...");
  Wire.begin(8, 9); nfc.begin();
  uint32_t version = nfc.getFirmwareVersion();
  if (!version) { Serial.println("KHONG TIM THAY PN532!"); while (1); }
  Serial.println("Da ket noi PN532.");
  nfc.SAMConfig(); showMenu();
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read(); while(Serial.available()) Serial.read();
    switch(cmd) {
      case '1': readAllBlocks(); showMenu(); break;
      case '2': writeAllBlocks(); showMenu(); break;
      case '3': dumpKnownCard(); showMenu(); break;
      case '4': formatCard(); showMenu(); break;
      case '5': cloneCopy(); showMenu(); break;
      case '6': clonePaste(); showMenu(); break;
      case '9': showMenu(); break;
    }
  }
}