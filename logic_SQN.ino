// Logika Testu (Low-Level): Sekwencje czasowe RAMP / STABILIZE / MEASURE. Tu żyje algorytm testu.

#include "config.h"
#include "types.h"

// ==========================================
// ZMIENNE GLOBALNE MODUŁU
// ==========================================

// Zmienne pomocnicze timera
unsigned long stateTimer = 0;
bool isTimerStarted = false;

// Referencje
extern TestConfig activeConfig; // Zdefiniowana w logic_FSM.ino
extern SystemState currentSystemState; // Żeby móc zakończyć test

// ==========================================
// FUNKCJE POMOCNICZE
// ==========================================

void sqn_Reset() {
    isTimerStarted = false;
    stateTimer = 0;
}

// Pomocnik do obsługi opóźnień (zamiast delay)
// Zwraca true, jeśli minął zadany czas
bool sqn_Wait(unsigned long duration_ms) {
    if (!isTimerStarted) {
        stateTimer = millis();
        isTimerStarted = true;
    }
    
    if (millis() - stateTimer >= duration_ms) {
        isTimerStarted = false; // Reset na przyszłość
        return true; // Czas minął
    }
    return false; // Jeszcze czekamy
}

// ==========================================
// LOGIKA KONKRETNYCH TESTÓW
// ==========================================

void run_TestRamp(SystemData* data) {
    switch (data->current_step) {
        case STEP_IDLE:
            // Ustawienie początkowe
            data->target_PWM_us = activeConfig.pwm_start;
            data->current_step = STEP_RAMP_UP;
            Serial.print("[SQN] Start PWM: "); Serial.println(data->target_PWM_us);
            break;

        case STEP_RAMP_UP:
            // Ustawienie PWM na ESC
            esc_SetPWM(data->target_PWM_us);
            
            // Przejście do stabilizacji (wody)
            Serial.println("[SQN] Stabilizing water...");
            data->current_step = STEP_STABILIZE;
            break;

        case STEP_STABILIZE:
            // Czekamy 6 sekund na uspokojenie wirów
            if (sqn_Wait(TIME_STAB_MS)) {
                Serial.println("[SQN] Measuring...");
                data->current_step = STEP_MEASURE;
            }
            break;

        case STEP_MEASURE:
            // Czekamy 4 sekundy zbierając dane (w tle hmi_BT je loguje)
            if (sqn_Wait(TIME_MEASURE_MS)) {
                // Koniec pomiaru dla tego punktu. Decyzja co dalej.
                
                int nextPWM = data->target_PWM_us + activeConfig.pwm_step;
                
                if (nextPWM > activeConfig.pwm_end || nextPWM > ESC_MAX_PWM) {
                    Serial.println("[SQN] Test Finished!");
                    currentSystemState = STATE_FINISHED;
                    data->current_step = STEP_IDLE;
                } else {
                    Serial.print("[SQN] Next Step: "); Serial.println(nextPWM);
                    data->target_PWM_us = nextPWM;
                    data->current_step = STEP_RAMP_UP; // Pętla
                }
            }
            break;
            
        default:
             data->current_step = STEP_IDLE;
             break;
    }
}

void run_TestConstThrust(SystemData* data) {
    // Prosty algorytm "Seek & Hold"
    // 1. Zwiększaj PWM aż osiągniesz cel
    // 2. Ustabilizuj
    // 3. Mierz przez 4s
    // 4. Koniec
    
    const int SEEK_STEP = 5; // Krok PWM przy szukaniu (precyzja)
    const int SEEK_INTERVAL = 50; // Co ile ms zmiana PWM (szybkość narastania)
    
    static unsigned long lastSeekTime = 0;

    switch (data->current_step) {
        case STEP_IDLE:
            data->target_PWM_us = ESC_MIN_PWM;
            data->current_step = STEP_RAMP_UP; // Używamy tego enum jako "Seek"
            Serial.print("[SQN] Seek Target: "); Serial.println(activeConfig.target_thrust_g);
            break;

        case STEP_RAMP_UP: // Tutaj: SEEKING
            if (millis() - lastSeekTime > SEEK_INTERVAL) {
                lastSeekTime = millis();
                
                // Algorytm Bang-Bang z histerezą
                if (data->thrust_g < activeConfig.target_thrust_g) {
                    data->target_PWM_us += SEEK_STEP;
                } else {
                    // Osiągnięto cel (lub przekroczono)
                    Serial.println("[SQN] Target Reached. Stabilizing...");
                    data->current_step = STEP_STABILIZE;
                }
                
                // Safety limit
                if (data->target_PWM_us > ESC_MAX_PWM) data->target_PWM_us = ESC_MAX_PWM;
                
                esc_SetPWM(data->target_PWM_us);
            }
            break;

        case STEP_STABILIZE:
            // Tu trzymamy stały PWM (ostatni wyznaczony) i czekamy
            if (sqn_Wait(TIME_STAB_MS)) {
                Serial.println("[SQN] Measuring Stability...");
                data->current_step = STEP_MEASURE;
            }
            break;

        case STEP_MEASURE:
            // Mierzymy jak pływa ciąg przy stałym PWM
            if (sqn_Wait(TIME_MEASURE_MS)) {
                Serial.println("[SQN] Done.");
                currentSystemState = STATE_FINISHED;
            }
            break;
            
        default: break;
    }
}

// ==========================================
// GŁÓWNE API
// ==========================================

void sqn_Update(SystemData* data) {
    // Router do odpowiedniej funkcji testowej
    if (activeConfig.selected_test == TEST_MAP_RAMP) {
        run_TestRamp(data);
    } else if (activeConfig.selected_test == TEST_CONST_THRUST) {
        run_TestConstThrust(data);
    }
}
