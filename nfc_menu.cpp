// CODE ĐẦY ĐỦ CHO PN532 + ESP32-S3 QUA I2C
// Chức năng: Đọc UID, Ghi dữ liệu, Đọc dữ liệu, Xác thực thẻ

#include <Wire.h>
#include <Adafruit_PN532.h>

// Khởi tạo PN532 cho I2C (không cần chân IRQ/RESET)
Adafruit_PN532 nfc(-1, -1);

// ========== CẤU HÌNH ==========
const uint8_t BLOCK_NUMBER = 4;  // Block để ghi/đọc dữ liệu

// Key mặc định (không dùng const để tương thích thư viện)
uint8_t KEY_A[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

// Danh sách keys phổ biến để thử (mở rộng thêm)
uint8_t commonKeys[][6] = {
  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // Key mặc định
  {0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5}, // MAD key
  {0xD3, 0xF7, 0xD3, 0xF7, 0xD3, 0xF7}, // NFCForum MAD key
  {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // Blank key
  {0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xF0}, // Common key 1
  {0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5}, // Common key 2
  {0x4D, 0x3A, 0x99, 0xC3, 0x51, 0xDD}, // Common key 3
  {0x1A, 0x98, 0x2C, 0x7E, 0x45, 0x9A}, // Common key 4
  {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}, // Common key 5
  {0x01, 0x02, 0x03, 0x04, 0x05, 0x06}, // Common key 6
  {0x00, 0x11, 0x22, 0x33, 0x44, 0x55}, // Sequential 1
  {0x11, 0x11, 0x11, 0x11, 0x11, 0x11}, // Repeated 1
  {0x22, 0x22, 0x22, 0x22, 0x22, 0x22}, // Repeated 2
  {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC}, // Sequential 2
  {0xAB, 0xCD, 0xEF, 0x12, 0x34, 0x56}, // Mixed 1
  {0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6}, // Mixed 2
  {0x48, 0x4F, 0x54, 0x45, 0x4C, 0x31}, // HOTEL1
  {0x42, 0x52, 0x45, 0x41, 0x4B, 0x31}, // BREAK1
  {0xD3, 0x5F, 0x9E, 0x2B, 0x7A, 0x11}, // Random 1
  {0x8F, 0xD0, 0xA4, 0xF2, 0x56, 0xE9}, // Random 2
};
const uint8_t NUM_KEYS = 20;

// Dữ liệu mẫu để ghi vào thẻ
uint8_t SAMPLE_DATA[16] = { 
  'E', 'S', 'P', '3', '2', '-', 'S', '3', 
  'N', 'F', 'C', 'T', 'e', 's', 't', 0 
};

// UID hợp lệ để kiểm tra (thay bằng UID thẻ của bạn)
uint8_t VALID_UID[4] = { 0x00, 0x00, 0x00, 0x00 };  // Đổi thành UID thật

// ========== HÀM PHỤ TRỢ ==========

// In mảng dạng HEX
void printHex(uint8_t* buffer, uint8_t length) {
  for (uint8_t i = 0; i < length; i++) {
    Serial.print(" 0x");
    if (buffer[i] < 0x10) Serial.print("0");
    Serial.print(buffer[i], HEX);
  }
}

// In mảng dạng ký tự
void printText(uint8_t* buffer, uint8_t length) {
  for (uint8_t i = 0; i < length; i++) {
    if (buffer[i] >= 32 && buffer[i] <= 126) {
      Serial.print((char)buffer[i]);
    } else {
      Serial.print(".");
    }
  }
}

// So sánh 2 mảng
bool compareArray(uint8_t* arr1, uint8_t* arr2, uint8_t length) {
  for (uint8_t i = 0; i < length; i++) {
    if (arr1[i] != arr2[i]) return false;
  }
  return true;
}

// Thử xác thực block với nhiều keys
bool authenticateBlock(uint8_t* uid, uint8_t uidLength, uint8_t block, uint8_t* foundKey) {
  for (uint8_t k = 0; k < NUM_KEYS; k++) {
    if (nfc.mifareclassic_AuthenticateBlock(uid, uidLength, block, 0, commonKeys[k])) {
      // Copy key thành công vào foundKey
      if (foundKey != nullptr) {
        memcpy(foundKey, commonKeys[k], 6);
      }
      return true;
    }
  }
  return false;
}

// Hiển thị menu
void showMenu() {
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║     MENU - PN532 NFC ESP32-S3         ║");
  Serial.println("╠════════════════════════════════════════╣");
  Serial.println("║  1. Doc UID the                        ║");
  Serial.println("║  2. Ghi du lieu vao the               ║");
  Serial.println("║  3. Doc du lieu tu the                ║");
  Serial.println("║  4. Xac thuc the bang UID             ║");
  Serial.println("║  5. Xac thuc the bang du lieu         ║");
  Serial.println("║  6. CLONE THE (backup)                 ║");
  Serial.println("║  7. Doc toan bo the (dump 30s)        ║");
  Serial.println("║  8. BRUTE-FORCE MODE (Python)          ║");
  Serial.println("║  9. Hien thi menu                      ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println("⚠️  CHU Y: Chi clone the CA NHAN/HOP PHAP!");
  Serial.println("Nhap lua chon hoac dat the gan cam bien...\n");
}

// ========== CÁC CHỨC NĂNG NFC ==========

// 1. Đọc UID
void readUID() {
  uint8_t uid[7] = {0};
  uint8_t uidLength;
  
  Serial.println("\n[1] DOC UID - Dat the gan cam bien...");
  
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
    Serial.println("✓ Tim thay the!");
    Serial.print("  Do dai UID: ");
    Serial.print(uidLength);
    Serial.println(" bytes");
    Serial.print("  UID:");
    printHex(uid, uidLength);
    Serial.println("\n");
  } else {
    Serial.println("✗ Khong tim thay the!");
  }
}

// 2. Ghi dữ liệu
void writeData() {
  uint8_t uid[7] = {0};
  uint8_t uidLength;
  
  Serial.println("\n[2] GHI DU LIEU - Dat the gan cam bien...");
  
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
    Serial.println("✓ Tim thay the!");
    Serial.print("  Xac thuc block ");
    Serial.print(BLOCK_NUMBER);
    Serial.println("...");
    
    if (nfc.mifareclassic_AuthenticateBlock(uid, uidLength, BLOCK_NUMBER, 0, KEY_A)) {
      Serial.println("✓ Xac thuc thanh cong!");
      Serial.println("  Dang ghi du lieu...");
      
      if (nfc.mifareclassic_WriteDataBlock(BLOCK_NUMBER, SAMPLE_DATA)) {
        Serial.println("✓ GHI THANH CONG!");
        Serial.print("  Du lieu da ghi: ");
        printText(SAMPLE_DATA, 16);
        Serial.println();
      } else {
        Serial.println("✗ Loi ghi du lieu!");
      }
    } else {
      Serial.println("✗ Loi xac thuc!");
    }
  } else {
    Serial.println("✗ Khong tim thay the!");
  }
}

// 3. Đọc dữ liệu
void readData() {
  uint8_t uid[7] = {0};
  uint8_t uidLength;
  uint8_t data[16];
  
  Serial.println("\n[3] DOC DU LIEU - Dat the gan cam bien...");
  
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
    Serial.println("✓ Tim thay the!");
    Serial.print("  Xac thuc block ");
    Serial.print(BLOCK_NUMBER);
    Serial.println("...");
    
    if (nfc.mifareclassic_AuthenticateBlock(uid, uidLength, BLOCK_NUMBER, 0, KEY_A)) {
      Serial.println("✓ Xac thuc thanh cong!");
      Serial.println("  Dang doc du lieu...");
      
      if (nfc.mifareclassic_ReadDataBlock(BLOCK_NUMBER, data)) {
        Serial.println("✓ DOC THANH CONG!");
        Serial.print("  HEX:");
        printHex(data, 16);
        Serial.println();
        Serial.print("  TEXT: ");
        printText(data, 16);
        Serial.println();
      } else {
        Serial.println("✗ Loi doc du lieu!");
      }
    } else {
      Serial.println("✗ Loi xac thuc!");
    }
  } else {
    Serial.println("✗ Khong tim thay the!");
  }
}

