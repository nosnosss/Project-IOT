#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <MFRC522.h>
#include <Keypad.h>

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

const char* ssid = "iPhoneST";
const char* pw = "0987654321";
const char *serverUrl = "http://192.168.0.104:3000/record";

// inisialisasi Bot Token
#define BOTtoken "6158547813:AAHn6RAnJ_cFKB2W4aSk3ij27ymyCOL_ImE"  // Bot Token dari BotFather

// chat id dari @myidbot
#define CHAT_ID "5578935888"

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

int botRequestDelay = 1000;
unsigned long lastTimeBotRan;

#define LED 4
#define RST_PIN 22
#define SS_PIN 5 // Chọn một chân digital khác để kết nối với RST_PIN của MFRC522
#define RELAY_PIN 21
#define MAX_RFID_CARDS 6
String secondaryCards[MAX_RFID_CARDS];
int numSecondaryCards = 0;
// Khởi tạo đối tượng LCD và RFID
LiquidCrystal_I2C lcd(0x27, 16, 2);
MFRC522 mfrc522(SS_PIN, RST_PIN);
// Định nghĩa các thông tin cần thiết
const byte ROW_NUM    = 4; // Số hàng của bàn phím ma trận
const byte COLUMN_NUM = 4; // Số cột của bàn phím ma trận
char customKeymap[ROW_NUM][COLUMN_NUM] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte pin_rows[ROW_NUM]    = {32, 33, 25, 26}; // Chân nối các hàng của bàn phím ma trận
byte pin_column[COLUMN_NUM] = {27, 14, 12, 13}; // Chân nối các cột của bàn phím ma trận
Keypad my_keypad = Keypad(makeKeymap(customKeymap), pin_rows, pin_column, ROW_NUM, COLUMN_NUM);
byte icon[] = {
  B01110, B10001, B01110, B00100, B11111, B00100, B01010, B10001
};
// Thông tin thẻ RFID chính và mật khẩu
String mainCardID = "73B6D60D";
String password = "1234";

// Thiết lập chương trình
void setup() {
  Serial.begin(115200);
  Wire.begin(17, 16);
  lcd.init();
  lcd.backlight();
  SPI.begin();
  mfrc522.PCD_Init();
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Relay ngắt đầu tiên
  pinMode(LED, OUTPUT);
  lcd.clear();
  lcd.createChar(0, icon);
  lcd.setCursor(0, 0);
  lcd.write(0);
  lcd.setCursor(2, 0);
  lcd.print("XIN CHAO !");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pw);
  #ifdef ESP32
    client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  #endif
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());
}

// Hàm chính lặp lại liên tục
void loop() {
  if (millis() > lastTimeBotRan + botRequestDelay)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while(numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }

  if (digitalRead(RELAY_PIN) == HIGH){
    digitalWrite(LED, HIGH);
  }else{
    digitalWrite(LED, LOW);
  }
  char key = my_keypad.getKey();
  if (key) {
    handleKeypress(key);
  }
  // Kiểm tra nếu có thẻ RFID mới được đặt lên
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    String cardID = getCardID();
    if (isMainCard(cardID) || isSecondaryCard(cardID)) {
    // Kiểm tra xem thẻ có phải là thẻ chính hoặc thẻ phụ hay không
      digitalWrite(RELAY_PIN, LOW); // Đảo trạng thái relay
      lcd.clear();
      Serial.println("Relay: " + String(digitalRead(RELAY_PIN) == HIGH ? "Ngat" : "Dong"));
      if (digitalRead(RELAY_PIN) == HIGH){
        lcd.clear();
        lcd.createChar(0, icon);
        lcd.setCursor(0, 0);
        lcd.write(0);
        lcd.setCursor(2, 0);
        lcd.print("XIN CHAO !!");
      }else{
        lcd.clear();
        lcd.print(" CUA MO ");
        if (sendDataToServer(cardID)) {}
      }
      if(isSecondaryCard(cardID)){
        char checkKey = 0;
        while (!checkKey) {
          checkKey = my_keypad.getKey();
        }
        if(checkKey == 'C'){
          deleteRFIDCard(cardID);
          String xxx = "DAXOATHE:"+cardID;
          if (sendDataToServer(xxx)) {}
        }
      }
    }
    else {
      lcd.clear();
      lcd.print("KHONG CO QUYEN");
      Serial.println("KHONG CO QUYEN");
      String xxx = "THELA:"+cardID;
      if (sendDataToServer(xxx)) {}
      lcd.setCursor(0, 1);
      lcd.print("B: THEM THE ||");     
      char secondaryKey = 0;
      while (!secondaryKey) {
        secondaryKey = my_keypad.getKey();
      }
      if (secondaryKey == 'B') {
        Serial.println("tien hanh them the");
        addRFIDCard(cardID);
        String xxx = "DATHEMTHE:"+cardID;
        if (sendDataToServer(xxx)) {}
      }
      else{
        lcd.clear();
        lcd.createChar(0, icon);
        lcd.setCursor(0, 0);
        lcd.write(0);
        lcd.setCursor(2, 0);
        lcd.print("XIN CHAO !!");
      }
    }
    delay(1000); // Tránh đọc thẻ liên tục
  }
}

