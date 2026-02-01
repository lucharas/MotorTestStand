// Wysyłanie pakietów CSV przez Bluetooth Serial.

#include "BluetoothSerial.h"
#include "config.h"
#include "types.h"

// ==========================================
// OBIEKTY GLOBALNE
// ==========================================

BluetoothSerial SerialBT;

// Timer do limitowania wysyłki danych (1Hz)
unsigned long lastBtLogTime = 0;
const unsigned long BT_REPORT_INTERVAL_MS = 1000; // Sztywne 1Hz dla raportów

// Bufory do uśredniania (Averaging)
// Zbieramy sumy z ~50 cykli loop(), żeby raz na sekundę wysłać średnią
float sum_thrust = 0.0f;
float sum_voltage = 0.0f;
float sum_current = 0.0f;
float sum_power = 0.0f;
float sum_temp = 0.0f;
long  sum_pwm = 0;
int   avg_sample_count = 0;

// ==========================================
// API MODUŁU
// ==========================================

void bt_Init() {
    Serial.println("[BT] Init...");
    
    // Uruchomienie z nazwą widoczną w telefonie
    if (!SerialBT.begin("MotorTestStand")) {
        Serial.println("[BT] Error: Bluetooth Init Failed!");
    } else {
        Serial.println("[BT] Ready. Pair with 'MotorTestStand'.");
    }
}

// Wysyła nagłówek kolumn (wywoływać raz przy starcie testu)
void bt_SendHeader() {
    if (SerialBT.hasClient()) {
        SerialBT.println("Time_ms,Step,Avg_PWM_us,Avg_Thrust_g,Avg_Volt_V,Avg_Curr_A,Avg_Pwr_W,Avg_Temp_C");
    }
}

// Wysyła wiersz danych (cyklicznie)
// Funkcja wywoływana w loop() ~50 razy na sekundę.
// Akumuluje dane i wysyła średnią co 1 sekundę.
void bt_SendTelemetry(SystemData* data) {
    // 1. Sprawdź czy ktoś nas słucha (oszczędność czasu CPU)
    if (!SerialBT.hasClient()) {
        // Reset buforów, żeby nie przepełnić zmiennych jak nikt nie słucha
        avg_sample_count = 0;
        sum_thrust = 0;
        sum_voltage = 0;
        sum_current = 0;
        sum_power = 0;
        sum_temp = 0;
        sum_pwm = 0;
        return;
    }

    // 2. Akumulacja danych (zbieramy każdą próbkę z loopa)
    sum_thrust  += data->thrust_g;
    sum_voltage += data->voltage_V;
    sum_current += data->current_A;
    sum_power   += data->power_W;
    sum_temp    += data->temp_C;
    sum_pwm     += data->current_PWM_us;
    avg_sample_count++;

    // 3. Sprawdzenie czasu raportu (1Hz)
    unsigned long now = millis();
    if (now - lastBtLogTime < BT_REPORT_INTERVAL_MS) {
        return; // Czekamy na pełną sekundę
    }
    lastBtLogTime = now;

    // 4. Obliczenie średnich
    if (avg_sample_count == 0) avg_sample_count = 1; // Safety divide by zero

    float avg_thrust  = sum_thrust / avg_sample_count;
    float avg_voltage = sum_voltage / avg_sample_count;
    float avg_current = sum_current / avg_sample_count;
    float avg_power   = sum_power / avg_sample_count;
    float avg_temp    = sum_temp / avg_sample_count;
    int   avg_pwm     = sum_pwm / avg_sample_count;

    // 5. Wysyłka CSV
    SerialBT.print(now);
    SerialBT.print(",");
    
    SerialBT.print(data->current_step); // Krok testu (nie uśredniamy, stan chwilowy)
    SerialBT.print(",");
    
    SerialBT.print(avg_pwm);
    SerialBT.print(",");
    
    SerialBT.print(avg_thrust, 1);
    SerialBT.print(",");
    
    SerialBT.print(avg_voltage, 2);
    SerialBT.print(",");
    
    SerialBT.print(avg_current, 2);
    SerialBT.print(",");
    
    SerialBT.print(avg_power, 1);
    SerialBT.print(",");
    
    SerialBT.println(avg_temp, 1);

    // 6. Reset akumulatorów na następną sekundę
    sum_thrust = 0;
    sum_voltage = 0;
    sum_current = 0;
    sum_power = 0;
    sum_temp = 0;
    sum_pwm = 0;
    avg_sample_count = 0;
}