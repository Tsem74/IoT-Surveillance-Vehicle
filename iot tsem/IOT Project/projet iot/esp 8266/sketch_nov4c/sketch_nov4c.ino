#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* ssid = "pchpch";
const char* password = "tsem7447";

ESP8266WebServer server(80);

const int motorA_IN1 = D1;
const int motorA_IN2 = D2;
const int motorA_ENA = D3;
const int motorB_IN3 = D4;
const int motorB_IN4 = D5;
const int motorB_ENB = D6;

const int DRIVE_POWER = 255;
const int STEER_POWER = 255;
const int STEER_TIME = 500;

unsigned long lastCommandTime = 0;
const unsigned long COMMAND_COOLDOWN = 50;
const unsigned long AUTO_STOP_TIME = 2000;

void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(motorA_IN1, OUTPUT);
  pinMode(motorA_IN2, OUTPUT);
  pinMode(motorA_ENA, OUTPUT);
  pinMode(motorB_IN3, OUTPUT);
  pinMode(motorB_IN4, OUTPUT);
  pinMode(motorB_ENB, OUTPUT);

  stopAllMotors();

  Serial.println();
  Serial.println("RC Car Controller - 5V MODE");
  Serial.println("Power Supply: 5V");
  Serial.println("Drive Power: " + String(DRIVE_POWER));
  Serial.println("Steering Power: " + String(STEER_POWER));
  Serial.println("Steering Time: " + String(STEER_TIME) + " ms");

  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);

  Serial.print("🛜 Connecting to: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  unsigned long connectionStart = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - connectionStart < 10000) {
    delay(200);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.print("🌐 IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi Failed, Restarting...");
    delay(2000);
    ESP.restart();
  }

  Serial.println("Testing motors (gentle 5V check)...");
  testAllMotors();

  server.on("/", HTTP_GET, []() { server.send(200, "text/html", getOptimizedHTML()); });
  server.on("/forward", HTTP_GET, []() { server.send(200, "text/plain", "OK"); handleMotorCommand(); moveForward(); });
  server.on("/backward", HTTP_GET, []() { server.send(200, "text/plain", "OK"); handleMotorCommand(); moveBackward(); });
  server.on("/left", HTTP_GET, []() { server.send(200, "text/plain", "OK"); handleMotorCommand(); steerLeft(); });
  server.on("/right", HTTP_GET, []() { server.send(200, "text/plain", "OK"); handleMotorCommand(); steerRight(); });
  server.on("/stop", HTTP_GET, []() { server.send(200, "text/plain", "OK"); handleMotorCommand(); stopAllMotors(); });

  server.begin();
  Serial.println("HTTP Server Started - 5V Power Mode!");
}

void loop() {
  server.handleClient();
  if (millis() - lastCommandTime > AUTO_STOP_TIME) stopAllMotors();
}

void handleMotorCommand() {
  if (millis() - lastCommandTime < COMMAND_COOLDOWN) return;
  lastCommandTime = millis();
}

void testAllMotors() {
  Serial.println("🔧 Quick Motor Check (Low Power 5V Mode)...");
  int testPower = DRIVE_POWER / 2;
  int testTime = 300;

  analogWrite(motorA_ENA, testPower);
  digitalWrite(motorA_IN1, LOW);
  digitalWrite(motorA_IN2, HIGH);
  Serial.println("→ Forward pulse...");
  delay(testTime);
  stopDriveMotor();
  delay(200);

  analogWrite(motorA_ENA, testPower);
  digitalWrite(motorA_IN1, HIGH);
  digitalWrite(motorA_IN2, LOW);
  Serial.println("← Backward pulse...");
  delay(testTime);
  stopDriveMotor();
  delay(200);

  analogWrite(motorB_ENB, testPower);
  digitalWrite(motorB_IN3, HIGH);
  digitalWrite(motorB_IN4, LOW);
  Serial.println("↰ Left steer pulse...");
  delay(200);
  stopSteering();
  delay(200);

  analogWrite(motorB_ENB, testPower);
  digitalWrite(motorB_IN3, LOW);
  digitalWrite(motorB_IN4, HIGH);
  Serial.println("↱ Right steer pulse...");
  delay(200);
  stopSteering();

  Serial.println("✅ Motor check complete (low-power test)");
}

