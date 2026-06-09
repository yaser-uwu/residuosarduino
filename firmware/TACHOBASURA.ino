// ESP32 - 3x HX711 + LCD I2C + Supabase (mismos gramos que el LCD en la web)
//
// Comandos Serial (115200): cal cal1 cal2 cal3 | tara | raw | sync
//
// La web muestra lo último que el ESP envía a Supabase (kg, 3 decimales).

#include <WiFi.h>
#include <HTTPClient.h>
#include <HX711.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>


const char* ssid = "TU_WIFI_SSID";
const char* password = "TU_WIFI_PASSWORD";

const char* supabaseUrl = "https://fllgcqincjxqavpfmmbi.supabase.co";
const char* supabaseKey = "sb_publishable_CGbsdtCnVmNsLqq6d_tBbg_ksFSNIuF";
const char* tableName = "registros_residuos";

LiquidCrystal_I2C* lcd = nullptr;

HX711 scale1, scale2, scale3;
const int DOUT_1 = 4, CLK_1 = 5;
const int DOUT_2 = 16, CLK_2 = 17;
const int DOUT_3 = 18, CLK_3 = 19;
const int I2C_SDA = 21;
const int I2C_SCL = 22;

const int ADDR_FACTOR_1 = 0;
const int ADDR_FACTOR_2 = 8;
const int ADDR_FACTOR_3 = 16;

float scaleFactor1 = 276.29f;
float scaleFactor2 = 276.29f;
float scaleFactor3 = 276.29f;

const float PESO_MAX_G = 10500.0f;
const float UMBRAL_CAMBIO_G = 2.0f;            // 2 g → reacción rápida
const int MUESTRAS_RAPIDAS = 2;                // loop: pocas muestras = LCD ágil
const unsigned long INTERVALO_HEARTBEAT_MS = 3000;
const unsigned long INTERVALO_LECTURA_MS = 80;
const uint16_t HTTP_TIMEOUT_MS = 4000;
const unsigned long INTERVALO_LCD_MS = 50;
const unsigned long MIN_MS_ENTRE_ENVIOS = 120; // anti-spam HTTP

int peso_calibracion = 160;

float pesoRef1 = 0, pesoRef2 = 0, pesoRef3 = 0;
float pesoPantalla1 = 0, pesoPantalla2 = 0, pesoPantalla3 = 0;
unsigned long ultimoEnvioLote = 0;
unsigned long ultimaLectura = 0;
unsigned long ultimaActualizacionLCD = 0;

bool lcdOK = false;

bool iniciarLCD() {
  Wire.begin(I2C_SDA, I2C_SCL);
  delay(200);

  Serial.println("Buscando LCD...");
  const uint8_t direcciones[] = {0x27, 0x3F, 0x26, 0x20};

  for (uint8_t addr : direcciones) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf("LCD en 0x%02X\n", addr);
      lcd = new LiquidCrystal_I2C(addr, 20, 4);
      lcd->init();
      lcd->backlight();
      lcd->print("Tachos listos");
      delay(1500);
      lcd->clear();
      lcdOK = true;
      return true;
    }
  }

  Serial.println("LCD no encontrado (sigue sin pantalla)");
  lcdOK = false;
  return false;
}

void lcdLinea(int fila, const char* texto) {
  if (!lcdOK || !lcd) return;
  lcd->setCursor(0, fila);
  lcd->print(texto);
  int len = strlen(texto);
  for (int i = len; i < 20; i++) lcd->print(' ');
}

void mostrarLCD() {
  if (!lcdOK || !lcd) return;
  if (millis() - ultimaActualizacionLCD < INTERVALO_LCD_MS) return;

  float v = pesoPantalla1 / 1000.0f;
  float p = pesoPantalla2 / 1000.0f;
  float pa = pesoPantalla3 / 1000.0f;
  float total = v + p + pa;

  char linea[21];
  snprintf(linea, sizeof(linea), "Vidrio  : %6.3f kg", v);
  lcdLinea(0, linea);
  snprintf(linea, sizeof(linea), "Plastico: %6.3f kg", p);
  lcdLinea(1, linea);
  snprintf(linea, sizeof(linea), "Papel   : %6.3f kg", pa);
  lcdLinea(2, linea);
  snprintf(linea, sizeof(linea), "Total   : %6.3f kg", total);
  lcdLinea(3, linea);

  ultimaActualizacionLCD = millis();
}