// 4. Xác thực bằng UID
void verifyByUID() {
  uint8_t uid[7] = {0};
  uint8_t uidLength;
  
  Serial.println("\n[4] XAC THUC BANG UID - Dat the gan cam bien...");
  
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
    Serial.print("  UID:");
    printHex(uid, uidLength);
    Serial.println();
    
    if (compareArray(uid, VALID_UID, uidLength)) {
      Serial.println("✓ THE HOP LE! ✓");
    } else {
      Serial.println("✗ THE KHONG HOP LE!");
      Serial.println("  (Cap nhat VALID_UID trong code neu can)");
    }
  } else {
    Serial.println("✗ Khong tim thay the!");
  }
}

// 5. Xác thực bằng dữ liệu
void verifyByData() {
  uint8_t uid[7] = {0};
  uint8_t uidLength;
  uint8_t data[16];
  
  Serial.println("\n[5] XAC THUC BANG DU LIEU - Dat the gan cam bien...");
  
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
    Serial.println("✓ Tim thay the!");
    
    if (nfc.mifareclassic_AuthenticateBlock(uid, uidLength, BLOCK_NUMBER, 0, KEY_A)) {
      if (nfc.mifareclassic_ReadDataBlock(BLOCK_NUMBER, data)) {
        Serial.print("  Du lieu doc duoc: ");
        printText(data, 16);
        Serial.println();
        
        if (compareArray(data, SAMPLE_DATA, 16)) {
          Serial.println("✓ THE HOP LE! (Du lieu trung khop) ✓");
        } else {
          Serial.println("✗ THE KHONG HOP LE! (Du lieu khong trung)");
        }
      } else {
        Serial.println("✗ Loi doc du lieu!");
      }
    } else {
      Serial.println("✗ Loi xac thuc!");
    }
  } else {
    Serial.println("✗ Khong tim thay the!");
  }
}