String getOptimizedHTML() {
  return R"=====(
<!DOCTYPE html>
<html>
<head>
  <title>RC Car - 5V Power</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta http-equiv="Cache-Control" content="no-cache">
  <style>
    * { margin:0; padding:0; box-sizing:border-box; -webkit-tap-highlight-color:transparent; }
    body { font-family:Arial; background:#1a1a1a; padding:15px; user-select:none; }
    .container { max-width:320px; margin:0 auto; }
    h1 { color:#00ff88; text-align:center; margin-bottom:10px; font-size:24px; }
    .status { color:#4CAF50; text-align:center; margin-bottom:15px; font-size:14px; font-weight:bold; }
    .power-info { color:#ff9800; text-align:center; margin-bottom:10px; font-size:12px; }
    .btn { display:block; width:100%; padding:20px 15px; margin:6px 0; font-size:18px; font-weight:bold; border:none; border-radius:10px; cursor:pointer; transition:all 0.05s; color:white; box-shadow:0 3px 6px rgba(0,0,0,0.3); }
    .btn:active { transform:scale(0.95); box-shadow:0 1px 3px rgba(0,0,0,0.2); }
    .forward { background:#27ae60; }
    .backward { background:#e74c3c; }
    .left { background:#3498db; width:48%; display:inline-block; margin-right:2%; }
    .right { background:#f39c12; width:48%; display:inline-block; margin-left:2%; }
    .stop { background:#7f8c8d; }
    .row { text-align:center; margin:8px 0; }
    .instructions { color:#666; text-align:center; margin-top:15px; font-size:11px; }
  </style>
</head>
<body>
  <div class="container">
    <h1>RC CAR</h1>
    <div class="power-info">5V Power Mode | All Motors: 255/255 PWM</div>
    <div class="status" id="status">🟢 Ready - 5V Power</div>
    <button class="btn forward" ontouchstart="sendCommand('forward')" ontouchend="sendCommand('stop')" onmousedown="sendCommand('forward')" onmouseup="sendCommand('stop')" onmouseleave="sendCommand('stop')">▲ FORWARD</button>
    <div class="row">
      <button class="btn left" ontouchstart="sendCommand('left')" onmousedown="sendCommand('left')">◀ LEFT</button>
      <button class="btn right" ontouchstart="sendCommand('right')" onmousedown="sendCommand('right')">RIGHT ▶</button>
    </div>
    <button class="btn stop" onclick="sendCommand('stop')">⏹ STOP</button>
    <button class="btn backward" ontouchstart="sendCommand('backward')" ontouchend="sendCommand('stop')" onmousedown="sendCommand('backward')" onmouseup="sendCommand('stop')" onmouseleave="sendCommand('stop')">▼ BACKWARD</button>
    <div class="instructions">5V Power Supply Active<br>Max PWM: 255/255 | Steering time extended</div>
  </div>
  <script>
    let lastCommandTime = 0;
    const MIN_COMMAND_INTERVAL = 50;
    function sendCommand(cmd) {
      const now = Date.now();
      if (now - lastCommandTime < MIN_COMMAND_INTERVAL) return;
      lastCommandTime = now;
      document.getElementById('status').textContent = '🎯 ' + cmd.toUpperCase();
      fetch('/' + cmd, { method:'GET', cache:'no-cache' })
        .then(r => document.getElementById('status').textContent = '✅ ' + cmd.toUpperCase())
        .catch(e => document.getElementById('status').textContent = '❌ ' + cmd.toUpperCase());
      event.preventDefault();
      return false;
    }
    document.addEventListener('contextmenu', e => e.preventDefault());
  </script>
</body>
</html>
)=====";
}

void moveForward() {
  digitalWrite(motorA_IN1, LOW);
  digitalWrite(motorA_IN2, HIGH);
  analogWrite(motorA_ENA, DRIVE_POWER);
  Serial.println("Moving Forward (reversed wiring)");
}

void moveBackward() {
  digitalWrite(motorA_IN1, HIGH);
  digitalWrite(motorA_IN2, LOW);
  analogWrite(motorA_ENA, DRIVE_POWER);
  Serial.println("Moving Backward (reversed wiring)");
}

void steerLeft() {
  digitalWrite(motorB_IN3, HIGH);
  digitalWrite(motorB_IN4, LOW);
  analogWrite(motorB_ENB, STEER_POWER);
  Serial.println("Steering LEFT - 5V MAX POWER");
  delay(STEER_TIME);
  stopSteering();
}

void steerRight() {
  digitalWrite(motorB_IN3, LOW);
  digitalWrite(motorB_IN4, HIGH);
  analogWrite(motorB_ENB, STEER_POWER);
  Serial.println("Steering RIGHT - 5V MAX POWER");
  delay(STEER_TIME);
  stopSteering();
}

void stopDriveMotor() {
  digitalWrite(motorA_IN1, LOW);
  digitalWrite(motorA_IN2, LOW);
  analogWrite(motorA_ENA, 0);
}

void stopSteering() {
  digitalWrite(motorB_IN3, LOW);
  digitalWrite(motorB_IN4, LOW);
  analogWrite(motorB_ENB, 0);
}

void stopAllMotors() {
  stopDriveMotor();
  stopSteering();
}
