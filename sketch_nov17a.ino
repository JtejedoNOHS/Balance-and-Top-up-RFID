#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

// RFID configuration
#define RST_PIN 9 // Reset pin for MFRC522
#define SS_PIN 10 // Slave Select pin for MFRC522
MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance

// LCD configuration
LiquidCrystal_I2C lcd(0x27, 16, 2); // Adjust address if needed

// Keypad configuration
const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};
byte rowPins[ROWS] = {5, 6, 7, 8};  // Row pins connected to Arduino
byte colPins[COLS] = {A0, A1, A2};  // Column pins connected to Arduino
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Card Memory System
#define MAX_CARDS 10 // Maximum number of cards to store
struct Card {
  byte uid[4];   // Store UID (assumes 4-byte UID, adjust if necessary)
  float balance; // Store balance
};

Card cardMemory[MAX_CARDS]; // Array to store up to MAX_CARDS
int storedCardCount = 0;    // Number of cards currently in memory

// RFID Block and Balance Management
#define BLOCK 1 // Block to store balance on RFID tag
MFRC522::MIFARE_Key key;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("Initializing...");
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("RFID Reader Ready.");

  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;  // Default key for authentication

  lcd.init();
  lcd.backlight();
  lcd.print("Ready...");
  delay(1000);  // Shortened delay after startup
}

void loop() {
  // Waiting for the '*' key to be pressed to scan a card
  char keyPressed = keypad.getKey();
  if (keyPressed == '*') {
    // Scan the card when '*' is pressed
    lcd.clear();
    lcd.print("Scan your card...");
    Serial.println("Scan your card...");

    if (scanCard()) {
      int cardIndex = findCardInMemory();
      if (cardIndex >= 0) {
        // If the card is found in memory, show the menu options
        showMenu(cardIndex);
      } else {
        // If the card is not found, save it to memory
        saveCardUID();
        lcd.clear();
        lcd.print("New card saved!");
        delay(1000);  // Shortened delay after saving a new card
      }
    } else {
      indicateError("Scan Error");
    }
  }
}

bool scanCard() {
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    Serial.print("Card UID: ");
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      Serial.print(mfrc522.uid.uidByte[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
    return true;
  }
  return false;
}

int findCardInMemory() {
  // Look for the card's UID in stored memory
  for (int i = 0; i < storedCardCount; i++) {
    if (compareUID(mfrc522.uid.uidByte, cardMemory[i].uid)) {
      return i; // Return the index of the found card
    }
  }
  return -1; // Card not found
}

bool compareUID(byte uid1[], byte uid2[]) {
  for (int i = 0; i < 4; i++) {  // Assuming 4-byte UID, adjust if necessary
    if (uid1[i] != uid2[i]) {
      return false;
    }
  }
  return true;
}

void saveCardUID() {
  if (storedCardCount >= MAX_CARDS) {
    Serial.println("Memory full. Cannot store more UIDs.");
    return;
  }

  // Save the new card's UID and set its initial balance to 0
  for (int i = 0; i < 4; i++) {
    cardMemory[storedCardCount].uid[i] = mfrc522.uid.uidByte[i];
  }
  cardMemory[storedCardCount].balance = 0.0; // Initialize balance to 0 for new card
  storedCardCount++;
  Serial.println("Card UID saved successfully.");
}

void showMenu(int cardIndex) {
  // Show the menu to check balance or top up
  lcd.clear();
  lcd.print("1: Check Balance");
  lcd.setCursor(0, 1);
  lcd.print("2: Top-Up");

  // Wait for user to select option
  char menuChoice = keypad.getKey();
  while (!menuChoice) {
    menuChoice = keypad.getKey();
  }

  if (menuChoice == '1') {
    // Check balance
    lcd.clear();
    lcd.print("Balance: $");
    lcd.print(cardMemory[cardIndex].balance);
    Serial.print("Balance: $");
    Serial.println(cardMemory[cardIndex].balance);
    delay(1000);  // Shortened delay after balance check
  } else if (menuChoice == '2') {
    // Top-up balance
    lcd.clear();
    lcd.print("Current Balance: $");
    lcd.print(cardMemory[cardIndex].balance);
    delay(1000);  // Shortened delay after showing current balance

    lcd.clear();
    lcd.print("Enter amount:");
    float amount = getAmountFromKeypad();
    if (amount > 0) {
      cardMemory[cardIndex].balance += amount; // Add to the current balance
      lcd.clear();
      lcd.print("New Balance:");
      lcd.setCursor(0, 1);
      lcd.print("$");
      lcd.print(cardMemory[cardIndex].balance);
      Serial.print("New balance: $");
      Serial.println(cardMemory[cardIndex].balance);
    } else {
      indicateError("Invalid Input");
    }
    delay(1000);  // Shortened delay after top-up
  } else {
    lcd.clear();
    lcd.print("Invalid Option");
    delay(500);  // Shortened delay after invalid option
  }
}

float getAmountFromKeypad() {
  String amountStr = "";
  char key;
  while (true) {
    key = keypad.getKey();
    if (key) {
      if (key == '#') break;  // User presses # to confirm amount input
      else if (key >= '0' && key <= '9') {
        amountStr += key;
        lcd.setCursor(0, 1);
        lcd.print(amountStr);
      }
    }
  }
  return amountStr.toFloat();
}

void indicateError(String message) {
  lcd.clear();
  lcd.print(message);
  Serial.print("Error: ");
  Serial.println(message);
  delay(500);  // Shortened delay after an error
}
