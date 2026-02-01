// Definicje enum (Stany Maszyny, Kroki Testu), struct SystemData (Globalny kontener na wyniki pomiarów).

#pragma once
#include <Arduino.h>

// ==========================================
// ENUMS - Stany Maszyny i Testów
// ==========================================

// Główne stany FSM (High-Level)
enum SystemState {
    STATE_BOOT,         // Inicjalizacja, kalibracja (tarowanie)
    STATE_READY,        // Czekanie na start (Safety Switch OFF)
    STATE_TEST_RUN,     // Wykonywanie sekwencji testowej
    STATE_ABORT,        // Nagłe przerwanie (Błąd, Panic, Limit Temp)
    STATE_FINISHED      // Po udanym teście (opcjonalnie, lub powrót do READY)
};

// Rodzaje dostępnych testów
enum TestType {
    TEST_NONE,
    TEST_MAP_RAMP,      // TEST 1: Mapa Silnika (Skokowe zwiększanie PWM)
    TEST_CONST_THRUST   // TEST 2: Stały Ciąg (PID/Seek target)
};

// Kroki wewnątrz sekwencji testowej (Low-Level SQN)
enum TestStep {
    STEP_IDLE,          // Czekanie
    STEP_RAMP_UP,       // Zmienianie PWM (lub szukanie ciągu)
    STEP_STABILIZE,     // Czekanie na uspokojenie wirów (TIME_STAB)
    STEP_MEASURE,       // Zbieranie danych do CSV (TIME_MEASURE)
    STEP_COOLDOWN       // Opcjonalne chłodzenie
};

// ==========================================
// DATA STRUCTURES - Kontenery danych
// ==========================================

// Główny kontener na dane pomiarowe.
// Idea: "Latest Known Value". Sensory wpisują tu asynchronicznie.
// Logika/BT czyta synchronicznie co 100ms.
struct SystemData {
    // Timestamps
    unsigned long sys_time_ms;  // Czas systemowy (millis)
    
    // Power (INA226)
    float voltage_V;            // Napięcie pakietu [V]
    float current_A;            // Prąd [A]
    float power_W;              // Moc [W] (V * A)
    
    // Thrust (HX711)
    float thrust_raw;           // Surowy odczyt (debug)
    float thrust_g;             // Ciąg po filtracji i tarowaniu [g]
    
    // Environment
    float temp_C;               // Temperatura silnika/ESC (DS18B20) [°C]
    
    // Control
    int current_PWM_us;         // Aktualnie wysterowany sygnał (1000-2000)
    int target_PWM_us;          // Cel sterowania
    
    // Logic Flags
    bool is_armed;              // Czy ESC jest uzbrojone
    TestStep current_step;      // Aktualny krok procedury
    int sample_count;           // Licznik próbek w bloku pomiarowym (dla uśredniania)
};

// Konfiguracja uruchomieniowa testu (wybierana guzikami/BT przed startem)
struct TestConfig {
    TestType selected_test;
    int pwm_start;              // Np. 1100
    int pwm_end;                // Np. 1900
    int pwm_step;               // Np. 50
    float target_thrust_g;      // Dla TEST 2
};