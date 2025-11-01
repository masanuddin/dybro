// --- Library Baru ---
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// --- Library Lama ---
// #include <Adafruit_SSD1306.h> // DIHAPUS
// #include <Adafruit_GFX.h> // DIHAPUS
#include <DHT.h>
#include <math.h>

// --- Definisi Pin (SAMA) ---
#define DHTPIN 4
#define DHTTYPE DHT11
#define SENSOR_PIN 34 
#define BUTTON_PIN 12
#define RED_PIN 27
#define GREEN_PIN 26
#define BLUE_PIN 25
#define BUZZER_PIN 23 

// --- Pengaturan Layar (DIHAPUS) ---
// #define SCREEN_WIDTH 128
// #define SCREEN_HEIGHT 64

// --- Pengaturan Pomodoro (SAMA) ---
const unsigned long sessionDuration = 25 * 60 * 1000; 
const unsigned long breakDuration = 5 * 60 * 1000;   
const unsigned long pauseAutoResume = 2 * 60 * 1000; 

// --- WiFi & MQTT (SAMA) ---
const char* ssid = "Nibba";
const char* password = "Yazidgei";
const char* mqtt_server = "broker.hivemq.com"; 
const int mqtt_port = 1883;


WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMqttReconnectAttempt = 0;

// --- TOPIK MQTT (SAMA) ---
#define MQTT_TOPIC_DHT "sic/dibimbing/DoaIbuMenyertai/dht"
#define MQTT_TOPIC_IR "sic/dibimbing/DoaIbuMenyertai/ir"
#define MQTT_TOPIC_BUTTON "sic/dibimbing/DoaIbuMenyertai/button"

// --- Inisialisasi Library (DIUBAH) ---
// Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1); // DIHAPUS
DHT dht(DHTPIN, DHTTYPE);

// --- Variabel State Machine (SAMA) ---
enum State { IDLE, SESSION, BREAK, PAUSED };
State currentState = IDLE;

// (Variabel state, timer, sensor SAMA)
unsigned long sessionStart = 0;
unsigned long lastSensorCalc = 0; 
unsigned long lastInputLog = 0;
float lastTemp = 0.0;
unsigned long pauseStartTime = 0;
byte buttonState = HIGH;
byte lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long buttonPressTime = 0; 
unsigned long lastClickTime = 0;   
bool buttonIsPressed = false;
bool longPressTriggered = false;
int clickCount = 0;

// --- PERBAIKAN TIMING TOMBOL ---
const long debounceDelay = 75;     // (Diubah dari 50)
const long multiClickDelay = 600;  // (Diubah dari 300)
const long longPressDelay = 1000;  
// ---------------------------------

bool isFokus = false;
float sensorNoise = 0.0;
#define SENSOR_SAMPLE_SIZE 20 
int sensorReadings[SENSOR_SAMPLE_SIZE];
int sensorIndex = 0;
unsigned long lastSensorReadTime = 0;
const int sensorReadInterval = 100;
#define NOISE_THRESHOLD 10.0 

// --- Fungsi Helper WiFi & MQTT (SAMA) ---

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

String getStatusString() {
  switch (currentState) {
    case IDLE: return "IDLE";
    case SESSION: return "SESSION";
    case BREAK: return "BREAK";
    case PAUSED: return "PAUSED";
    default: return "UNKNOWN";
  }
}

unsigned long getRemainingTime() {
  unsigned long elapsed = 0;
  unsigned long duration = 0;
  
  if (currentState == SESSION) {
    elapsed = millis() - sessionStart;
    duration = sessionDuration;
  } else if (currentState == BREAK) {
    elapsed = millis() - sessionStart;
    duration = breakDuration;
  } else if (currentState == PAUSED) {
    elapsed = pauseStartTime - sessionStart; 
    duration = sessionDuration;
  } else {
    return 0; 
  }
  
  if (elapsed >= duration) return 0;
  return (duration - elapsed) / 1000; // Sisa waktu dalam detik
}

// --- FUNGSI PUBLISH (SAMA) ---
void publishButtonState() {
  if (!client.connected()) return;
  
  StaticJsonDocument<128> doc;
  doc["state"] = getStatusString();
  doc["timeRemaining_s"] = getRemainingTime();
  
  char buffer[128];
  size_t n = serializeJson(doc, buffer);
  
  Serial.print("Publishing to MQTT Button: ");
  Serial.println(buffer);
  client.publish(MQTT_TOPIC_BUTTON, buffer, n);
}

void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "JendelaFokus-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      return; 
    }
  }
}

