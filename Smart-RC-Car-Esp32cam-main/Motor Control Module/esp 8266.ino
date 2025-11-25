/* ESP8266 Relay: receive frames via TCP from ESP32-CAM and serve MJPEG to PC
   Board: NodeMCU 1.0 (ESP-12E)
   Serial Monitor: 115200 baud
   Make sure to edit your Wi-Fi credentials below
*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* ssid = "Lil Broomstick 11";       // <-- change this
const char* password = "lebonbon games";   // <-- change this

WiFiServer relayServer(5000);        // TCP port to receive frames from ESP32-CAM
ESP8266WebServer httpServer(80);     // HTTP port to serve MJPEG to PC

uint8_t* lastFrameBuf = nullptr;     // pointer to latest JPEG frame
uint32_t lastFrameLen = 0;           // length of latest JPEG frame

// Serve MJPEG stream
void handleRoot() {
  WiFiClient client = httpServer.client();

  client.println("HTTP/1.1 200 OK");
  client.println("Cache-Control: no-cache");
  client.println("Pragma: no-cache");
  client.println("Connection: close");
  client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
  client.println();

  while (client.connected()) {
    if (lastFrameLen > 0 && lastFrameBuf != nullptr) {
      // send MJPEG frame
      client.print("--frame\r\n");
      client.print("Content-Type: image/jpeg\r\n");
      client.print("Content-Length: ");
      client.println(lastFrameLen);
      client.println();
      client.write(lastFrameBuf, lastFrameLen);
      client.println();
    }
    delay(100); // ~10 FPS
    yield();
  }

  client.stop();
}

void setup() {
  Serial.begin(115200);
  delay(10);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.printf("Connecting to WiFi '%s' ...\n", ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected. ESP8266 IP: ");
  Serial.println(WiFi.localIP());

  relayServer.begin();
  relayServer.setNoDelay(true);

  httpServer.on("/", handleRoot);
  httpServer.begin();

  Serial.println("Relay TCP server started on port 5000");
  Serial.println("HTTP server started on port 80");
}

void loop() {
  httpServer.handleClient();

  // Accept new TCP client from ESP32-CAM
  WiFiClient cam = relayServer.available();
  if (cam && cam.connected()) {
    Serial.println("ESP32-CAM connected to relay.");
    while (cam.connected()) {
      // Wait until we have at least 4 bytes (frame length)
      if (cam.available() >= 4) {
        uint32_t len = 0;
        if (cam.readBytes((char*)&len, 4) != 4) {
          Serial.println("Failed to read length");
          break;
        }
        if (len == 0 || len > 5*1024*1024) { // sanity check (5MB max)
          Serial.printf("Bad frame length: %u\n", len);
          break;
        }

        uint8_t* buf = (uint8_t*)malloc(len);
        if (!buf) {
          Serial.println("Memory alloc failed");
          uint32_t toDrain = len;
          while (toDrain && cam.available()) { cam.read(); toDrain--; }
          break;
        }

        size_t got = 0;
        while (got < len) {
          size_t r = cam.readBytes((char*)buf + got, len - got);
          if (r == 0) { delay(2); continue; }
          got += r;
        }

        // Save latest frame (replace old)
        if (lastFrameBuf) { free(lastFrameBuf); lastFrameBuf = nullptr; lastFrameLen = 0; }
        lastFrameBuf = buf;
        lastFrameLen = len;

        Serial.printf("Received frame %u bytes\n", len);
      } else {
        delay(2);
        yield();
      }
    }
    Serial.println("ESP32-CAM disconnected.");
    cam.stop();
  }
}