bool sendDataToServer(String cardID) {
  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String postData = "cardID=" + cardID;
  int httpResponseCode = http.POST(postData);

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Server response: " + response);
    http.end();
    return true;
  } else {
    Serial.println("Error on sending data to server");
    http.end();
    return false;
  }
}

// Xử lý sự kiện khi nhấn các phím trên bàn phím ma trận
void handleKeypress(char key) {
  switch (key) {
    case 'A':
      enterPassword();
      break;
    case 'D':
      if (digitalRead(RELAY_PIN) == LOW) {
        changePassword();
      }
      break;
    case '#':
      if (digitalRead(RELAY_PIN) == LOW) {
        resetPassword();
      }
      break;
    case '*':
      if (digitalRead(RELAY_PIN) == LOW) {
        digitalWrite(RELAY_PIN, HIGH); // Ngắt relay
        lcd.clear();
        lcd.createChar(0, icon);
        lcd.setCursor(0, 0);
        lcd.write(0);
        lcd.setCursor(2, 0);
        lcd.print("XIN CHAO !");
        Serial.println("Relay: Ngat");
      }
      break;
    default:
      break;
  }
}

// Nhập mật khẩu từ bàn phím ma trận
void enterPassword() {
  lcd.clear();
  lcd.print("NHAP MAT KHAU:");
  Serial.println("Nhap mat khau:");
  lcd.setCursor(0, 1);
  String enteredPassword = "";
  while (enteredPassword.length() < password.length()) {
    char key = my_keypad.getKey();
    if (key) {
      enteredPassword += key;
      Serial.print("*");
      lcd.print('*');
    }
  }
  if (enteredPassword.equals(password)) {
    digitalWrite(RELAY_PIN, !digitalRead(RELAY_PIN)); // Đảo trạng thái relay
    lcd.clear();
    lcd.print(" CUA MO");
    String xxx = "MOCUABANGMK";
    if (sendDataToServer(xxx)) {}
    // lcd.print(digitalRead(RELAY_PIN) == HIGH ? "Ngat" : "Dong");
    Serial.println("Relay: " + String(digitalRead(RELAY_PIN) == HIGH ? "Ngat" : "Dong"));
  } 
  else {
    lcd.clear();
    lcd.print("MAT KHAU SAI");
    Serial.println("Mat khau sai");
    String xxx = "NHAPMKSAI";
    if (sendDataToServer(xxx)) {}
    delay(2000);
    lcd.clear();
    lcd.createChar(0, icon);
    lcd.setCursor(0, 0);
    lcd.write(0);
    lcd.setCursor(2, 0);
    lcd.print("XIN CHAO !");
  }
}

// Thêm thẻ RFID mới vào danh sách thẻ phụ
void addRFIDCard(String cardID) {
  lcd.clear();
  lcd.print("NHAP MAT KHAU:");
  Serial.println("Nhap mat khau:");
  lcd.setCursor(0, 1);
  String enteredPassword = "";
  while (enteredPassword.length() < password.length()) {
    char key = my_keypad.getKey();
    if (key) {
      enteredPassword += key;
      Serial.print("*");
      lcd.print('*');
    }
  }
  if (enteredPassword.equals(password)) {
    if (!isMainCard(cardID) && !isSecondaryCard(cardID)) {
      if (numSecondaryCards <= MAX_RFID_CARDS) {
        // Thêm cardID vào danh sách thẻ phụ
        secondaryCards[numSecondaryCards++] = cardID;
        lcd.clear();
        lcd.print("THEM THANH CONG");
        Serial.println("THEM THANH CONG");
        digitalWrite(RELAY_PIN, HIGH);
      } else {
        lcd.clear();
        lcd.print("DANH SACH DAY");
        Serial.println("DANH SACH DAY");
      }
    } else {
      lcd.clear();
      lcd.print("KHONG THE THEM");
      Serial.println("Khong the them");
    }
    delay(2000);
    lcd.clear();
    lcd.createChar(0, icon);
    lcd.setCursor(0, 0);
    lcd.write(0);
    lcd.setCursor(2, 0);
    lcd.print("XIN CHAO !");
  } 
  else {
    lcd.clear();
    lcd.print("MAT KHAU SAI");
    Serial.println("Mat khau sai");
    delay(2000);
    lcd.clear();
    lcd.createChar(0, icon);
    lcd.setCursor(0, 0);
    lcd.write(0);
    lcd.setCursor(2, 0);
    lcd.print("XIN CHAO !");
  }
}