void calibrarCelda(HX711& scale, int numeroCelda, int addr) {
  Serial.printf("\n========== CALIBRACION CELDA %d ==========\n", numeroCelda);
  Serial.println("Retira el peso de la celda");
  delay(2000);

  scale.set_scale(1.0);
  scale.tare(20);
  delay(500);

  Serial.printf("COLOCA %d GRAMOS EN LA CELDA\n", peso_calibracion);
  Serial.println("Presiona ENTER\n");
  while (!Serial.available()) delay(100);
  Serial.readStringUntil('\n');
  delay(1000);

  long adc_lecture = scale.get_value(100);
  float escala = (float)adc_lecture / (float)peso_calibracion;

  Serial.printf("RAW: %ld, Factor: %.6f\n", adc_lecture, escala);

  EEPROM.put(addr, escala);
  EEPROM.commit();

  delay(2000);
}

float limitarGramos(float g) {
  if (g < 0) return 0;
  if (g > PESO_MAX_G) return PESO_MAX_G;
  return g;
}

float leerPesoRapido(HX711& scale, float factor) {
  scale.set_scale(factor);

  float suma = 0;
  int validas = 0;

  for (int i = 0; i < MUESTRAS_RAPIDAS; i++) {
    if (scale.wait_ready_timeout(100)) {
      suma += scale.get_units(1);
      validas++;
    }
  }

  if (validas < 1) return -1;
  return limitarGramos(suma / validas);
}

float leerPesoEstable(HX711& scale, float factor) {
  scale.set_scale(factor);

  float suma = 0;
  int validas = 0;

  for (int i = 0; i < 6; i++) {
    if (scale.wait_ready_timeout(200)) {
      suma += scale.get_units(1);
      validas++;
    }
    delay(8);
  }

  if (validas < 2) return -1;
  return limitarGramos(suma / validas);
}

void conectarWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.printf("Wi-Fi: %s\n", ssid);
  if (lcdOK) {
    lcd->clear();
    lcd->print("Wi-Fi...");
  }

  WiFi.begin(ssid, password);
  for (int i = 0; i < 25 && WiFi.status() != WL_CONNECTED; i++) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("Wi-Fi OK %s\n", WiFi.localIP().toString().c_str());
    if (lcdOK) {
      lcd->clear();
      lcd->print("Wi-Fi OK");
      delay(800);
      lcd->clear();
    }
  } else {
    Serial.println("Wi-Fi FALLO");
  }
}

float gramosAKg(float pesoGramos) {
  return roundf((pesoGramos / 1000.0f) * 1000.0f) / 1000.0f;
}

void agregarFilaSupabase(JsonArray& arr, const char* categoria, float pesoGramos) {
  JsonObject row = arr.createNestedObject();
  row["categoria"] = categoria;
  row["peso"] = gramosAKg(pesoGramos);
  row["usuario"] = "esp32";
}

bool enviarUnoASupabase(const char* categoria, float pesoGramos) {
  if (WiFi.status() != WL_CONNECTED) return false;

  StaticJsonDocument<256> doc;
  doc["categoria"] = categoria;
  doc["peso"] = gramosAKg(pesoGramos);
  doc["usuario"] = "esp32";

  String payload;
  serializeJson(doc, payload);

  HTTPClient http;
  String url = String(supabaseUrl) + "/rest/v1/" + tableName;

  http.begin(url);
  http.setTimeout(HTTP_TIMEOUT_MS);
  http.addHeader("apikey", supabaseKey);
  http.addHeader("Authorization", String("Bearer ") + supabaseKey);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Prefer", "return=minimal");

  int code = http.POST(payload);
  http.end();

  if (code == 201 || code == 200) {
    Serial.printf("[Supabase OK] %s %.0f g\n", categoria, pesoGramos);
    return true;
  }

  Serial.printf("[Supabase ERROR] %s HTTP %d\n", categoria, code);
  return false;
}

