// Simply upload this code to your Esp32 wroom or any compatible boards
// ESP32 Code for Wireless Holonomic Robot Control via WebSocket Web Server (AP Mode) created by Vikas Singh Thakur

#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

// AP credentials (change if desired)
const char* ap_ssid = "RobotControl";
const char* ap_password = "12345678";

// Web server on port 80
WebServer server(80);
// WebSocket server on port 81
WebSocketsServer webSocket(81);

// Wheel Pins (same as before)
#define W1_IN1 21
#define W1_IN2 19
#define W1_EN  32

#define W2_IN1 27
#define W2_IN2 14
#define W2_EN  13

#define W3_IN1 22
#define W3_IN2 23
#define W3_EN  26

int vx = 0, vy = 0, vr = 0;

// Target and current wheel speeds for ramping
float target_w1 = 0, target_w2 = 0, target_w3 = 0;
float current_w1 = 0, current_w2 = 0, current_w3 = 0;

// Ramping parameters
const float RAMP_RATE = 0.1;  // 0.0 to 1.0; lower = slower ramp (smoother but less responsive)
const unsigned long UPDATE_INTERVAL = 20;  // ms between updates
unsigned long last_update = 0;

// Overall max speed reduction (out of 100)
const float MAX_SPEED_SCALE = 0.7;  // 70% of full speed; adjust as needed (0.5 for half speed)

// Calibration Parameters (tuned for smoother response: lower Ks for soft curve, lower Kb for less jump)
float Kp1 = 1.00, Kb1 = 15, Ks1 = 0.5;
float Kp2 = 1.00, Kb2 = 15, Ks2 = 0.5;  // Set to 1.00 for balance
float Kp3 = 1.00, Kb3 = 15, Ks3 = 0.5;

// HTML webpage with JS for pad and keyboard control (unchanged)
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Robot Control</title>
    <style>
        canvas { border: 1px solid black; background: lightgray; }
    </style>
</head>
<body>
    <h1>Robot Control Pad</h1>
    <canvas id="pad" width="200" height="200"></canvas>
    <p>Use mouse/touch on pad for direction/speed. Keyboard: Numpad/Arrows for directions, 9/3/7/1 for diagonals, R rotate right, L rotate left.</p>
    
    <script>
        const ws = new WebSocket('ws://' + location.hostname + ':81/');
        const canvas = document.getElementById('pad');
        const ctx = canvas.getContext('2d');
        const cx = 100, cy = 100;
        
        // Draw circle
        ctx.beginPath();
        ctx.arc(cx, cy, 95, 0, 2 * Math.PI);
        ctx.stroke();
        
        let lastX = 0, lastY = 0, lastR = 0;
        let useKeyboard = false;
        let keys = new Set();
        let kVx = 0, kVy = 0, kVr = 0;
        
        function sendCommand(vx, vy, vr) {
            if (Math.abs(vx - lastX) > 5 || Math.abs(vy - lastY) > 5 || Math.abs(vr - lastR) > 5) {
                ws.send(`V ${vx} ${vy} ${vr}`);
                lastX = vx; lastY = vy; lastR = vr;
            }
        }
        
        function stopRobot() {
            ws.send('S');
            lastX = 0; lastY = 0; lastR = 0;
        }
        
        // Mouse/Touch handling
        function handleMove(event) {
            if (useKeyboard) return;
            event.preventDefault();
            let rect = canvas.getBoundingClientRect();
            let mx = (event.touches ? event.touches[0].clientX : event.clientX) - rect.left;
            let my = (event.touches ? event.touches[0].clientY : event.clientY) - rect.top;
            let dx = mx - cx;
            let dy = cy - my;  // Invert y for forward positive
            let dist = Math.sqrt(dx*dx + dy*dy);
            if (dist > 100) {
                dx *= 100 / dist;
                dy *= 100 / dist;
            }
            let x = Math.round(dx);
            let y = Math.round(dy);
            sendCommand(x, y, 0);
        }
        
        function handleEnd(event) {
            if (!useKeyboard) stopRobot();
        }
        
        canvas.addEventListener('mousemove', handleMove);
        canvas.addEventListener('touchmove', handleMove);
        canvas.addEventListener('mouseleave', handleEnd);
        canvas.addEventListener('touchend', handleEnd);
        
        // Keyboard handling
        document.addEventListener('keydown', (e) => {
            keys.add(e.key.toLowerCase());
            updateKeyboard();
            useKeyboard = true;
        });
        
        document.addEventListener('keyup', (e) => {
            keys.delete(e.key.toLowerCase());
            updateKeyboard();
            if (keys.size === 0) {
                useKeyboard = false;
                stopRobot();
            }
        });
        
        function updateKeyboard() {
            let vx = 0, vy = 0, vr = 0;
            
            // Directions
            if (keys.has('arrowup') || keys.has('8')) vy += 100;
            if (keys.has('arrowdown') || keys.has('2')) vy -= 100;
            if (keys.has('arrowleft') || keys.has('4')) vx -= 100;
            if (keys.has('arrowright') || keys.has('6')) vx += 100;
            
            // Diagonals (override if pressed)
            if (keys.has('9')) { vx = 100; vy = 100; }
            if (keys.has('3')) { vx = 100; vy = -100; }
            if (keys.has('7')) { vx = -100; vy = 100; }
            if (keys.has('1')) { vx = -100; vy = -100; }
            
            // Rotation
            if (keys.has('r')) vr += 100;
            if (keys.has('l')) vr -= 100;
            
            // Normalize translation
            let dist = Math.sqrt(vx*vx + vy*vy);
            if (dist > 100) {
                vx = Math.round(vx * 100 / dist);
                vy = Math.round(vy * 100 / dist);
            }
            
            sendCommand(vx, vy, vr);
        }
        
        ws.onopen = () => console.log('Connected');
        ws.onclose = () => console.log('Disconnected');
    </script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  
  // Motor pins
  pinMode(W1_IN1, OUTPUT);
  pinMode(W1_IN2, OUTPUT);
  pinMode(W1_EN, OUTPUT);
  pinMode(W2_IN1, OUTPUT);
  pinMode(W2_IN2, OUTPUT);
  pinMode(W2_EN, OUTPUT);
  pinMode(W3_IN1, OUTPUT);
  pinMode(W3_IN2, OUTPUT);
  pinMode(W3_EN, OUTPUT);
  stopAll();
  
  // Start AP mode
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);
  Serial.println("AP started");
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());  // Usually 192.168.4.1
  
  // Serve webpage
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", htmlPage);
  });
  
  server.begin();
  
  // WebSocket
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  server.handleClient();
  webSocket.loop();
  
  // Update motor speeds with ramping
  unsigned long now = millis();
  if (now - last_update >= UPDATE_INTERVAL) {
    last_update = now;
    
    // Ramp currents towards targets
    current_w1 += (target_w1 - current_w1) * RAMP_RATE;
    current_w2 += (target_w2 - current_w2) * RAMP_RATE;
    current_w3 += (target_w3 - current_w3) * RAMP_RATE;
    
    // Snap to target if very close (avoid drift)
    if (abs(target_w1 - current_w1) < 1) current_w1 = target_w1;
    if (abs(target_w2 - current_w2) < 1) current_w2 = target_w2;
    if (abs(target_w3 - current_w3) < 1) current_w3 = target_w3;
    
    // Drive motors with current speeds
    driveMotor(W1_IN1, W1_IN2, W1_EN, current_w1);
    driveMotor(W2_IN1, W2_IN2, W2_EN, current_w2);
    driveMotor(W3_IN1, W3_IN2, W3_EN, current_w3);
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      stopAll();
      break;
    case WStype_CONNECTED:
      Serial.printf("[%u] Connected!\n", num);
      break;
    case WStype_TEXT:
      String cmd = (char*)payload;
      parseCommand(cmd);
      break;
  }
}

