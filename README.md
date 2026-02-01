# MotorTestStand
F2838 Thruster Test Bench / Statyczny test ciągu F2838
# Motor Test Stand (ESP32)

System do automatycznego testowania charakterystyki napędów (Ciąg / Moc) oparty na mikrokontrolerze ESP32. Projekt skupia się na nieblokującym przetwarzaniu danych (FSM) i precyzyjnych pomiarach z częstotliwością 10Hz.

## Procedura Uruchomieniowa

### Faza 1: Smoke Test (Bez zasilania głównego LiPo)
**Cel:** Sprawdzenie żywotności ESP32, sensorów i wyświetlacza OLED.
1. Odłącz LiPo. Zostaw tylko USB podłączone do ESP32.
2. Wgraj kod i otwórz Serial Monitor (115200 baud).
3. **Analiza Logów:** Szukaj statusów `Ready` dla INA226, HX711 (sprawdź piny DT/SCK w razie błędu), DS18B20 oraz modułu BT.
4. **Weryfikacja OLED:** Powinien pokazać status `READY`. Parametry V, I, P bliskie 0, a Temp powinna odpowiadać pokojowej.
5. **Test Przycisków:** - `SELECT`: Zmiana trybu (CONST / RAMP).
   - `CONFIRM`: Zmiana statusu na `TEST_RUN`.

### Faza 2: Kalibracja Wagi (Tare & Scale)
**Cel:** Ustawienie zera i współczynnika dla tensometru.
1. **Zero:** Zamontuj ramię (bez obciążenia), zrestartuj ESP32 (funkcja `sensors_TareLoadCell()` w setup). OLED powinien wskazać `T: 0g` (+/- 2g).
2. **Skala:** Zwieś znany ciężar (np. 1000g) w miejscu montażu silnika.
3. **Korekta:** Jeśli odczyt jest błędny, skoryguj `LOADCELL_CALIBRATION_FACTOR` w `config.h` wg wzoru:
   `Nowy_Factor = Stary_Factor * (Odczyt_OLED / Rzeczywista_Masa)`

### Faza 3: Test Napędu (Bez Śruby!)
**Cel:** Sprawdzenie sterowania ESC i kierunku obrotów.
**UWAGA:** Silnik bez obciążenia grzeje się błyskawicznie. Testuj max 2-3 sekundy!
1. Podłącz LiPo (OLED powinien wskazać ok. 22-25V dla 6S).
2. Wciśnij `CONFIRM` – ESC powinien wygenerować sygnał uzbrojenia (Arming).
3. Jeśli silnik milczy, sprawdź `ESC_ARM_PWM` (1000us) w `config.h`.
4. Sprawdź, czy kierunek obrotów jest prawidłowy.

### Faza 4: Test Wodny (Full Load)
**Cel:** Pełny pomiar charakterystyki.
1. Zamontuj śrubę i zanurz układ (antena ESP musi być nad wodą!).
2. Połącz się przez "Serial Bluetooth Terminal" w telefonie.
3. Uruchom test przyciskiem `CONFIRM`.
4. Dane CSV spływające na telefon skopiuj po teście do Excela.