bool enviarLoteASupabase(float p1, float p2, float p3, bool ok1, bool ok2, bool ok3) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[Supabase] sin Wi-Fi");
    return false;
  }
  if (!ok1 && !ok2 && !ok3) return false;

  StaticJsonDocument<768> doc;
  JsonArray arr = doc.to<JsonArray>();
  if (ok1) agregarFilaSupabase(arr, "vidrio", p1);
  if (ok2) agregarFilaSupabase(arr, "plastico", p2);
  if (ok3) agregarFilaSupabase(arr, "papel", p3);

  String payload;
  serializeJson(arr, payload);

  HTTPClient http;
  String url = String(supabaseUrl) + "/rest/v1/" + tableName;

  http.begin(url);
  http.setTimeout(HTTP_TIMEOUT_MS);
  http.addHeader("apikey", supabaseKey);
  http.addHeader("Authorization", String("Bearer ") + supabaseKey);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Prefer", "return=minimal");

  int code = http.POST(payload);
  String cuerpo = http.getString();
  http.end();

  if (code == 201 || code == 200) {
    Serial.printf("[Supabase OK] V:%.0f P:%.0f Pa:%.0f g\n",
                  ok1 ? p1 : 0, ok2 ? p2 : 0, ok3 ? p3 : 0);
    return true;
  }

  Serial.printf("[Supabase ERROR] HTTP %d\n", code);
  if (cuerpo.length() > 0) Serial.println(cuerpo);
  return false;
}

void procesarEnvio(float p1, float p2, float p3) {
  bool ok1 = p1 >= 0;
  bool ok2 = p2 >= 0;
  bool ok3 = p3 >= 0;
  bool huboCambio = false;

  if (millis() - ultimoEnvioLote >= MIN_MS_ENTRE_ENVIOS) {
    if (ok1 && fabs(p1 - pesoRef1) >= UMBRAL_CAMBIO_G) {
      if (enviarUnoASupabase("vidrio", p1)) {
        pesoRef1 = p1;
        ultimoEnvioLote = millis();
        huboCambio = true;
      }
    }
    if (ok2 && fabs(p2 - pesoRef2) >= UMBRAL_CAMBIO_G) {
      if (enviarUnoASupabase("plastico", p2)) {
        pesoRef2 = p2;
        ultimoEnvioLote = millis();
        huboCambio = true;
      }
    }
    if (ok3 && fabs(p3 - pesoRef3) >= UMBRAL_CAMBIO_G) {
      if (enviarUnoASupabase("papel", p3)) {
        pesoRef3 = p3;
        ultimoEnvioLote = millis();
        huboCambio = true;
      }
    }
  }

  if (!huboCambio && (millis() - ultimoEnvioLote) >= INTERVALO_HEARTBEAT_MS) {
    if (enviarLoteASupabase(p1, p2, p3, ok1, ok2, ok3)) {
      ultimoEnvioLote = millis();
      if (ok1) pesoRef1 = p1;
      if (ok2) pesoRef2 = p2;
      if (ok3) pesoRef3 = p3;
    }
  }
}

void syncTodosLosTachos() {
  float p1 = leerPesoEstable(scale1, scaleFactor1);
  float p2 = leerPesoEstable(scale2, scaleFactor2);
  float p3 = leerPesoEstable(scale3, scaleFactor3);

  bool ok1 = p1 >= 0;
  bool ok2 = p2 >= 0;
  bool ok3 = p3 >= 0;

  if (enviarLoteASupabase(p1, p2, p3, ok1, ok2, ok3)) {
    ultimoEnvioLote = millis();
    if (ok1) pesoRef1 = p1;
    if (ok2) pesoRef2 = p2;
    if (ok3) pesoRef3 = p3;
  }
}