// --- Setup (DIUBAH) ---
void setup() {
  Serial.begin(115200); 
  Serial.println("\nBooting JendelaFokus (v6.0: Headless MQTT)..."); 

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  setColor(0, 0, 0);             
  dht.begin();
  Serial.println("Sensors and outputs OK.");

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  
  // Blok Inisialisasi OLED DIHAPUS
  // if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { ... }
  // Serial.println("OLED display OK."); // DIHAPUS

  Serial.print("Mengkalibrasi Sensor... Baca nilai awal: ");
  int initialSensorValue = analogRead(SENSOR_PIN);
  for (int i = 0; i < SENSOR_SAMPLE_SIZE; i++) {
    sensorReadings[i] = initialSensorValue;
  }
  Serial.println(initialSensorValue);

  // showMessage("JendelaFokus", "Tekan untuk Mulai"); // DIHAPUS
  Serial.println("Setup complete. Masuk mode IDLE.");
}

// --- Loop (SAMA) ---
void loop() {
  
  // PRIORITAS #1: BACA TOMBOL
  handleButton(); 
  checkButtonActions();

  // Prioritas #2: Jaga Koneksi MQTT
  if (!client.connected()) {
    if (millis() - lastMqttReconnectAttempt > 5000) {
      lastMqttReconnectAttempt = millis();
      reconnect_mqtt();
    }
  }
  client.loop(); 

  // Prioritas #3: Logika State Machine
  switch (currentState) {
    case IDLE:
      handleIdleState();
      break;
    case SESSION:
      handleSessionState();
      break;
    case BREAK:
      handleBreakState();
      break;
    case PAUSED:
      handlePausedState();
      break;
  }
  
  // Prioritas #4: Sensor & Logging
  
  if (millis() - lastSensorReadTime >= sensorReadInterval) {
    lastSensorReadTime = millis();
    sensorReadings[sensorIndex] = analogRead(SENSOR_PIN);
    sensorIndex++;
    if (sensorIndex >= SENSOR_SAMPLE_SIZE) {
      sensorIndex = 0;
    }
  }

  if (currentState == SESSION && (millis() - lastSensorCalc >= 2000)) {
    lastSensorCalc = millis();
    lastTemp = dht.readTemperature();
    sensorNoise = calculateStdDev(sensorReadings, SENSOR_SAMPLE_SIZE);
    isFokus = (sensorNoise > NOISE_THRESHOLD);
    
    Serial.print("--- SESI UPDATE --- | ");
    Serial.print("Sensor Noise (StdDev): "); Serial.print(sensorNoise);
    Serial.print(" | Fokus: "); Serial.print(isFokus ? "AKTIF" : "DIAM");
    Serial.print(" | Temp: "); Serial.println(lastTemp);

    if (client.connected()) {
      StaticJsonDocument<128> doc;
      doc["isFokus"] = isFokus;
      doc["noise"] = sensorNoise;
      char buffer[128];
      size_t n = serializeJson(doc, buffer);
      Serial.print("Publishing to MQTT IR: ");
      Serial.println(buffer);
      client.publish(MQTT_TOPIC_IR, buffer, n);
    }
  }

  if (millis() - lastInputLog >= 2000) {
    lastInputLog = millis();
    float logTemp = dht.readTemperature(); 
    int logButton = digitalRead(BUTTON_PIN);
    int logSensor = analogRead(SENSOR_PIN);

    Serial.print("[LOG] | ");
    Serial.print("State: "); Serial.print(getStatusString());
    Serial.print(" | Tombol: "); Serial.print(logButton == LOW ? "DITEKAN" : "LEPAS");
    Serial.print(" | Sensor (Raw): "); Serial.print(logSensor);
    Serial.print(" | Temp: "); Serial.println(logTemp);

    if (client.connected()) {
      StaticJsonDocument<64> doc;
      doc["temperature"] = logTemp; 
      char buffer[64];
      size_t n = serializeJson(doc, buffer);
      Serial.print("Publishing to MQTT DHT: ");
      Serial.println(buffer);
      client.publish(MQTT_TOPIC_DHT, buffer, n);
    }
  }
}

// --- Fungsi Button Handler (SAMA) ---
void handleButton() {
  int reading = digitalRead(BUTTON_PIN);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) { 
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) {
        digitalWrite(BUZZER_PIN, HIGH); 
        buttonIsPressed = true;
        longPressTriggered = false;
        buttonPressTime = millis();
      } else {
        digitalWrite(BUZZER_PIN, LOW); 
        buttonIsPressed = false;
        if (!longPressTriggered) {
          clickCount++;
          lastClickTime = millis(); 
        }
      }
    }
  }
  lastButtonState = reading;
}

// --- Fungsi Button Actions (SAMA) ---
void checkButtonActions() {
  if (buttonIsPressed && !longPressTriggered) {
    if (millis() - buttonPressTime > longPressDelay) {
      Serial.println("Aksi: Long Press Terdeteksi!");
      handleLongPress();
      longPressTriggered = true; 
      clickCount = 0;          
    }
  }
  if (clickCount > 0 && !buttonIsPressed && (millis() - lastClickTime > multiClickDelay)) { 
    if (clickCount == 1) {
      Serial.println("Aksi: Single Click Terdeteksi!");
      handleSingleClick();
    }
    if (clickCount == 2) {
      Serial.println("Aksi: Double Click Terdeteksi!");
      handleDoubleClick();
    }
    clickCount = 0;
  }
}

