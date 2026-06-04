// ESP32 - 3x HX711 + Wi-Fi + Supabase + calibración por Serial
// Copiá este archivo a tu sketch TACHOBASURA.ino en Arduino IDE
//
// Comandos Serial (115200 baud):
//   cal   cal1  cal2  cal3  = calibrar con peso conocido (160 g por defecto)
//   tara  = poner cero (tachos vacíos)
//   raw   = ver si el HX711 responde (debe cambiar al tocar la celda)
//   diag  = diagnóstico completo

#include <WiFi.h>
#include <HTTPClient.h>
#include <HX711.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

const char* ssid = "TU_WIFI_SSID";
const char* password = "TU_WIFI_PASSWORD";

const char* supabaseUrl = "https://fllgcqincjxqavpfmmbi.supabase.co";
const char* supabaseKey = "sb_publishable_CGbsdtCnVmNsLqq6d_tBbg_ksFSNIuF";
const char* tableName = "registros_residuos";

HX711 scale1, scale2, scale3;
const int DOUT_1 = 4, CLK_1 = 5;
const int DOUT_2 = 16, CLK_2 = 17;
const int DOUT_3 = 18, CLK_3 = 19;

const int ADDR_F1 = 0;
const int ADDR_F2 = 4;
const int ADDR_F3 = 8;
const uint32_t EEPROM_MAGIC = 0xC41101;

float scaleFactor1 = 420.0f;
float scaleFactor2 = 420.0f;
float scaleFactor3 = 420.0f;

const float PESO_MAX = 10.5f;
const float UMBRAL_CAMBIO = 0.03f;

// Peso conocido para calibrar (cambialo según lo que tengas)
// 160 g lata de atún → 160 gramos
const int PESO_CAL_GRAMOS = 160;
const float PESO_CAL_KG = PESO_CAL_GRAMOS / 1000.0f;  // 0.16 kg
const int MUESTRAS = 7;
const unsigned long INTERVALO_ENVIO = 1000;
const unsigned long INTERVALO_LECTURA = 500;

float pesoRef1 = 0, pesoRef2 = 0, pesoRef3 = 0;
unsigned long ultimoEnvio1 = 0, ultimoEnvio2 = 0, ultimoEnvio3 = 0;
unsigned long ultimaLectura = 0;

void guardarFactor(int addr, float factor) {
  EEPROM.put(addr, EEPROM_MAGIC);
  EEPROM.put(addr + 4, factor);
  EEPROM.commit();
}

float leerFactor(int addr, float defecto) {
  uint32_t magic = 0;
  float factor = defecto;
  EEPROM.get(addr, magic);
  EEPROM.get(addr + 4, factor);
  if (magic != EEPROM_MAGIC || factor <= 0 || factor > 500000 || isnan(factor)) {
    return defecto;
  }
  return factor;
}

bool celdaResponde(HX711& scale, const char* nombre) {
  if (!scale.wait_ready_timeout(3000)) {
    Serial.printf("  [ERROR] %s: sin respuesta (revisar DT/CLK/cables)\n", nombre);
    return false;
  }
  long r = scale.read();
  Serial.printf("  [OK] %s: lectura raw %ld\n", nombre, r);
  return true;
}

float leerPeso(HX711& scale, float factor, const char* nombre, bool verbose) {
  scale.set_scale(factor);

  float suma = 0;
  int validas = 0;

  for (int i = 0; i < MUESTRAS; i++) {
    if (scale.wait_ready_timeout(1000)) {
      float valor = scale.get_units(1);
      if (valor >= -0.3f && valor <= PESO_MAX + 1.0f) {
        suma += valor;
        validas++;
      }
    }
    delay(25);
  }

  if (validas < 2) {
    if (verbose) {
      Serial.printf("  [AVISO] %s: lectura fallida (factor=%.1f?)\n", nombre, factor);
    }
    return -1;
  }

  float promedio = suma / validas;
  if (promedio < 0) promedio = 0;
  if (promedio > PESO_MAX) promedio = PESO_MAX;
  return promedio;
}