// 6. CLONE THẺ (Backup thẻ cá nhân) - với auto-detect keys
void cloneCard() {
  uint8_t originalUID[7] = {0};
  uint8_t uidLength;
  uint8_t allData[64][16]; // Lưu 64 blocks
  uint8_t sectorKeys[16][6]; // Lưu key của mỗi sector
  bool sectorAuth[16] = {false}; // Track sectors đã auth được
  
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║         CLONE THE (BACKUP)             ║");
  Serial.println("╠════════════════════════════════════════╣");
  Serial.println("║ ⚠️  CHI SU DUNG CHO THE CA NHAN!      ║");
  Serial.println("║ ⚠️  KHONG CLONE THE CUA NGUOI KHAC!   ║");
  Serial.println("╚════════════════════════════════════════╝\n");
  
  // BƯỚC 1: Đọc thẻ gốc
  Serial.println("BUOC 1: Dat THE GOC len cam bien...");
  delay(2000);
  
  if (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, originalUID, &uidLength)) {
    Serial.println("✗ Khong doc duoc the goc!");
    return;
  }
  
  Serial.println("✓ Tim thay the goc!");
  Serial.print("  UID:");
  printHex(originalUID, uidLength);
  Serial.println("\n");
  
  // Đọc tất cả blocks với auto-detect keys
  Serial.println("Dang doc du lieu tu the goc...");
  Serial.println("(Thu tat ca keys pho bien cho moi sector)\n");
  
  uint8_t successBlocks = 0;
  
  for (uint8_t sector = 0; sector < 16; sector++) {
    uint8_t firstBlock = sector * 4;
    
    // Thử authenticate sector này
    if (authenticateBlock(originalUID, uidLength, firstBlock, sectorKeys[sector])) {
      sectorAuth[sector] = true;
      Serial.print("  Sector ");
      Serial.print(sector);
      Serial.print(" [Key:");
      printHex(sectorKeys[sector], 6);
      Serial.print("] ");
      
      // Đọc các blocks trong sector (trừ trailer)
      uint8_t sectorSuccess = 0;
      for (uint8_t blockOffset = 0; blockOffset < 3; blockOffset++) {
        uint8_t block = firstBlock + blockOffset;
        if (nfc.mifareclassic_ReadDataBlock(block, allData[block])) {
          successBlocks++;
          sectorSuccess++;
        }
      }
      Serial.print(sectorSuccess);
      Serial.println("/3 blocks OK");
    } else {
      Serial.print("  Sector ");
      Serial.print(sector);
      Serial.println(" - KHONG AUTHENTICATE DUOC!");
    }
  }
  
  Serial.print("\n✓ Doc thanh cong ");
  Serial.print(successBlocks);
  Serial.println(" blocks");
  
  if (successBlocks == 0) {
    Serial.println("✗ Khong doc duoc block nao! Huy clone.");
    return;
  }
  
  // BƯỚC 2: Ghi vào thẻ trắng
  Serial.println("\n----------------------------------------");
  Serial.println("BUOC 2: THAO THE GOC ra!");
  Serial.println("        Dat THE TRANG len cam bien...");
  Serial.println("Cho 5 giay...");
  delay(5000);
  
  uint8_t newUID[7] = {0};
  if (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, newUID, &uidLength)) {
    Serial.println("✗ Khong doc duoc the trang!");
    return;
  }
  
  Serial.println("✓ Tim thay the trang!");
  Serial.print("  UID:");
  printHex(newUID, uidLength);
  Serial.println("\n");
  
  // Ghi từng block
  Serial.println("Dang ghi du lieu vao the trang...\n");
  uint8_t writeSuccess = 0;
  
  for (uint8_t sector = 0; sector < 16; sector++) {
    if (!sectorAuth[sector]) {
      Serial.print("  Sector ");
      Serial.print(sector);
      Serial.println(" - Bo qua (khong doc duoc tu the goc)");
      continue;
    }
    
    uint8_t firstBlock = sector * 4;
    
    // Thử authenticate với key mặc định hoặc key tìm được
    uint8_t* keyToUse = sectorKeys[sector];
    if (!nfc.mifareclassic_AuthenticateBlock(newUID, uidLength, firstBlock, 0, keyToUse)) {
      // Thử key mặc định
      uint8_t defaultKey[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
      if (!nfc.mifareclassic_AuthenticateBlock(newUID, uidLength, firstBlock, 0, defaultKey)) {
        Serial.print("  Sector ");
        Serial.print(sector);
        Serial.println(" - KHONG AUTHENTICATE DUOC the trang!");
        continue;
      }
    }
    
    Serial.print("  Sector ");
    Serial.print(sector);
    Serial.print(": ");
    
    // Ghi các blocks (trừ block 0 và trailers)
    uint8_t sectorWrite = 0;
    for (uint8_t blockOffset = 0; blockOffset < 3; blockOffset++) {
      uint8_t block = firstBlock + blockOffset;
      if (block == 0) continue; // Không ghi block 0 (UID)
      
      if (nfc.mifareclassic_WriteDataBlock(block, allData[block])) {
        writeSuccess++;
        sectorWrite++;
      }
    }
    Serial.print(sectorWrite);
    Serial.println("/3 blocks OK");
  }
  
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.print("║  ✓ CLONE HOAN THANH! (");
  if (writeSuccess < 10) Serial.print(" ");
  Serial.print(writeSuccess);
  Serial.println(" blocks)       ║");
  Serial.println("╠════════════════════════════════════════╣");
  Serial.println("║  LUU Y:                                ║");
  Serial.println("║  - UID cua the moi KHAC the goc       ║");
  Serial.println("║  - He thong kiem tra UID se KHONG     ║");
  Serial.println("║    nhan the nay!                       ║");
  Serial.println("║  - Can 'Magic Card' de clone UID      ║");
  Serial.println("╚════════════════════════════════════════╝\n");
}

