# Specyfikacja Techniczna i Architektura

## Hardware & Modyfikacje
System wykorzystuje ESP32 Wroom, INA226 (Prąd), HX711 (Waga), DS18B20 (Temp) oraz ESC (LittleBee/Flycolor).

### Wymagane modyfikacje HW (dla próbkowania 10Hz):
* **HX711:** Przelutowanie zworki (pin 15 na VCC) dla trybu **80Hz** (redukcja opóźnienia z 400ms do <100ms).
* **INA226:** Konfiguracja uśredniania na **16-64 próbki** (~8.2ms) dla uniknięcia aliasingu.
* **Panic Button:** Fizyczny wyłącznik zasilania (nadrzędny nad kodem).

## Struktura Oprogramowania
Projekt oparty na maszynie stanów (FSM) i nieblokującym I/O (brak `delay()`).

| Plik | Rola |
| :--- | :--- |
| `main.ino` | Koordynator; inicjalizacja i główna pętla `loop()`. |
| `config.h` | Pinout, stałe czasowe, limity i makra kalibracyjne. |
| `types.h` | Definicje struktur (np. `SystemData`) i stanów FSM. |
| `logic_FSM.ino` | Maszyna stanów High-Level (BOOT -> READY -> TEST_RUN). |
| `logic_SQN.ino` | Logika sekwencji testowych (RAMP / STABILIZE / MEASURE). |
| `drv_Sensors.ino` | Asynchroniczna obsługa czujników (Latest Known Value). |
| `drv_ESC.ino` | Sterowanie silnikiem (PWM/Dshot). |
| `hmi_OLED.ino` | Wizualizacja danych pomiarowych. |
| `hmi_BT.ino` | Transmisja danych CSV przez Bluetooth. |

## Algorytmy Testowe
1.  **Mapa Silnika (Ramp):** Stopniowe zwiększanie PWM co 100 jednostek, stabilizacja (`TIME_STAB` = 6s) i pomiar (`TIME_MEASURE` = 4s).
2.  **Stały Ciąg (Target Thrust):** Dynamiczne szukanie PWM dla osiągnięcia zadanej wagi (np. 400g), a następnie pomiar stabilności ciągu.

## Wytyczne Kodowania
* Całkowity zakaz używania `delay()`.
* Zmienne konfiguracyjne wyłącznie w `config.h`.
* Bezpieczeństwo: Każde wyjście z testu musi wymusić `esc_SetPWM(1000)`.