void parseCommand(String cmd) {
  if (cmd.startsWith("V")) {
    sscanf(cmd.c_str(), "V %d %d %d", &vx, &vy, &vr);
    omniMove(vx, vy, vr);
  } else if (cmd.startsWith("S")) {
    stopAll();
  }
}

void omniMove(int vx, int vy, int vr) {
  float wheel1 = vy - vr;
  float wheel2 = (-0.5 * vy - 0.866 * vx) - vr;
  float wheel3 = (-0.5 * vy + 0.866 * vx) - vr;
  
  // Normalize if any exceeds 100
  float maxSpeed = max(max(abs(wheel1), abs(wheel2)), abs(wheel3));
  if (maxSpeed > 100) {
    wheel1 = wheel1 / maxSpeed * 100;
    wheel2 = wheel2 / maxSpeed * 100;
    wheel3 = wheel3 / maxSpeed * 100;
  }
  
  // Apply overall speed reduction
  wheel1 *= MAX_SPEED_SCALE;
  wheel2 *= MAX_SPEED_SCALE;
  wheel3 *= MAX_SPEED_SCALE;
  
  // Apply per-wheel calibration
  target_w1 = applyCalibration(wheel1, Kp1, Kb1, Ks1);
  target_w2 = applyCalibration(wheel2, Kp2, Kb2, Ks2);
  target_w3 = applyCalibration(wheel3, Kp3, Kb3, Ks3);
  
  // Note: Actual driving happens in loop() with ramping
}

float applyCalibration(float speed, float Kp, float Kb, float Ks) {
  float adjusted = speed * Kp;
  adjusted = copysign(pow(abs(adjusted / 100.0), Ks) * 100.0, adjusted);
  if (adjusted != 0) {
    if (adjusted > 0) adjusted += Kb;
    else adjusted -= Kb;
  }
  if (adjusted > 100) adjusted = 100;
  if (adjusted < -100) adjusted = -100;
  return adjusted;
}

void driveMotor(int in1, int in2, int en, float val) {
  int pwm = map(abs(val), 0, 100, 0, 255);
  if (val > 0) {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
    analogWrite(en, pwm);
  } else if (val < 0) {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    analogWrite(en, pwm);
  } else {
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
    analogWrite(en, 0);
  }
}

void stopAll() {
  target_w1 = 0;
  target_w2 = 0;
  target_w3 = 0;
  // Let ramping in loop() handle smooth stop; for immediate stop, uncomment below:
  // current_w1 = current_w2 = current_w3 = 0;
  // driveMotor(W1_IN1, W1_IN2, W1_EN, 0);
  // driveMotor(W2_IN1, W2_IN2, W2_EN, 0);
  // driveMotor(W3_IN1, W3_IN2, W3_EN, 0);
  vx = vy = vr = 0;
}
