// ESP32 - 3x HX711 (10 kg) → Supabase + LCD I2C 20x4
// Librerías: HX711, ArduinoJson, WiFi, LiquidCrystal I2C
//
// LCD no muestra nada → mirá Serial Monitor: busca "LCD en 0x.."
// Si no encuentra LCD, probá otro par SDA/SCL (abajo) o girá el potenciómetro azul del módulo I2C.

#include <WiFi.h>
#include <HTTPClient.h>
#include <HX711.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const char* ssid = "TU_WIFI_SSID";
const char* password = "TU_WIFI_PASSWORD";

const char* supabaseUrl = "https://fllgcqincjxqavpfmmbi.supabase.co";
const char* supabaseKey = "sb_publishable_CGbsdtCnVmNsLqq6d_tBbg_ksFSNIuF";
const char* tableName = "registros_residuos";

// === LCD I2C ===
// ESP32 habitual: SDA=21, SCL=22. Si no hay imagen, probá SDA=4 y SCL=15
#define I2C_SDA 21
#define I2C_SCL 22
#define LCD_COLS 20
#define LCD_ROWS 4

LiquidCrystal_I2C* lcd = nullptr;
bool lcdOk = false;

HX711 scale1, scale2, scale3;
const int DOUT_1 = 4, CLK_1 = 5;
const int DOUT_2 = 16, CLK_2 = 17;
const int DOUT_3 = 18, CLK_3 = 19;

float scaleFactor1 = 420.0f;
float scaleFactor2 = 420.0f;
float scaleFactor3 = 420.0f;

const float PESO_MAX_CELDA = 10.5f;
const float UMBRAL_CAMBIO_KG = 0.03f;     // 30 g
const int MUESTRAS_LECTURA = 7;
const unsigned long INTERVALO_ENVIO_MS = 800;
const unsigned long INTERVALO_LOOP_MS = 500;
const unsigned long INTERVALO_LCD_MS = 250;

float pesoRef1 = 0, pesoRef2 = 0, pesoRef3 = 0;
float pesoPantalla1 = 0, pesoPantalla2 = 0, pesoPantalla3 = 0;
unsigned long ultimoEnvio1 = 0, ultimoEnvio2 = 0, ultimoEnvio3 = 0;
unsigned long ultimoLcd = 0;

uint8_t buscarLCD() {
  const uint8_t preferidos[] = {0x27, 0x3F, 0x26, 0x20, 0x38};
  uint8_t primero = 0;

  Wire.begin(I2C_SDA, I2C_SCL);
  delay(200);

  Serial.println("Escaneo I2C (SDA/SCL):");
  Serial.printf("  SDA=%d SCL=%d\n", I2C_SDA, I2C_SCL);

  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf("  Encontrado: 0x%02X\n", addr);
      if (primero == 0) primero = addr;
      for (uint8_t p : preferidos) {
        if (addr == p) return addr;
      }
    }
  }
  return primero;
}

void lcdEncender() {
  if (!lcd) return;
  lcd->init();
  lcd->backlight();
  lcd->display();
  lcd->clear();
}

bool iniciarLCD() {
  uint8_t addr = buscarLCD();
  if (addr == 0) {
    Serial.println("ERROR: ningun dispositivo I2C. Revisá cables SDA/SCL y 5V.");
    lcdOk = false;
    return false;
  }

  Serial.printf("Iniciando LCD en 0x%02X\n", addr);
  lcd = new LiquidCrystal_I2C(addr, LCD_COLS, LCD_ROWS);
  lcdEncender();

  lcd->setCursor(0, 0);
  lcd->print("Tachos reciclaje");
  lcd->setCursor(0, 1);
  lcd->print("LCD OK 0x");
  lcd->print(addr, HEX);
  delay(1200);
  lcd->clear();

  lcdOk = true;
  return true;
}

void lcdLinea(int fila, const char* texto) {
  if (!lcdOk || !lcd) return;
  lcd->setCursor(0, fila);
  lcd->print(texto);
  int len = strlen(texto);
  for (int i = len; i < LCD_COLS; i++) lcd->print(' ');
}

void mostrarLCD(float v, float p, float pa) {
  if (!lcdOk || !lcd) return;

  float total = v + p + pa;
  if (total < 0) total = 0;

  char linea[21];
  snprintf(linea, sizeof(linea), "Vidrio  : %5.2f kg", v);
  lcdLinea(0, linea);

  snprintf(linea, sizeof(linea), "Plastico: %5.2f kg", p);
  lcdLinea(1, linea);

  snprintf(linea, sizeof(linea), "Papel   : %5.2f kg", pa);
  lcdLinea(2, linea);

  snprintf(linea, sizeof(linea), "Total   : %5.2f kg", total);
  lcdLinea(3, linea);
}

float limitarPeso(float kg) {
  if (kg < 0) return 0;
  if (kg > PESO_MAX_CELDA) return PESO_MAX_CELDA;
  return kg;
}