void mostrarMenu() {
  Serial.println("\n========== MENU ==========");
  Serial.println("cal / cal1 / cal2 / cal3");
  Serial.println("tara | raw | sync");
  Serial.println("==========================\n");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  EEPROM.begin(512);

  Serial.println("\n========================================");
  Serial.println("  TACHOS RECICLAJE - LCD + Supabase");
  Serial.println("========================================\n");

  lcdOK = iniciarLCD();

  scale1.begin(DOUT_1, CLK_1);
  scale2.begin(DOUT_2, CLK_2);
  scale3.begin(DOUT_3, CLK_3);
  delay(500);

  EEPROM.get(ADDR_FACTOR_1, scaleFactor1);
  EEPROM.get(ADDR_FACTOR_2, scaleFactor2);
  EEPROM.get(ADDR_FACTOR_3, scaleFactor3);

  if (isnan(scaleFactor1) || scaleFactor1 <= 0) scaleFactor1 = 276.29f;
  if (isnan(scaleFactor2) || scaleFactor2 <= 0) scaleFactor2 = 276.29f;
  if (isnan(scaleFactor3) || scaleFactor3 <= 0) scaleFactor3 = 276.29f;

  Serial.printf("Factores: %.2f, %.2f, %.2f\n", scaleFactor1, scaleFactor2, scaleFactor3);

  scale1.set_scale(scaleFactor1);
  scale2.set_scale(scaleFactor2);
  scale3.set_scale(scaleFactor3);

  if (scale1.wait_ready_timeout(2000)) scale1.tare(10);
  if (scale2.wait_ready_timeout(2000)) scale2.tare(10);
  if (scale3.wait_ready_timeout(2000)) scale3.tare(10);

  delay(500);
  conectarWiFi();

  pesoPantalla1 = pesoRef1 = leerPesoEstable(scale1, scaleFactor1);
  pesoPantalla2 = pesoRef2 = leerPesoEstable(scale2, scaleFactor2);
  pesoPantalla3 = pesoRef3 = leerPesoEstable(scale3, scaleFactor3);
  if (pesoRef1 < 0) pesoRef1 = 0;
  if (pesoRef2 < 0) pesoRef2 = 0;
  if (pesoRef3 < 0) pesoRef3 = 0;

  Serial.println("Sync inicial Supabase...");
  syncTodosLosTachos();

  mostrarMenu();
  ultimaLectura = millis();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) conectarWiFi();

  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toLowerCase();

    if (cmd == "cal") {
      calibrarCelda(scale1, 1, ADDR_FACTOR_1);
      calibrarCelda(scale2, 2, ADDR_FACTOR_2);
      calibrarCelda(scale3, 3, ADDR_FACTOR_3);
      EEPROM.get(ADDR_FACTOR_1, scaleFactor1);
      EEPROM.get(ADDR_FACTOR_2, scaleFactor2);
      EEPROM.get(ADDR_FACTOR_3, scaleFactor3);
    } else if (cmd == "cal1") {
      calibrarCelda(scale1, 1, ADDR_FACTOR_1);
      EEPROM.get(ADDR_FACTOR_1, scaleFactor1);
    } else if (cmd == "cal2") {
      calibrarCelda(scale2, 2, ADDR_FACTOR_2);
      EEPROM.get(ADDR_FACTOR_2, scaleFactor2);
    } else if (cmd == "cal3") {
      calibrarCelda(scale3, 3, ADDR_FACTOR_3);
      EEPROM.get(ADDR_FACTOR_3, scaleFactor3);
    } else if (cmd == "tara") {
      scale1.tare(10);
      scale2.tare(10);
      scale3.tare(10);
      pesoRef1 = pesoRef2 = pesoRef3 = 0;
      syncTodosLosTachos();
    } else if (cmd == "raw") {
      for (int i = 0; i < 10; i++) {
        Serial.printf("C1:%ld C2:%ld C3:%ld\n", scale1.read(), scale2.read(), scale3.read());
        delay(300);
      }
    } else if (cmd == "sync") {
      syncTodosLosTachos();
    } else {
      mostrarMenu();
    }
  }

  if (millis() - ultimaLectura >= INTERVALO_LECTURA_MS) {
    float p1 = leerPesoRapido(scale1, scaleFactor1);
    float p2 = leerPesoRapido(scale2, scaleFactor2);
    float p3 = leerPesoRapido(scale3, scaleFactor3);

    bool refrescarLCD = false;

    if (p1 >= 0) {
      if (fabs(p1 - pesoPantalla1) >= 1.0f) refrescarLCD = true;
      pesoPantalla1 = p1;
    }
    if (p2 >= 0) {
      if (fabs(p2 - pesoPantalla2) >= 1.0f) refrescarLCD = true;
      pesoPantalla2 = p2;
    }
    if (p3 >= 0) {
      if (fabs(p3 - pesoPantalla3) >= 1.0f) refrescarLCD = true;
      pesoPantalla3 = p3;
    }

    if (refrescarLCD) ultimaActualizacionLCD = 0;
    mostrarLCD();

    procesarEnvio(p1, p2, p3);

    Serial.printf("V:%.0f P:%.0f Pa:%.0f g\n", pesoPantalla1, pesoPantalla2, pesoPantalla3);
    ultimaLectura = millis();
  } else {
    mostrarLCD();
  }

  delay(5);
}