void calibrarCelda(HX711& scale, int numero, int addrEeprom) {
  Serial.printf("\n===== CALIBRACION CELDA %d =====\n", numero);
  Serial.printf("Peso de referencia: %d g (%.2f kg)\n", PESO_CAL_GRAMOS, PESO_CAL_KG);
  Serial.println("1) Sacá TODO el peso de esa celda");
  Serial.println("2) Esperá 3 s...");
  delay(3000);

  scale.set_scale(1.0f);
  scale.tare(25);
  delay(800);

  Serial.printf("3) Poné la lata (%d g) SOLO en esta celda\n", PESO_CAL_GRAMOS);
  Serial.println("4) Escribí ENTER en el monitor...");
  while (!Serial.available()) delay(50);
  Serial.readStringUntil('\n');
  delay(1500);

  if (!scale.wait_ready_timeout(3000)) {
    Serial.println("ERROR: celda no responde");
    return;
  }

  long lectura = scale.get_value(30);
  if (lectura <= 0) {
    Serial.println("ERROR: lectura <= 0. Revisá montaje y cables.");
    return;
  }

  float factor = (float)lectura / PESO_CAL_KG;
  guardarFactor(addrEeprom, factor);
  scale.set_scale(factor);

  Serial.printf("Lectura con peso: %ld\n", lectura);
  Serial.printf("Factor guardado: %.2f (kg)\n", factor);
  Serial.printf("Prueba ahora: %.2f kg\n\n", scale.get_units(10));
  delay(2000);
}

void diagnosticoCompleto() {
  Serial.println("\n===== DIAGNOSTICO =====");
  celdaResponde(scale1, "Vidrio");
  celdaResponde(scale2, "Plastico");
  celdaResponde(scale3, "Papel");
  Serial.printf("Factores: %.2f | %.2f | %.2f\n", scaleFactor1, scaleFactor2, scaleFactor3);
  Serial.printf("Colocá el peso de calibracion (%d g) y mirá si 'raw' cambia.\n\n", PESO_CAL_GRAMOS);
}

void mostrarRaw() {
  Serial.println("\nRAW (10 lecturas, tocá la celda que quieras probar):\n");
  scale1.set_scale(1.0f);
  scale2.set_scale(1.0f);
  scale3.set_scale(1.0f);
  for (int i = 0; i < 10; i++) {
    long r1 = scale1.is_ready() ? scale1.read() : -999999;
    long r2 = scale2.is_ready() ? scale2.read() : -999999;
    long r3 = scale3.is_ready() ? scale3.read() : -999999;
    Serial.printf("C1:%7ld  C2:%7ld  C3:%7ld\n", r1, r2, r3);
    delay(400);
  }
  Serial.println("Si siempre -999999 o no cambia al poner peso → cableado/pines.\n");
}

void conectarWiFi() {
  Serial.printf("Wi-Fi: %s\n", ssid);
  WiFi.begin(ssid, password);
  for (int i = 0; i < 25 && WiFi.status() != WL_CONNECTED; i++) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("Wi-Fi OK - %s\n", WiFi.localIP().toString().c_str());
  }
}

bool enviarASupabase(const char* categoria, float pesoKg) {
  if (WiFi.status() != WL_CONNECTED) return false;

  HTTPClient http;
  String url = String(supabaseUrl) + "/rest/v1/" + tableName;

  StaticJsonDocument<256> doc;
  doc["categoria"] = categoria;
  doc["peso"] = roundf(pesoKg * 100) / 100.0f;
  doc["usuario"] = "esp32";

  String payload;
  serializeJson(doc, payload);

  http.begin(url);
  http.addHeader("apikey", supabaseKey);
  http.addHeader("Authorization", String("Bearer ") + supabaseKey);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Prefer", "return=minimal");

  int code = http.POST(payload);
  http.end();

  if (code == 201 || code == 200) {
    Serial.printf("[Supabase] %s %.2f kg\n", categoria, pesoKg);
    return true;
  }
  Serial.printf("[Supabase ERROR] %d\n", code);
  return false;
}

void procesarTacho(const char* cat, float pesoActual, float& pesoRef, unsigned long& ultimoEnvio) {
  if (pesoActual < 0) return;
  if (fabs(pesoActual - pesoRef) < UMBRAL_CAMBIO) return;
  if (millis() - ultimoEnvio < INTERVALO_ENVIO) return;
  if (enviarASupabase(cat, pesoActual)) {
    pesoRef = pesoActual;
    ultimoEnvio = millis();
  }
}