// 7. Dump toàn bộ thẻ (với đọc nhiều lần trong 30s)
void dumpCard() {
  uint8_t uid[7] = {0};
  uint8_t uidLength;
  uint8_t allData[64][16]; // Lưu dữ liệu đã đọc được
  bool blockRead[64] = {false}; // Track blocks đã đọc
  uint8_t sectorKeys[16][6]; // Lưu key của mỗi sector
  bool sectorAuth[16] = {false}; // Track sectors đã auth
  
  Serial.println("\n[7] DUMP TOAN BO THE - Dat the gan cam bien...");
  
  if (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
    Serial.println("✗ Khong tim thay the!");
    return;
  }
  
  Serial.println("✓ Tim thay the!");
  Serial.print("  UID:");
  printHex(uid, uidLength);
  Serial.println("\n");
  Serial.println("========================================");
  Serial.println("Che do DOC NHIEU LAN trong 30 giay...");
  Serial.println("Thu tat ca keys va doc lai nhieu lan!");
  Serial.println("========================================\n");
  
  unsigned long startTime = millis();
  unsigned long duration = 30000; // 30 giây
  uint8_t attempt = 0;
  
  // Đọc liên tục trong 30 giây
  while (millis() - startTime < duration) {
    attempt++;
    uint8_t newReads = 0;
    
    Serial.print("Lan thu ");
    Serial.print(attempt);
    Serial.print(" (");
    Serial.print((duration - (millis() - startTime)) / 1000);
    Serial.println("s con lai)...");
    
    // Thử đọc từng sector
    for (uint8_t sector = 0; sector < 16; sector++) {
      uint8_t firstBlock = sector * 4;
      
      // Nếu chưa auth được sector này, thử lại
      if (!sectorAuth[sector]) {
        if (authenticateBlock(uid, uidLength, firstBlock, sectorKeys[sector])) {
          sectorAuth[sector] = true;
          Serial.print("  ✓ Sector ");
          Serial.print(sector);
          Serial.print(" authenticated! Key:");
          printHex(sectorKeys[sector], 6);
          Serial.println();
        }
      }
      
      // Nếu đã auth, thử đọc các blocks chưa đọc được
      if (sectorAuth[sector]) {
        for (uint8_t blockOffset = 0; blockOffset < 4; blockOffset++) {
          uint8_t block = firstBlock + blockOffset;
          
          // Nếu block này chưa đọc được, thử đọc
          if (!blockRead[block]) {
            if (nfc.mifareclassic_ReadDataBlock(block, allData[block])) {
              blockRead[block] = true;
              newReads++;
              Serial.print("    ✓ Block ");
              Serial.print(block);
              Serial.println(" doc thanh cong!");
            }
          }
        }
      }
    }
    
    // Đếm tổng số blocks đã đọc
    uint8_t totalRead = 0;
    for (uint8_t i = 0; i < 64; i++) {
      if (blockRead[i]) totalRead++;
    }
    
    Serial.print("  -> Tong: ");
    Serial.print(totalRead);
    Serial.print("/64 blocks (");
    Serial.print((totalRead * 100) / 64);
    Serial.println("%)");
    
    // Nếu đã đọc được hết, thoát sớm
    if (totalRead == 64) {
      Serial.println("\n✓✓✓ DOC DUOC TAT CA! Thoat som.");
      break;
    }
    
    // Nếu lần này không đọc được thêm gì, chờ rồi thử lại
    if (newReads == 0) {
      Serial.println("  (Khong doc them duoc, cho 500ms...)");
      delay(500);
    }
    
    Serial.println();
  }
  
  // Hiển thị kết quả chi tiết
  Serial.println("\n========================================");
  Serial.println("KET QUA DOC THE:");
  Serial.println("========================================\n");
  
  uint8_t totalSuccess = 0;
  
  for (uint8_t sector = 0; sector < 16; sector++) {
    Serial.print("Sector ");
    if (sector < 10) Serial.print(" ");
    Serial.print(sector);
    Serial.print(": ");
    
    if (sectorAuth[sector]) {
      Serial.print("[Key:");
      printHex(sectorKeys[sector], 6);
      Serial.println("]");
      
      for (uint8_t blockOffset = 0; blockOffset < 4; blockOffset++) {
        uint8_t block = sector * 4 + blockOffset;
        
        Serial.print("  Block ");
        if (block < 10) Serial.print(" ");
        Serial.print(block);
        Serial.print(": ");
        
        if (blockRead[block]) {
          totalSuccess++;
          printHex(allData[block], 16);
          Serial.print("  |");
          printText(allData[block], 16);
          Serial.println("|");
        } else {
          Serial.println("[KHONG DOC DUOC]");
        }
      }
    } else {
      Serial.println("[KHONG AUTHENTICATE DUOC]");
      for (uint8_t blockOffset = 0; blockOffset < 4; blockOffset++) {
        uint8_t block = sector * 4 + blockOffset;
        Serial.print("  Block ");
        if (block < 10) Serial.print(" ");
        Serial.print(block);
        Serial.println(": [Khong doc duoc]");
      }
    }
    Serial.println();
  }
  
  Serial.println("========================================");
  Serial.print("TONG KET: ");
  Serial.print(totalSuccess);
  Serial.print("/64 blocks (");
  Serial.print((totalSuccess * 100) / 64);
  Serial.println("%)");
  
  if (totalSuccess < 64) {
    Serial.println("\n⚠️  Mot so blocks van khong doc duoc!");
    Serial.println("   -> The co the dung keys tu dinh");
    Serial.println("   -> Hoac bi bao ve doc dac biet");
    Serial.println("   -> Can cong cu Proxmark3 de crack");
  } else {
    Serial.println("\n✓✓✓ DOC THANH CONG TOAN BO THE!");
  }
  Serial.println("========================================\n");
}