// --- Fungsi Aksi Tombol (DIUBAH) ---
void handleSingleClick() {
  if (currentState == IDLE) {
    Serial.println("Tombol DITEKAN. Memulai Sesi...");
    currentState = SESSION;
    sessionStart = millis();
    isFokus = true; 
    // showMessage("Sesi Dimulai", "25:00"); // DIHAPUS
    playStartSound(); 
    lastSensorCalc = millis();
    lastSensorReadTime = millis();
    publishButtonState(); 
  }
}

void handleDoubleClick() {
  if (currentState == SESSION) {
    currentState = PAUSED;
    pauseStartTime = millis(); 
    unsigned long timeElapsedSoFar = pauseStartTime - sessionStart;
    Serial.println("Sesi di-PAUSE.");
    // showMessage("DiJeda", formatTime(sessionDuration - timeElapsedSoFar)); // DIHAPUS
    setColor(255, 165, 0); // Oranye
    playNudgeSound();
    publishButtonState(); 
  } else if (currentState == PAUSED) {
    currentState = SESSION;
    unsigned long pauseDuration = millis() - pauseStartTime;
    sessionStart += pauseDuration;
    Serial.println("Sesi di-LANJUTKAN.");
    playStartSound();
    publishButtonState(); 
  }
}

void handleLongPress() {
  if (currentState == SESSION || currentState == BREAK || currentState == PAUSED) {
    Serial.println("Sesi di-STOP. Kembali ke IDLE.");
    currentState = IDLE;
    // showMessage("Dibatalkan", "Tekan untuk Mulai"); // DIHAPUS
    playEndSound();
    setColor(0, 0, 0);
    clickCount = 0; 
    publishButtonState(); 
  }
}

// --- Fungsi State Machine (DIUBAH) ---

void handleIdleState() {
  setColor(0, 0, 0); 
}

void handlePausedState() {
  setColor(255, 165, 0); // Oranye
  unsigned long timeSincePaused = millis() - pauseStartTime;

  if (timeSincePaused >= pauseAutoResume) {
    currentState = SESSION;
    sessionStart += timeSincePaused;
    Serial.println("Sesi di-LANJUTKAN (Otomatis dari Pause).");
    playStartSound();
    publishButtonState(); 
  } else {
    // Logika showMessage DIHAPUS
    delay(100); 
  }
}

void handleSessionState() {
  unsigned long elapsed = millis() - sessionStart;

  if (elapsed >= sessionDuration) {
    Serial.println("Sesi Selesai, Mulai Istirahat.");
    currentState = BREAK;
    sessionStart = millis(); 
    // showMessage("Istirahat!", "05:00"); // DIHAPUS
    setColor(0, 0, 255); 
    playEndSound(); 
    publishButtonState(); 
  } else {
    if (isFokus) { 
      setColor(0, 255, 0); 
      // Logika showMessage DIHAPUS
    } else {
      setColor(255, 0, 0); 
      // Logika showMessage DIHAPUS
      playNudgeSound();
    }
  }
}


void handleBreakState() {
  unsigned long elapsed = millis() - sessionStart;
  // showMessage("Istirahat", timeRemaining); // DIHAPUS
  
  if (elapsed >= breakDuration) {
    Serial.println("Istirahat Selesai.");
    currentState = IDLE;
    // showMessage("Selesai!", "Tekan untuk Mulai"); // DIHAPUS
    playEndSound();
    setColor(0, 0, 0);
    publishButtonState(); 
  }
  delay(100);
}

// --- Fungsi Helper (DIUBAH) ---

float calculateStdDev(int arr[], int n) {
  if (n == 0) return 0.0;
  float sum = 0.0, mean, stdDev = 0.0;
  for (int i = 0; i < n; i++) {
    sum += arr[i];
  }
  mean = sum / n;
  for (int i = 0; i < n; i++) {
    stdDev += pow(arr[i] - mean, 2);
  }
  return sqrt(stdDev / n);
}

void setColor(int r, int g, int b) {
  analogWrite(RED_PIN, r);
  analogWrite(GREEN_PIN, g);
  analogWrite(BLUE_PIN, b);
}

// Fungsi showMessage() DIHAPUS
/*
void showMessage(String line1, String line2) {
  display.clearDisplay();
  ...
}
*/

String formatTime(unsigned long ms) {
  unsigned long totalSeconds = ms / 1000;
  int minutes = totalSeconds / 60;
  int seconds = totalSeconds % 60;
  char timeStr[6];
  sprintf(timeStr, "%02d:%02d", minutes, seconds);
  return String(timeStr);
}

void playNudgeSound() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(50); 
  digitalWrite(BUZZER_PIN, LOW);
}

void playStartSound() {
  playNudgeSound();
  delay(100);
  playNudgeSound();
}

void playEndSound() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(300);
  digitalWrite(BUZZER_PIN, LOW);
}