// Xóa thẻ RFID khỏi danh sách thẻ phụ
void deleteRFIDCard(String cardID) {
  lcd.clear();
  lcd.print("XOATHE: " + cardID);
  Serial.println("XOATHE: " + cardID);
  for (int i = 0; i < numSecondaryCards; i++) {
    if (cardID.equals(secondaryCards[i])) {
      // Di chuyển tất cả các phần tử phía sau về trước để ghi đè cardID cần xóa
      for (int j = i; j < numSecondaryCards - 1; j++) {
        secondaryCards[j] = secondaryCards[j + 1];
      }
      numSecondaryCards--;
      break;
    }
  }
  lcd.setCursor(0, 1);
  lcd.print("=> XOA THE XONG");
  Serial.println("Xoa xong the: " + cardID);
  digitalWrite(RELAY_PIN, HIGH);
  delay(2000);
  lcd.clear();
  lcd.createChar(0, icon);
  lcd.setCursor(0, 0);
  lcd.write(0);
  lcd.setCursor(2, 0);
  lcd.print("XIN CHAO !");
}

// Thay đổi mật khẩu
void changePassword() {
  lcd.clear();
  lcd.print("NHAP MK MOI:");
  Serial.println("NHAP MK MOI:");
  lcd.setCursor(0, 1);
  String newPassword = "";
  while (newPassword.length() < password.length()) {
    char key = my_keypad.getKey();
    if (key) {
      newPassword += key;
      lcd.print("*");
      Serial.print("*");
    }
  }
  // Xác nhận mật khẩu mới
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("NHAP LAI MK MOI:");
  lcd.setCursor(0, 1);
  String confirmedPassword = "";
  while (confirmedPassword.length() < password.length()) {
    char key = my_keypad.getKey();
    if (key) {
      confirmedPassword += key;
      lcd.print("*");
      Serial.print("*");
    }
  }
  if (confirmedPassword.equals(newPassword)){
    password = newPassword;
    lcd.clear();
    lcd.print("MAT KHAU DA DOI");
    Serial.println("MAT KHAU DA DOI");
  }
  else{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("XAC NHAN SAI");
  }
  delay(2000);
  lcd.clear();
  lcd.createChar(0, icon);
  lcd.setCursor(0, 0);
  lcd.write(0);
  lcd.setCursor(2, 0);
  lcd.print("XIN CHAO !");
}

// Reset mật khẩu về mật khẩu mặc định
void resetPassword() {
  lcd.clear();
  lcd.print("RESET MAT KHAU");
  Serial.println("Reset mat khau");
  password = "1234";
  lcd.clear();
  lcd.print("RESET THANH CONG");
  Serial.println("Mat khau da reset");
  delay(2000);
  lcd.clear();
  lcd.createChar(0, icon);
  lcd.setCursor(0, 0);
  lcd.write(0);
  lcd.setCursor(2, 0);
  lcd.print("XIN CHAO !");
}

// Lấy ID của thẻ RFID
String getCardID() {
  String cardID = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    cardID += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
    cardID += String(mfrc522.uid.uidByte[i], HEX);
  }
  cardID.toUpperCase();
  return cardID;
}

bool isMainCard(String cardID) {
  return cardID.equals(mainCardID);
}

bool isSecondaryCard(String cardID) {
  for (int i = 0; i < numSecondaryCards; i++) {
    if (cardID.equals(secondaryCards[i])) {
      return true;
    }
  }
  return false;
}

void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i=0; i<numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID){
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }
    
    String text = bot.messages[i].text;
    Serial.println(text);

    String from_name = bot.messages[i].from_name;

    if (text == "/start") {
      String control = "Ngoc Son, " + from_name + ".\n";
      control += "Chao mung den voi he thong cua khoa thong minh.\n\n";
      control += "/relay_dong Mo cua tu xa.\n";
      control += "/relay_ngat Dong cua tu xa.\n";
      control += "/Status Kiem tra trang thai cua \n";
      bot.sendMessage(chat_id, control, "");
    }

    if (text == "/relay_dong") {
      bot.sendMessage(chat_id, "Cua da mo", "");
      digitalWrite(RELAY_PIN, LOW);
      lcd.clear();
      lcd.print(" CUA MO ");
      String xxx = "MOCUATUXAQUATELEGRAM";
      if (sendDataToServer(xxx)) {}
    }
    
    if (text == "/relay_ngat") {
      bot.sendMessage(chat_id, "Cua da dong", "");
      digitalWrite(RELAY_PIN, HIGH);
      lcd.clear();
      lcd.createChar(0, icon);
      lcd.setCursor(0, 0);
      lcd.write(0);
      lcd.setCursor(2, 0);
      lcd.print("XIN CHAO !");
    }
    
    if(text == "/Status") {
      if (digitalRead(RELAY_PIN)){
        bot.sendMessage(chat_id, "Cua dang dong", "");
      }
      else{
        bot.sendMessage(chat_id, "Cua dang mo", "");
      }
    }
  }
}