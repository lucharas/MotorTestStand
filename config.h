// Pinout, stałe (TIMING, LIMITS), makra kalibracyjne. Deklaracje funkcji (prototypy), aby uniknąć błędów kompilacji.

#pragma once
#include <Arduino.h>
#include "types.h" // Wymagane dla deklaracji funkcji używających typów

// ==========================================
// 1. PINOUT (ESP32 WROOM Defs)
// ==========================================

// I2C (INA226, OLED, BNO055)
#define PIN_I2C_SDA     21
#define PIN_I2C_SCL     22

// Load Cell (HX711) - Unikaj pinów Input-Only (34-39) jeśli to możliwe, ale HX711 to wejście
#define PIN_HX711_DT    18
#define PIN_HX711_SCK   19 

// Temp Sensor (DS18B20) - OneWire
#define PIN_ONE_WIRE    4 

// ESC Control (PWM/DShot)
#define PIN_ESC_SIG     16  // Hardware Timer capable

// User Interface
#define PIN_BTN_SELECT  13  // Button 1 (Next option)
#define PIN_BTN_CONFIRM 12  // Button 2 (Enter/Start)
// Panic Button jest fizyczny (odcięcie zasilania), nie definiujemy pinu.

// ==========================================
// 2. HARDWARE CONSTANTS & LIMITS (SAFETY)
// ==========================================

// System Limits
#define MAX_CURRENT_A       40.0f   // Absolutny limit (programowy bezpiecznik)
#define MAX_TEMP_C          80.0f   // Temp. krytyczna (Hard Stop)
#define WARN_TEMP_C         60.0f   // Temp. ostrzegawcza (Cooldown required)
#define BATTERY_MIN_V       21.0f   // 3.5V per cell dla 6S (ochrona LiPo)

// INA226 Settings
#define SHUNT_RESISTOR_OHM  0.005f  // Twój bocznik R005
#define INA_MAX_CURRENT     50.0f   // Zakres kalibracji

// HX711 Calibration (System End-to-End)
// 1. Zmontuj ramię.
// 2. Wgraj kod, odpal monitor portu.
// 3. Po uruchomieniu (Tare), powieś znany ciężar (np. 1000g) w miejscu silnika.
// 4. Zobacz jaki jest "raw reading" (surowy odczyt).
// 5. Oblicz: FACTOR = RAW_READING / MASA_W_GRAMACH.
// 6. Podmień wartość poniżej.

// Przykład: Jeśli 500g daje odczyt 100000, to factor = 200.0.
#define LOADCELL_CALIBRATION_FACTOR 200.0f 

// ESC Settings
#define ESC_MIN_PWM         1000
#define ESC_MAX_PWM         2000
#define ESC_ARM_PWM         1000    // Wartość "Zero" do uzbrojenia
#define ESC_DEADZONE        1180      // Poniżej 1050 silnik może nie kręcić stabilnie

// ==========================================
// 3. TIMING & LOGIC (Zgodne ze specyfikacją)
// ==========================================

// Częstotliwości pętli
#define LOOP_FREQ_HZ        50      // Główna pętla loop() (musi być szybsza niż 10Hz logowania)
#define LOG_FREQ_HZ         10      // Częstotliwość logowania CSV i BT
#define LOG_INTERVAL_MS     (1000 / LOG_FREQ_HZ)

// Stałe czasowe testu (zgodnie ze specyfikacją)
#define TIME_STAB_MS        6000    // Czas na uspokojenie wody
#define TIME_MEASURE_MS     4000    // Czas zbierania uśrednianych danych
#define TIME_RAMP_STEP_MS   1000    // Czas narastania (opcjonalny)

// Filtrowanie (Load Cell)
#define LOADCELL_ALPHA      0.1f    // Współczynnik filtra dolnoprzepustowego (0.0 - 1.0). Mniejszy = gładszy, większy lag.

// ==========================================
// 4. FUNCTION PROTOTYPES (Global Interface)
// ==========================================

// Aby uniknąć "implicit declaration" w Arduino IDE, deklarujemy funkcje globalne tutaj.

// drv_Sensors.ino
void sensors_Init();
void sensors_UpdateAsync(); // Wywoływane w loop() jak najczęściej
void sensors_TareLoadCell();
bool sensors_IsTempCritical();

// drv_ESC.ino
void esc_Init();
void esc_Arm();
void esc_SetPWM(int microseconds);
void esc_Stop(); // Natychmiastowe 1000us

// logic_FSM.ino
void fsm_Init();
void fsm_Update();

// logic_SQN.ino
void sqn_Reset();
void sqn_Update(SystemData* data); // Logika testu

// hmi_BT.ino
void bt_Init();
void bt_SendTelemetry(SystemData* data);
void bt_SendHeader();

// hmi_OLED.ino
void oled_Init();
void oled_Update(SystemData* data, SystemState state);