// 8. BRUTE-FORCE MODE - Giao tiếp với Python script
void bruteForceMode() {
  uint8_t uid[7] = {0};
  uint8_t uidLength;
  uint8_t sectorData[16][4][16]; // 16 sectors, 4 blocks, 16 bytes
  bool sectorFound[16] = {false};
  uint8_t sectorKeys[16][6];
  
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║      BRUTE-FORCE MODE                  ║");
  Serial.println("╠════════════════════════════════════════╣");
  Serial.println("║  Cho Python script ket noi...          ║");
  Serial.println("║  Dat the len cam bien va GIU YEN!      ║");
  Serial.println("╚════════════════════════════════════════╝\n");
  
  // Đọc thẻ
  Serial.println("STATUS:WAITING_CARD");
  
  unsigned long startWait = millis();
  while (millis() - startWait < 10000) { // Chờ 10 giây
    if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
      Serial.print("READY:UID:");
      for (uint8_t i = 0; i < uidLength; i++) {
        if (uid[i] < 0x10) Serial.print("0");
        Serial.print(uid[i], HEX);
      }
      Serial.println();
      break;
    }
    delay(200);
  }
  
  if (uidLength == 0) {
    Serial.println("ERROR:NO_CARD");
    return;
  }
  
  // Vào chế độ lắng nghe lệnh
  Serial.println("STATUS:READY");
  
  bool bruteRunning = true;
  String inputBuffer = "";
  
  while (bruteRunning) {
    // Đọc lệnh từ Serial
    while (Serial.available()) {
      char c = Serial.read();
      
      if (c == '\n' || c == '\r') {
        if (inputBuffer.length() > 0) {
          processCommand(inputBuffer, uid, uidLength, sectorData, sectorFound, sectorKeys, bruteRunning);
          inputBuffer = "";
        }
      } else {
        inputBuffer += c;
      }
    }
    
    delay(10);
  }
  
  Serial.println("STATUS:STOPPED");
}

