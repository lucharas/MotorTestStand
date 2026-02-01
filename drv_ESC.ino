// Wrapper na sterowanie silnikiem (PWM/Dshot).
#include <ESP32Servo.h>
#include "config.h"
#include "types.h"

// ==========================================
// OBIEKTY GLOBALNE
// ==========================================

Servo esc;

// Odwołanie do głównej struktury danych (z main.ino)
extern SystemData sysData; 

// ==========================================
// FUNKCJE INIT
// ==========================================

void esc_Init() {
    Serial.println("[ESC] Init PWM...");

    // Standardowe ESC oczekuje sygnału 50Hz (okres 20ms)
    // Niektóre nowsze ESC obsługują wyższe częstotliwości, ale 50Hz to bezpieczny standard.
    esc.setPeriodHertz(50); 
    
    // Attach: Pin, Min us, Max us
    // Biblioteka automatycznie mapuje write() na odpowiednie wypełnienie.
    esc.attach(PIN_ESC_SIG, ESC_MIN_PWM, ESC_MAX_PWM);

    // SAFETY: Natychmiastowe wymuszenie stanu niskiego przy starcie (Disarm)
    esc_Stop();
    
    Serial.println("[ESC] Signal Attached. Output: 1000us.");
}

// ==========================================
// INTERFACE STEROWANIA
// ==========================================

// Uzbrajanie ESC (Wysłanie min. sygnału)
// Uwaga: Większość ESC wymaga, aby sygnał trwał min. 2-3 sekundy, żeby ESC zagrało melodyjkę.
// Ta funkcja tylko ustawia wartość sygnału. Czas trwania musi obsłużyć FSM (w stanie BOOT).
void esc_Arm() {
    esc_SetPWM(ESC_ARM_PWM);
    // Ustawiamy flagę w systemie - logika wie, że jesteśmy gotowi
    sysData.is_armed = true; 
}

// Główna funkcja sterująca (Direct Drive)
void esc_SetPWM(int microseconds) {
    // 1. Hard Clamp (Bezpiecznik programowy)
    // Zabezpieczenie przed błędem w algorytmach PID/Ramp
    if (microseconds < ESC_MIN_PWM) microseconds = ESC_MIN_PWM;
    if (microseconds > ESC_MAX_PWM) microseconds = ESC_MAX_PWM;

    // 2. Deadzone Check (opcjonalny)
    // Można tu dodać logikę, że poniżej ESC_DEADZONE (np. 1050) wymuszamy 1000,
    // żeby silnik nie piszczał. Na razie zostawiam liniowo dla testów.
    
    // 3. Actuate
    esc.writeMicroseconds(microseconds);

    // 4. Update Telemetry (Feedback dla logiki i BT)
    // Kluczowe: Zapisujemy co WYSŁALIŚMY, a nie co chcieliśmy wysłać.
    sysData.current_PWM_us = microseconds;
}

// Hard Stop / Safety Cutoff / Disarm
// Funkcja powinna być wywoływana przy każdym przerwaniu testu.
void esc_Stop() {
    esc.writeMicroseconds(ESC_MIN_PWM); // Bezpośredni wpis 1000us
    
    // Aktualizacja stanu systemu
    sysData.current_PWM_us = ESC_MIN_PWM;
    sysData.target_PWM_us = ESC_MIN_PWM; // Reset celu, żeby PID nie próbował gwałtownie wracać po wznowieniu
    sysData.is_armed = false;
}