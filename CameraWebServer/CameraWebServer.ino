#include "esp_camera.h"
#include <WiFi.h>
#include <esp_http_server.h>

// === Camera pin config for AI Thinker ===
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// === Wi-Fi credentials ===
const char* ssid = "pchpch";
const char* password = "tsem7447";

// === Configuration constants ===
#define WIFI_TIMEOUT_MS 20000
#define WIFI_RECONNECT_DELAY 5000

// === Function prototypes ===
void startCameraServer();
bool setupCamera();
bool connectWiFi();
void printCameraStatus();
void printMemoryInfo();
void optimizeCameraSettings();

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println("\n🚀 Booting optimized ESP32-Camera...");

  // Print initial memory info
  printMemoryInfo();

  // Setup camera with enhanced error handling
  if (!setupCamera()) {
    Serial.println("❌ Camera setup failed! Restarting...");
    delay(2000);
    ESP.restart();
  }

  // Connect WiFi with improved timeout and reconnection
  if (!connectWiFi()) {
    Serial.println("❌ WiFi connection failed! Restarting...");
    delay(2000);
    ESP.restart();
  }

  // Print system status
  Serial.println("\n✅ System ready!");
  Serial.print("📹 Video Stream: http://");
  Serial.print(WiFi.localIP());
  Serial.println(":81/stream");
  Serial.print("📷 Still Image: http://");
  Serial.print(WiFi.localIP());
  Serial.println(":81/capture");

  // Start stream server
  startCameraServer();

  // Final memory check
  printMemoryInfo();
}

bool setupCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;

  config.xclk_freq_hz = 18000000; // Increased to 20MHz for better performance
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_LATEST;

  // === OPTIMIZED MEMORY SETTINGS ===
  if (psramFound()) {
    config.frame_size = FRAMESIZE_VGA;    // 800x600 - good balance
    config.jpeg_quality = 15;              // Balanced quality/performance
    config.fb_count = 1;                   // Double buffer
    config.fb_location = CAMERA_FB_IN_PSRAM; // Force PSRAM usage
    Serial.println("✅ PSRAM detected - using optimized settings");
  } else {
    config.frame_size = FRAMESIZE_QVGA;    // 320x240
    config.jpeg_quality = 15;
    config.fb_count = 1;
    Serial.println("⚠️  No PSRAM - using fallback settings");
  }

  // Enhanced error reporting with specific handling
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("❌ Camera init failed: 0x%x - %s\n", err, esp_err_to_name(err));
    
    // Specific error handling
    if (err == ESP_ERR_CAMERA_NOT_DETECTED) {
      Serial.println("🔌 Check camera connection and wiring!");
    } else if (err == ESP_ERR_NO_MEM) {
      Serial.println("💥 Not enough PSRAM! Try smaller frame size.");
    } else if (err == ESP_ERR_CAMERA_NOT_SUPPORTED) {
      Serial.println("🔧 Unsupported camera module!");
    }
    return false;
  }

  // Apply optimized camera settings
  optimizeCameraSettings();
  
  // Print camera status
  printCameraStatus();
  
  Serial.println("✅ Camera initialized successfully");
  return true;
}

bool connectWiFi() {
  Serial.printf("📡 Connecting to: %s\n", ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  
  // Configure WiFi for better performance
  WiFi.setTxPower(WIFI_POWER_19_5dBm); // Max power
  WiFi.begin(ssid, password);
  
  unsigned long startAttemptTime = millis();
  
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startAttemptTime > WIFI_TIMEOUT_MS) {
      Serial.printf("❌ Failed to connect to %s\n", ssid);
      
      // Scan for networks for debugging
      Serial.println("🔍 Scanning available networks...");
      int n = WiFi.scanNetworks();
      if (n == 0) {
        Serial.println("   No networks found");
      } else {
        Serial.println("   Available networks:");
        for (int i = 0; i < n; i++) {
          Serial.printf("   %s (RSSI: %d) %s\n", 
                       WiFi.SSID(i).c_str(), 
                       WiFi.RSSI(i),
                       (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "open" : "secured");
        }
      }
      return false;
    }
    delay(500);
    Serial.print(".");
  }
  
  Serial.printf("\n✅ WiFi connected! IP: %s, RSSI: %d dBm\n", 
                WiFi.localIP().toString().c_str(), WiFi.RSSI());
  return true;
}