// Xử lý lệnh từ Python
void processCommand(String cmd, uint8_t* uid, uint8_t uidLength, 
                   uint8_t sectorData[16][4][16], bool sectorFound[16], 
                   uint8_t sectorKeys[16][6], bool &running) {
  
  cmd.trim();
  
  // KEY:sector:hexkey (VD: KEY:0:162AA5FDE407)
  if (cmd.startsWith("KEY:")) {
    int firstColon = cmd.indexOf(':', 4);
    if (firstColon == -1) {
      Serial.println("ERROR:INVALID_FORMAT");
      return;
    }
    
    String sectorStr = cmd.substring(4, firstColon);
    String keyStr = cmd.substring(firstColon + 1);
    
    uint8_t sector = sectorStr.toInt();
    if (sector > 15) {
      Serial.println("ERROR:INVALID_SECTOR");
      return;
    }
    
    // Chuyển hex string thành bytes
    uint8_t key[6];
    if (!hexStringToBytes(keyStr, key, 6)) {
      Serial.println("ERROR:INVALID_KEY_FORMAT");
      return;
    }
    
    // Thử authenticate với key này (3 lần)
    bool success = false;
    uint8_t firstBlock = sector * 4;
    
    for (uint8_t attempt = 0; attempt < 3; attempt++) {
      // Đọc lại thẻ
      uint8_t tempUID[7];
      uint8_t tempLen;
      
      if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, tempUID, &tempLen)) {
        if (nfc.mifareclassic_AuthenticateBlock(tempUID, tempLen, firstBlock, 0, key)) {
          success = true;
          
          // Đọc tất cả blocks trong sector
          bool allRead = true;
          for (uint8_t blockOffset = 0; blockOffset < 4; blockOffset++) {
            uint8_t block = firstBlock + blockOffset;
            if (!nfc.mifareclassic_ReadDataBlock(block, sectorData[sector][blockOffset])) {
              allRead = false;
            }
          }
          
          if (allRead) {
            // Lưu key và đánh dấu sector
            memcpy(sectorKeys[sector], key, 6);
            sectorFound[sector] = true;
            
            // Gửi kết quả OK
            Serial.print("OK:");
            Serial.print(sector);
            Serial.print(":");
            for (uint8_t i = 0; i < 6; i++) {
              if (key[i] < 0x10) Serial.print("0");
              Serial.print(key[i], HEX);
            }
            Serial.print(":");
            
            // Gửi data của 4 blocks (format: block0_hex:block1_hex:...)
            for (uint8_t blockOffset = 0; blockOffset < 4; blockOffset++) {
              for (uint8_t i = 0; i < 16; i++) {
                if (sectorData[sector][blockOffset][i] < 0x10) Serial.print("0");
                Serial.print(sectorData[sector][blockOffset][i], HEX);
              }
              if (blockOffset < 3) Serial.print(":");
            }
            Serial.println();
          } else {
            Serial.println("PARTIAL:READ_ERROR");
          }
          
          break;
        }
      }
      
      if (attempt < 2) delay(100); // Chờ trước khi thử lại
    }
    
    if (!success) {
      Serial.println("FAIL");
    }
  }
  else if (cmd == "STOP") {
    running = false;
  }
  else if (cmd == "DUMP") {
    // Dump tất cả sectors đã tìm được
    Serial.println("DUMP_START");
    for (uint8_t s = 0; s < 16; s++) {
      if (sectorFound[s]) {
        Serial.print("SECTOR:");
        Serial.print(s);
        Serial.print(":");
        for (uint8_t i = 0; i < 6; i++) {
          if (sectorKeys[s][i] < 0x10) Serial.print("0");
          Serial.print(sectorKeys[s][i], HEX);
        }
        Serial.println();
      }
    }
    Serial.println("DUMP_END");
  }
  else if (cmd == "PING") {
    Serial.println("PONG");
  }
  else {
    Serial.println("ERROR:UNKNOWN_COMMAND");
  }
}

