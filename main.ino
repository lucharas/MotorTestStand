// wersja 01.02.2026
// charakterystyka napędu (Thrust/Power)
// Koordynator. setup() inicjuje moduły. loop() wywołuje cyklicznie maszyny stanów i odświeżanie sensorów. Czysty kod, brak logiki biznesowej.

#include "config.h"
#include "types.h"

// ==========================================
// GLOBAL SYSTEM OBJECTS
// ==========================================
// To są jedyne instancje tych struktur w całym projekcie.
// Inne pliki (drivery, logika) odnoszą się do nich przez 'extern'.

SystemData sysData;
SystemState currentSystemState = STATE_BOOT;

// ==========================================
// SETUP (Jednorazowa konfiguracja)
// ==========================================
void setup() {
    // 1. Debug Serial
    Serial.begin(115200);
    // WARMUP: Czekamy chwilę na ustabilizowanie napięć po podłączeniu zasilania
    delay(1000); 
    
    Serial.println("\n[MAIN] System Booting...");

    // 2. Inicjalizacja Danych (Bezpieczne wartości startowe)
    sysData.sys_time_ms = 0;
    sysData.voltage_V = 0.0f;
    sysData.current_A = 0.0f;
    sysData.power_W = 0.0f;
    sysData.thrust_g = 0.0f;
    sysData.temp_C = 0.0f;
    sysData.current_PWM_us = 1000;
    sysData.target_PWM_us = 1000;
    sysData.is_armed = false;
    sysData.current_step = STEP_IDLE;
    sysData.sample_count = 0;

    // 3. Hardware Init Sequence (Kolejność ważna!)
    // Najpierw I2C i Sensory, żeby reszta nie wisiała na braku danych.
    sensors_Init(); 
    
    // Potem OLED, żeby pokazać status.
    oled_Init();    
    
    // Potem ESC (musi dostać sygnał STOP zanim włączymy zasilanie silnika)
    esc_Init();     
    
    // 4. Comms & Logic
    bt_Init();      // Bluetooth Serial
    fsm_Init();     // Reset Maszyny Stanów
    
    Serial.println("[MAIN] Boot Complete. Entering Loop.");
}

// ==========================================
// MAIN LOOP (Scheduler)
// ==========================================
void loop() {
    // 1. Akwizycja Danych (Hardware Layer)
    // Pobiera dane z INA226 (b. szybko), HX711 (80Hz) i Dallasa (asynchronicznie)
    sensors_UpdateAsync();

    // 2. Logika Systemu (Application Layer)
    // Decyduje o stanie systemu i steruje PWM
    fsm_Update();

    // 3. Interfejs Użytkownika (Presentation Layer)
    // Aktualizacja ekranu (wewnątrz jest limiter 10Hz)
    oled_Update(&sysData, currentSystemState);
    
    // Telemetria BT (tylko jeśli jest sens wysyłać)
    // Wysyłamy w stanie BOOT (kalibracja) i TEST_RUN (pomiary)
    if (currentSystemState == STATE_BOOT || currentSystemState == STATE_TEST_RUN) {
        bt_SendTelemetry(&sysData); // Uwaga: w config.h jest literówka 'Telemery', trzymamy się jej
    }
}
