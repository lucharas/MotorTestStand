// Wizualizacja danych (tylko odczyt z SystemData).
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"
#include "types.h"

// ==========================================
// KONFIGURACJA EKRANU
// ==========================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1 // Reset współdzielony z ESP32 lub brak
#define SCREEN_ADDRESS 0x3C // Standardowy adres dla 0.96" OLED

// Obiekt ekranu. Przekazujemy &Wire, bo inicjalizacją I2C zajmuje się drv_Sensors.ino
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Timer odświeżania (non-blocking)
unsigned long lastOledUpdateTime = 0;
const unsigned long OLED_REFRESH_INTERVAL = 100; // 10Hz (co 100ms)

// ==========================================
// FUNKCJE POMOCNICZE (FORMATOWANIE)
// ==========================================

// Helper do konwersji stanu na napis
const char* getStateString(SystemState state) {
    switch(state) {
        case STATE_BOOT:     return "BOOT";
        case STATE_READY:    return "READY";
        case STATE_TEST_RUN: return "RUNNING";
        case STATE_ABORT:    return "ABORT!";
        case STATE_FINISHED: return "DONE";
        default:             return "???";
    }
}

// ==========================================
// API MODUŁU
// ==========================================

void oled_Init() {
    Serial.println("[OLED] Init...");
    
    // Próba startu. SSD1306_SWITCHCAPVCC to standardowe zasilanie wewnętrzne.
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println("[OLED] Allocation failed!");
        // Tutaj normalnie byłby Error Handler, ale w embedded po prostu lecimy dalej bez ekranu
        return; 
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    // Ekran powitalny (szybki test pikseli)
    display.setCursor(0, 0);
    display.println(F("Motor Test Stand"));
    display.println(F("v1.0"));
    display.println(F("Init..."));
    display.display();
}

void oled_Update(SystemData* data, SystemState state) {
    // 1. Dławik częstotliwości (Throttling)
    // Nie odświeżamy ekranu częściej niż 10Hz, szkoda I2C.
    unsigned long now = millis();
    if (now - lastOledUpdateTime < OLED_REFRESH_INTERVAL) {
        return;
    }
    lastOledUpdateTime = now;

    // 2. Rysowanie Ramki
    display.clearDisplay();

    // --- SEKJA ŻÓŁTA (Top 16px) ---
    // Status Systemu
    display.setTextSize(2); // Duża czcionka (14px wysokości znaków)
    display.setCursor(0, 0);
    display.print(getStateString(state));

    // Opcjonalnie: Timer testu w prawym rogu (jeśli zmieścimy)
    // Na razie prosto.

    // --- SEKCJA NIEBIESKA (Bottom 48px) ---
    // Start od y=18 (zostawiamy mały odstęp od żółtego paska)
    display.setTextSize(1); // Standardowa czcionka

    // Wiersz 1: Napięcie i Prąd
    display.setCursor(0, 18);
    display.print("V: "); display.print(data->voltage_V, 1); display.print("V");
    display.setCursor(64, 18); // Druga kolumna
    display.print("I: "); display.print(data->current_A, 1); display.print("A");

    // Wiersz 2: Ciąg i Moc
    display.setCursor(0, 28);
    display.print("T: "); display.print((int)data->thrust_g); display.print("g");
    display.setCursor(64, 28);
    display.print("P: "); display.print((int)data->power_W); display.print("W");

    // Wiersz 3: PWM i Temp
    display.setCursor(0, 38);
    display.print("PWM:"); display.print(data->current_PWM_us);
    display.setCursor(64, 38);
    display.print("C: "); display.print((int)data->temp_C); display.print((char)247); // Stopień

    // Wiersz 4: Statusy (Armed, Step)
    display.setCursor(0, 48);
    if (data->is_armed) {
        display.print("[ARMED] ");
    } else {
        display.print("[SAFE]  ");
    }
    
    // Pokazujemy krok testu jeśli trwa
    if (state == STATE_TEST_RUN) {
        display.print("S:"); 
        display.print(data->current_step);
    }

    // 3. Wypchnięcie bufora do ekranu
    display.display();
}