// Chuyển hex string thành bytes
bool hexStringToBytes(String hexStr, uint8_t* output, uint8_t length) {
  hexStr.toUpperCase();
  hexStr.replace(" ", "");
  
  if (hexStr.length() != length * 2) {
    return false;
  }
  
  for (uint8_t i = 0; i < length; i++) {
    String byteStr = hexStr.substring(i * 2, i * 2 + 2);
    char* endPtr;
    output[i] = strtol(byteStr.c_str(), &endPtr, 16);
    if (*endPtr != '\0') {
      return false;
    }
  }
  
  return true;
}

// ========== SETUP & LOOP ==========

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║  ESP32-S3 + PN532 NFC Reader v2.0     ║");
  Serial.println("╚════════════════════════════════════════╝");
  
  Wire.begin(8, 9);  // SDA=8, SCL=9
  nfc.begin();
  
  uint32_t version = nfc.getFirmwareVersion();
  if (!version) {
    Serial.println("\n✗ KHONG TIM THAY PN532!");
    Serial.println("Kiem tra:");
    Serial.println("  - Day cam (VCC, GND, SDA, SCL)");
    Serial.println("  - DIP switch che do I2C");
    Serial.println("  - Nguon 3.3V");
    while (1);
  }
  
  Serial.print("\n✓ Ket noi PN532 thanh cong!");
  Serial.print("\n  Firmware: ");
  Serial.print((version >> 24) & 0xFF, DEC);
  Serial.print(".");
  Serial.println((version >> 16) & 0xFF, DEC);
  
  nfc.SAMConfig();
  showMenu();
}

void loop() {
  // Kiểm tra lệnh từ Serial
  if (Serial.available()) {
    char cmd = Serial.read();
    
    switch(cmd) {
      case '1': readUID(); break;
      case '2': writeData(); break;
      case '3': readData(); break;
      case '4': verifyByUID(); break;
      case '5': verifyByData(); break;
      case '6': cloneCard(); break;
      case '7': dumpCard(); break;
      case '8': bruteForceMode(); break;
      case '9': showMenu(); break;
      default: 
        Serial.println("Lenh khong hop le! Nhap 1-9");
    }
    
    // Xóa buffer
    while(Serial.available()) Serial.read();
  }
  
  delay(100);
}