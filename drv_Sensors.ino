#include <Wire.h>
#include <INA226_WE.h>
#include <HX711.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include "config.h" // Arduino.h siedzi tutaj
#include "types.h"

// ==========================================
// OBIEKTY GLOBALNE
// ==========================================

INA226_WE ina226 = INA226_WE(0x40);
HX711 scale;

// Standardowa obsługa Dallasa (zgodna z Twoim działającym kodem)
OneWire oneWire(PIN_ONE_WIRE);
DallasTemperature tempSensors(&oneWire);

extern SystemData sysData; 

// ==========================================
// ZMIENNE POMOCNICZE DRIVERA
// ==========================================

// Kalibracja
long loadCellOffset = 0;
float filteredThrust = 0.0f;

// Obsługa asynchroniczna Dallasa
unsigned long lastTempRequestTime = 0;
bool isTempRequestPending = false;
const unsigned long TEMP_MEASURE_INTERVAL = 1000; 

// ==========================================
// FUNKCJE INIT
// ==========================================

void sensors_Init() {
    Serial.println("[SENS] Init Hardware...");

    // 1. I2C Init
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

    // 2. INA226 Init
    if (!ina226.init()) {
        Serial.println("[SENS] INA226 Connection Failed!");
    } else {
        ina226.setResistorRange(SHUNT_RESISTOR_OHM, INA_MAX_CURRENT);
        // POPRAWKA: Używamy stałych z prefiksem INA226_ dla biblioteki WE
        ina226.setAverage(INA226_AVERAGE_16); 
        ina226.setConversionTime(INA226_CONV_TIME_1100); 
        Serial.println("[SENS] INA226 Ready.");
    }

    // 3. HX711 Init
    scale.begin(PIN_HX711_DT, PIN_HX711_SCK);
    
    unsigned long timeout = millis();
    while (!scale.is_ready() && millis() - timeout < 500) {
        delay(10);
    }
    
    if (scale.is_ready()) {
        Serial.println("[SENS] HX711 Ready.");
        scale.set_scale(LOADCELL_CALIBRATION_FACTOR); 
        scale.tare(20); 
    } else {
        Serial.println("[SENS] HX711 Not Detected!");
    }

    // 4. DS18B20 Init (Standard DallasTemperature)
    tempSensors.begin();
    
    // KLUCZOWE: Wyłączamy czekanie na wynik w requestTemperatures().
    // Dzięki temu funkcja ta zajmie <1ms zamiast 750ms!
    tempSensors.setWaitForConversion(false);
    
    // Sprawdzenie czy cokolwiek jest na linii
    if (tempSensors.getDeviceCount() > 0) {
        Serial.print("[SENS] DS18B20 Found: ");
        Serial.println(tempSensors.getDeviceCount());
    } else {
        Serial.println("[SENS] No DS18B20 found!");
    }
}

// ==========================================
// FUNKCJE POMOCNICZE
// ==========================================

void sensors_TareLoadCell() {
    if (scale.is_ready()) {
        Serial.println("[SENS] Tarowanie wagi...");
        scale.tare(50); 
        loadCellOffset = scale.get_offset();
        filteredThrust = 0.0f; 
        Serial.println("[SENS] Waga wyzerowana.");
    } else {
        Serial.println("[SENS] Błąd: Waga niegotowa.");
    }
}

bool sensors_IsTempCritical() {
    if (sysData.temp_C >= MAX_TEMP_C) {
        return true; 
    }
    return false;
}

// ==========================================
// GŁÓWNA PĘTLA AKTUALIZACJI (LOOP)
// ==========================================

void sensors_UpdateAsync() {
    // 1. INA226
    ina226.readAndClearFlags();
    sysData.voltage_V = ina226.getBusVoltage_V();
    sysData.current_A = ina226.getCurrent_A();
    sysData.power_W   = ina226.getBusPower();

    // 2. HX711
    if (scale.is_ready()) {
        float reading = scale.get_units(1); 
        filteredThrust = (LOADCELL_ALPHA * reading) + ((1.0f - LOADCELL_ALPHA) * filteredThrust);
        sysData.thrust_raw = reading; 
        sysData.thrust_g = filteredThrust;
    }

    // 3. DS18B20 (Maszyna stanów)
    unsigned long now = millis();
    
    // KROK 1: Zleć pomiar (jeśli minął czas interwału)
    if (!isTempRequestPending && (now - lastTempRequestTime > TEMP_MEASURE_INTERVAL)) {
        tempSensors.requestTemperatures(); // To teraz jest szybkie (async)
        lastTempRequestTime = now;
        isTempRequestPending = true;
    }

    // KROK 2: Odbierz wynik (jeśli minął czas konwersji ~800ms)
    if (isTempRequestPending && (now - lastTempRequestTime > 800)) {
        float temp = tempSensors.getTempCByIndex(0);
        
        // Dallas zwraca -127.00 przy błędzie
        if (temp > -100.0f) {
            sysData.temp_C = temp;
        }
        
        isTempRequestPending = false; 
    }
    
    sysData.sys_time_ms = millis();
}