void optimizeCameraSettings() {
  sensor_t *s = esp_camera_sensor_get();
  if (s == NULL) {
    Serial.println("⚠️  Cannot get camera sensor for optimization");
    return;
  }

  // Optimize for streaming
  s->set_brightness(s, 0);     // 0-2
  s->set_contrast(s, 0);       // 0-2
  s->set_saturation(s, 0);     // -2 to 2
  s->set_special_effect(s, 0); // 0-6 (0 = no effect)
  s->set_whitebal(s, 1);       // 0 = disable, 1 = enable
  s->set_awb_gain(s, 1);       // Auto white balance gain
  s->set_wb_mode(s, 0);        // 0 to 4 - Auto
  s->set_exposure_ctrl(s, 1);  // 1 = enable auto exposure
  s->set_aec2(s, 0);           // 0 = disable AEC2
  s->set_ae_level(s, 0);       // -2 to 2
  s->set_aec_value(s, 300);    // 0-1200
  s->set_gain_ctrl(s, 1);      // Auto gain
  s->set_agc_gain(s, 0);       // 0-30
  s->set_gainceiling(s, (gainceiling_t)0);  // 0-6
  s->set_bpc(s, 0);            // Black pixel correction
  s->set_wpc(s, 1);            // White pixel correction
  s->set_raw_gma(s, 1);        // Gamma correction
  s->set_lenc(s, 1);           // Lens correction
  s->set_hmirror(s, 0);        // Flip horizontally
  s->set_vflip(s, 0);          // Flip vertically
  s->set_dcw(s, 1);            // Downsize enable
  s->set_colorbar(s, 0);       // Color bar test pattern

  Serial.println("✅ Camera settings optimized for streaming");
}

void printCameraStatus() {
  sensor_t *s = esp_camera_sensor_get();
  if (s != NULL) {
    // Correct way to get sensor information
    Serial.printf("📸 Camera sensor detected - Framesize: %d, Quality: %d\n", 
                  s->status.framesize, s->status.quality);
  } else {
    Serial.println("⚠️  Could not read camera status");
  }
}

void printMemoryInfo() {
  // Correct PSRAM detection and memory info
  bool psramAvailable = psramFound();
  Serial.printf("💾 Heap: %d bytes free | PSRAM: %s | PSRAM Free: %d bytes\n",
                ESP.getFreeHeap(),
                psramAvailable ? "Yes" : "No",
                psramAvailable ? ESP.getFreePsram() : 0);
}

void loop() {
  static unsigned long lastStatusCheck = 0;
  static unsigned long lastMemoryCheck = 0;
  
  unsigned long currentMillis = millis();
  
  // WiFi status check every 15 seconds
  if (currentMillis - lastStatusCheck > 15000) {
    lastStatusCheck = currentMillis;
    
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("⚠️  WiFi disconnected! Attempting reconnect...");
      WiFi.reconnect();
      
      // If still disconnected after 30 seconds, restart
      delay(30000);
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("🔁 Restarting ESP due to WiFi failure...");
        ESP.restart();
      } else {
        Serial.println("✅ WiFi reconnected successfully!");
      }
    } else {
      Serial.printf("📶 Status: Connected | RSSI: %d dBm | IP: %s\n", 
                    WiFi.RSSI(), WiFi.localIP().toString().c_str());
    }
  }
  
  // Memory monitoring every 60 seconds
  if (currentMillis - lastMemoryCheck > 60000) {
    lastMemoryCheck = currentMillis;
    printMemoryInfo();
  }
  
  delay(1000);
}