// Lectura simple: promedio de varias muestras (sin filtro que bloquee todo)
float leerPeso(HX711& scale, float factor) {
  scale.set_scale(factor);

  float suma = 0;
  int validas = 0;

  for (int i = 0; i < MUESTRAS_LECTURA; i++) {
    if (scale.wait_ready_timeout(800)) {
      float v = scale.get_units(1);
      if (v >= -0.2f && v <= PESO_MAX_CELDA + 2.0f) {
        suma += v;
        validas++;
      }
    }
    delay(20);
  }

  if (validas < 2) {
    return -1;
  }

  return limitarPeso(suma / validas);
}

void conectarWiFi() {
  lcdLinea(2, "Wi-Fi conectando...");
  WiFi.begin(ssid, password);

  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 25) {
    delay(500);
    intentos++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    lcdLinea(2, "Wi-Fi OK            ");
    Serial.printf("Wi-Fi OK IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    lcdLinea(2, "Wi-Fi sin conexion  ");
  }
}

// Envía el peso ACTUAL del tacho (igual que muestra el LCD), no el delta
bool enviarASupabase(const char* categoria, float pesoActualKg) {
  if (WiFi.status() != WL_CONNECTED) return false;

  HTTPClient http;
  String url = String(supabaseUrl) + "/rest/v1/" + tableName;

  StaticJsonDocument<256> doc;
  doc["categoria"] = categoria;
  doc["peso"] = roundf(pesoActualKg * 100) / 100.0f;
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
    Serial.printf("OK %s %.2f kg (actual)\n", categoria, pesoActualKg);
    return true;
  }
  Serial.printf("Error HTTP %d: %s\n", code, http.getString().c_str());
  return false;
}

void procesarTacho(const char* categoria, float pesoActual, float& pesoRef,
                   unsigned long& ultimoEnvio) {
  if (pesoActual < 0) return;

  float delta = pesoActual - pesoRef;
  if (fabs(delta) < UMBRAL_CAMBIO_KG) return;
  if (millis() - ultimoEnvio < INTERVALO_ENVIO_MS) return;

  if (enviarASupabase(categoria, pesoActual)) {
    pesoRef = pesoActual;
    ultimoEnvio = millis();
  }
}

void setup() {
  Serial.begin(115200);
  delay(800);

  iniciarLCD();

  if (!lcdOk) {
    Serial.println("SIN LCD - el programa sigue, pero sin pantalla.");
  }

  scale1.begin(DOUT_1, CLK_1);
  scale2.begin(DOUT_2, CLK_2);
  scale3.begin(DOUT_3, CLK_3);
  scale1.set_scale(scaleFactor1);
  scale2.set_scale(scaleFactor2);
  scale3.set_scale(scaleFactor3);

  lcdLinea(0, "Tarando balanzas...");
  delay(500);
  if (scale1.wait_ready_timeout(2000)) scale1.tare(10);
  if (scale2.wait_ready_timeout(2000)) scale2.tare(10);
  if (scale3.wait_ready_timeout(2000)) scale3.tare(10);
  delay(500);
  Serial.println("Tara lista. Poné peso y mirá Serial.");

  conectarWiFi();

  pesoRef1 = pesoPantalla1 = leerPeso(scale1, scaleFactor1);
  pesoRef2 = pesoPantalla2 = leerPeso(scale2, scaleFactor2);
  pesoRef3 = pesoPantalla3 = leerPeso(scale3, scaleFactor3);
  if (pesoRef1 < 0) pesoRef1 = pesoPantalla1 = 0;
  if (pesoRef2 < 0) pesoRef2 = pesoPantalla2 = 0;
  if (pesoRef3 < 0) pesoRef3 = pesoPantalla3 = 0;

  mostrarLCD(pesoPantalla1, pesoPantalla2, pesoPantalla3);
  ultimoLcd = millis();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) conectarWiFi();

  float p1 = leerPeso(scale1, scaleFactor1);
  float p2 = leerPeso(scale2, scaleFactor2);
  float p3 = leerPeso(scale3, scaleFactor3);

  if (p1 >= 0) pesoPantalla1 = p1;
  else Serial.println("Aviso: lectura vidrio fallo");
  if (p2 >= 0) pesoPantalla2 = p2;
  else Serial.println("Aviso: lectura plastico fallo");
  if (p3 >= 0) pesoPantalla3 = p3;
  else Serial.println("Aviso: lectura papel fallo");

  if (lcdOk && millis() - ultimoLcd >= INTERVALO_LCD_MS) {
    mostrarLCD(pesoPantalla1, pesoPantalla2, pesoPantalla3);
    ultimoLcd = millis();
  }

  Serial.printf("V:%.2f P:%.2f Pa:%.2f kg\n", pesoPantalla1, pesoPantalla2, pesoPantalla3);

  if (p1 >= 0) procesarTacho("vidrio", p1, pesoRef1, ultimoEnvio1);
  if (p2 >= 0) procesarTacho("plastico", p2, pesoRef2, ultimoEnvio2);
  if (p3 >= 0) procesarTacho("papel", p3, pesoRef3, ultimoEnvio3);

  delay(INTERVALO_LOOP_MS);
}
