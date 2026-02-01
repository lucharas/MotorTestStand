// Główna Maszyna Stanów (High-Level): BOOT -> READY -> TEST_RUN -> STOP.

#include "config.h"
#include "types.h"

// ==========================================
// ZMIENNE GLOBALNE MODUŁU
// ==========================================

// Obiekt konfiguracji wybranego testu
TestConfig activeConfig;

// Referencje do globalnych zmiennych z main.ino
extern SystemData sysData;
extern SystemState currentSystemState;

// Obsługa przycisków (Debounce)
unsigned long lastBtnPress = 0;
const int DEBOUNCE_DELAY = 250;

// ==========================================
// FUNKCJE POMOCNICZE
// ==========================================

// Globalny Strażnik Bezpieczeństwa
// Zwraca true, jeśli jest bezpiecznie. False, jeśli trzeba przerwać.
bool fsm_CheckSafety() {
    // 1. Sprawdzenie temperatury (DS18B20)
    if (sensors_IsTempCritical()) {
        Serial.println("[FSM] SAFETY: Critical Temp!");
        return false;
    }

    // 2. Sprawdzenie prądu (INA226)
    if (sysData.current_A > MAX_CURRENT_A) {
        Serial.println("[FSM] SAFETY: Overcurrent!");
        return false;
    }

    // 3. Sprawdzenie napięcia (Lipo 6S protection)
    // Sprawdzamy tylko jeśli napięcie jest > 5V (żeby nie krzyczał przy podłączeniu USB)
    if (sysData.voltage_V > 5.0f && sysData.voltage_V < BATTERY_MIN_V) {
        // Serial.println("[FSM] SAFETY: Battery Low!"); 
        // Tu można dać tylko ostrzeżenie, hard stop może być irytujący przy słabym zasilaczu lab.
    }

    return true;
}

// Obsługa wejść (Input Handling)
void fsm_HandleButtons() {
    if (millis() - lastBtnPress < DEBOUNCE_DELAY) return;

    bool btnSel = !digitalRead(PIN_BTN_SELECT);   // Pullup: wciśnięty = 0 (zmienione na ! dla logiki)
    bool btnConf = !digitalRead(PIN_BTN_CONFIRM);

    if (!btnSel && !btnConf) return; // Brak akcji
    lastBtnPress = millis();

    // --- MASZYNA STANÓW OBSŁUGI GUZIKÓW ---
    
    switch (currentSystemState) {
        case STATE_BOOT:
            // W BOOT guziki mogą np. wymusić ponowne tarowanie wagi
            if (btnConf) {
                sensors_TareLoadCell();
            }
            break;

        case STATE_READY:
            // SELECT: Zmiana rodzaju testu (cyklicznie)
            if (btnSel) {
                if (activeConfig.selected_test == TEST_MAP_RAMP) {
                    activeConfig.selected_test = TEST_CONST_THRUST;
                    // Domyślne parametry dla CONST THRUST
                    activeConfig.target_thrust_g = 400.0f; 
                } else {
                    activeConfig.selected_test = TEST_MAP_RAMP;
                    // Domyślne parametry dla RAMP
                    activeConfig.pwm_start = 1100;
                    activeConfig.pwm_end = 1900;
                    activeConfig.pwm_step = 100;
                }
                Serial.print("[FSM] Test Mode: "); 
                Serial.println(activeConfig.selected_test == TEST_MAP_RAMP ? "RAMP" : "CONST");
            }

            // CONFIRM: Start Testu
            if (btnConf) {
                Serial.println("[FSM] ARMING MOTOR...");
                esc_Arm(); // Uzbrajanie ESC
                
                Serial.println("[FSM] STARTING TEST SEQUENCE");
                sqn_Reset(); // Reset sekwencera
                
                // Wysyłamy nagłówek CSV przez BT
                bt_SendHeader();
                
                currentSystemState = STATE_TEST_RUN;
            }
            break;

        case STATE_TEST_RUN:
            // W trakcie testu guzik CONFIRM to EMERGENCY STOP
            if (btnConf) {
                Serial.println("[FSM] USER ABORT!");
                currentSystemState = STATE_ABORT;
            }
            break;

        case STATE_FINISHED:
        case STATE_ABORT:
            // Jakikolwiek guzik resetuje do READY
            if (btnConf || btnSel) {
                Serial.println("[FSM] Reset to READY.");
                currentSystemState = STATE_READY;
            }
            break;
    }
}

// ==========================================
// GŁÓWNE API
// ==========================================

void fsm_Init() {
    Serial.println("[FSM] Init...");
    pinMode(PIN_BTN_SELECT, INPUT_PULLUP);
    pinMode(PIN_BTN_CONFIRM, INPUT_PULLUP);

    // Domyślna konfiguracja testu
    activeConfig.selected_test = TEST_MAP_RAMP;
    activeConfig.pwm_start = 1150;
    activeConfig.pwm_end = 1900;
    activeConfig.pwm_step = 50;
    
    currentSystemState = STATE_BOOT;
}

void fsm_Update() {
    // 1. Sprawdź bezpieczeństwo (Zawsze!)
    if (!fsm_CheckSafety() && currentSystemState == STATE_TEST_RUN) {
        currentSystemState = STATE_ABORT;
    }

    // 2. Obsłuż przyciski
    fsm_HandleButtons();

    // 3. Logika Stanów
    switch (currentSystemState) {
        case STATE_BOOT:
            // Tu czekamy na ustabilizowanie się czujników
            // Można dodać warunek, np. czekaj 3 sekundy
            if (millis() > 3000) {
                // Automatyczne przejście do READY po starcie
                Serial.println("[FSM] Boot Done -> READY");
                sensors_TareLoadCell(); // Auto-tare przy przejściu
                currentSystemState = STATE_READY;
            }
            break;

        case STATE_READY:
            // Czekamy na input użytkownika (obsłużone w HandleButtons)
            esc_Stop(); // Dla pewności trzymamy silnik zgaszony
            break;

        case STATE_TEST_RUN:
            // Przekazujemy sterowanie do Sekwencera (SQN)
            sqn_Update(&sysData);
            break;

        case STATE_ABORT:
            esc_Stop(); // HARD STOP
            sysData.is_armed = false;
            break;

        case STATE_FINISHED:
            esc_Stop(); // Normal STOP
            sysData.is_armed = false;
            break;
    }
}