void mostrarMenu() {
  Serial.println("\nComandos: cal | cal1 cal2 cal3 | tara | raw | diag");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  EEPROM.begin(64);

  Serial.println("\n========================================");
  Serial.println("  TACHOS RECICLAJE - HX711");
  Serial.println("========================================\n");

  scale1.begin(DOUT_1, CLK_1);
  scale2.begin(DOUT_2, CLK_2);
  scale3.begin(DOUT_3, CLK_3);
  delay(500);

  scaleFactor1 = leerFactor(ADDR_F1, 420.0f);
  scaleFactor2 = leerFactor(ADDR_F2, 420.0f);
  scaleFactor3 = leerFactor(ADDR_F3, 420.0f);
  Serial.printf("Factores EEPROM: %.2f | %.2f | %.2f\n", scaleFactor1, scaleFactor2, scaleFactor3);

  Serial.println("Comprobando celdas...");
  bool ok1 = celdaResponde(scale1, "Vidrio");
  bool ok2 = celdaResponde(scale2, "Plastico");
  bool ok3 = celdaResponde(scale3, "Papel");

  if (!ok1 && !ok2 && !ok3) {
    Serial.println("\nNINGUNA CELDA RESPONDE. Revisá:");
    Serial.println("  - Pines DT/CLK");
    Serial.println("  - Alimentacion 5V HX711");
    Serial.println("  - Cables E+/E-/A+/A-");
  }

  Serial.println("\nTarando (tachos VACIOS)...");
  delay(2000);
  if (ok1) { scale1.set_scale(scaleFactor1); scale1.tare(15); }
  if (ok2) { scale2.set_scale(scaleFactor2); scale2.tare(15); }
  if (ok3) { scale3.set_scale(scaleFactor3); scale3.tare(15); }
  delay(500);

  conectarWiFi();

  pesoRef1 = leerPeso(scale1, scaleFactor1, "Vidrio", true);
  pesoRef2 = leerPeso(scale2, scaleFactor2, "Plastico", true);
  pesoRef3 = leerPeso(scale3, scaleFactor3, "Papel", true);
  if (pesoRef1 < 0) pesoRef1 = 0;
  if (pesoRef2 < 0) pesoRef2 = 0;
  if (pesoRef3 < 0) pesoRef3 = 0;

  Serial.println("\nListo.");
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
      calibrarCelda(scale1, 1, ADDR_F1);
      calibrarCelda(scale2, 2, ADDR_F2);
      calibrarCelda(scale3, 3, ADDR_F3);
      scaleFactor1 = leerFactor(ADDR_F1, scaleFactor1);
      scaleFactor2 = leerFactor(ADDR_F2, scaleFactor2);
      scaleFactor3 = leerFactor(ADDR_F3, scaleFactor3);
    } else if (cmd == "cal1") {
      calibrarCelda(scale1, 1, ADDR_F1);
      scaleFactor1 = leerFactor(ADDR_F1, scaleFactor1);
    } else if (cmd == "cal2") {
      calibrarCelda(scale2, 2, ADDR_F2);
      scaleFactor2 = leerFactor(ADDR_F2, scaleFactor2);
    } else if (cmd == "cal3") {
      calibrarCelda(scale3, 3, ADDR_F3);
      scaleFactor3 = leerFactor(ADDR_F3, scaleFactor3);
    } else if (cmd == "tara") {
      Serial.println("Tarando...");
      scale1.set_scale(scaleFactor1);
      scale2.set_scale(scaleFactor2);
      scale3.set_scale(scaleFactor3);
      scale1.tare(15);
      scale2.tare(15);
      scale3.tare(15);
      pesoRef1 = pesoRef2 = pesoRef3 = 0;
      Serial.println("OK");
    } else if (cmd == "raw") {
      mostrarRaw();
    } else if (cmd == "diag") {
      diagnosticoCompleto();
    } else {
      mostrarMenu();
    }
  }

  if (millis() - ultimaLectura >= INTERVALO_LECTURA) {
    float p1 = leerPeso(scale1, scaleFactor1, "Vidrio", false);
    float p2 = leerPeso(scale2, scaleFactor2, "Plastico", false);
    float p3 = leerPeso(scale3, scaleFactor3, "Papel", false);

    float v1 = p1 >= 0 ? p1 : pesoRef1;
    float v2 = p2 >= 0 ? p2 : pesoRef2;
    float v3 = p3 >= 0 ? p3 : pesoRef3;

    Serial.printf("Vidrio: %.2f | Plastico: %.2f | Papel: %.2f kg\n", v1, v2, v3);

    if (p1 >= 0) procesarTacho("vidrio", p1, pesoRef1, ultimoEnvio1);
    if (p2 >= 0) procesarTacho("plastico", p2, pesoRef2, ultimoEnvio2);
    if (p3 >= 0) procesarTacho("papel", p3, pesoRef3, ultimoEnvio3);

    ultimaLectura = millis();
  }

  delay(50);
}
