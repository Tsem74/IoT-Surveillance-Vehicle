/* ESP32-CAM MJPEG server: stream video directly to PC
   Board: AI Thinker ESP32-CAM
   Access stream in browser: http://<ESP32-CAM-IP>:80/
*/

#include "esp_camera.h"
#include <WiFi.h>

// ---------- EDIT THESE ----------
const char* ssid = "Lil Broomstick 11";
const char* password = "lebonbon games";
// --------------------------------

// Pin definition for AI Thinker module
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM       5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#include <WebServer.h>
WebServer server(80);

camera_fb_t* captureFrame() {
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) Serial.println("Camera capture failed");
  return fb;
}

void handleJPGStream() {
  WiFiClient client = server.client();
  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
  client.print(response);

  while (client.connected()) {
    camera_fb_t* fb = captureFrame();
    if (!fb) break;

    client.printf("--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", fb->len);
    client.write(fb->buf, fb->len);
    client.print("\r\n");

    esp_camera_fb_return(fb);
    delay(100); // ~10 FPS
    yield();
  }
  client.stop();
}

void handleRoot() {
  server.send(200, "text/html",
    "<html><body>"
    "<h2>ESP32-CAM MJPEG Stream</h2>"
    "<img src=\"/stream\">"
    "</body></html>"
  );
}

void startCamera() {
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
  
  // POWER-SAVING CONFIGURATION:
  config.xclk_freq_hz = 10000000;  // Reduced from 20MHz to 10MHz
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;  // Use QVGA (320x240)
  config.jpeg_quality = 15;  // Increased (worse quality) to reduce processing
  config.fb_count = 1;       // Reduced from 2 to 1 frame buffer
  
  // Add camera sensor reset delay
  pinMode(PWDN_GPIO_NUM, OUTPUT);
  digitalWrite(PWDN_GPIO_NUM, LOW);
  delay(500);
  
  Serial.println("Initializing camera...");
  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera init failed - restarting...");
    delay(1000);
    ESP.restart();  // Auto-restart on failure
  }
  
  // Camera diagnostic
  sensor_t *s = esp_camera_sensor_get();
  if (s != NULL) {
    Serial.println("Camera sensor detected!");
    Serial.printf("Sensor ID: 0x%02X\n", s->id.PID);
    s->set_vflip(s, 1);  // Flip vertically if needed
  } else {
    Serial.println("No camera sensor detected!");
  }
}

void connectWiFi() {
  Serial.printf("Connecting to WiFi '%s' ...\n", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("WiFi connected. IP: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // Increased delay for stability
  
  Serial.println("Starting ESP32-CAM...");
  startCamera();
  connectWiFi();

  server.on("/", handleRoot);
  server.on("/stream", handleJPGStream);
  server.begin();
  Serial.println("HTTP server started on port 80");
  Serial.println("Access stream at: http://" + WiFi.localIP().toString());
}

void loop() {
  server.handleClient();
}