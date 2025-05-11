#include <Arduino.h>
#include "display_conf.h"

LGFX lcd;
HardwareSerial LidarSerial(1);

#define LIDAR_RX 17
#define LIDAR_TX 18
#define PACKET_SIZE 22

#define SLIDER_X 650
#define SLIDER_Y 80 
#define SLIDER_WIDTH 120
#define SLIDER_HEIGHT 30
#define SLIDER_THUMB_WIDTH 30
#define SLIDER_COLOR TFT_DARKGREY
#define SLIDER_THUMB_COLOR TFT_BLUE

uint8_t packet[PACKET_SIZE];
std::vector<int> distances(360, 0);
float rpm = 0.0;
float zoomFactor = 1.0;  
bool sliderActive = false;
bool gridNeedsUpdate = true;  
float prevZoomFactor = 1.0;   
int prevMinVal = 0, prevMaxVal = 0; 
float prevRpm = 0.0;          

struct Measurement {
  uint16_t angle;
  uint16_t distance;
};

void decode_packet(uint8_t* packet, Measurement* measurements) {
  uint8_t angle_index = packet[1] - 0xA0;
  rpm = ((packet[3] << 8) | packet[2]) / 64.0;

  for (int i = 0; i < 4; i++) {
    uint8_t byte0 = packet[4 + i * 4];
    uint8_t byte1 = packet[4 + i * 4 + 1];

    uint16_t distance_mm = ((byte1 & 0x3F) << 8) | byte0;
    uint16_t angle_deg = (angle_index * 4 + i) % 360;

    if (byte1 & 0x80) distance_mm = 0;

    measurements[i].angle = angle_deg;
    measurements[i].distance = distance_mm;
  }
}
float pointToLineDistance(float x, float y, float x1, float y1, float x2, float y2) {
  float A = x - x1;
  float B = y - y1;
  float C = x2 - x1;
  float D = y2 - y1;

  float dot = A * C + B * D;
  float len_sq = C * C + D * D;
  float param = -1;
  if (len_sq != 0) 
    param = dot / len_sq;

  float xx, yy;

  if (param < 0) {
    xx = x1;
    yy = y1;
  }
  else if (param > 1) {
    xx = x2;
    yy = y2;
  }
  else {
    xx = x1 + param * C;
    yy = y1 + param * D;
  }

  float dx = x - xx;
  float dy = y - yy;
  return sqrt(dx * dx + dy * dy);
}
void drawGrid() {
  int centerX = lcd.width() / 2;
  int centerY = lcd.height() / 2;
  float maxRadius = 215.0;
  float scale = (maxRadius / 2000.0) * zoomFactor;
  lcd.fillCircle(centerX, centerY, maxRadius + 10, TFT_WHITE);
  for (int r = 400; r <= 2000; r += 400) {
    int radius = r * scale;
    if (radius <= maxRadius) { 
      lcd.drawCircle(centerX, centerY, radius, TFT_BLACK);
      lcd.setCursor(centerX + radius, centerY + (-r) / 40);
      lcd.printf("%d mm", r);
    }
  }

  for (int angle = 0; angle < 360; angle += 30) {
    float adjustedAngle = (angle - 270) % 360;
    float angleRad = adjustedAngle * DEG_TO_RAD;
    int x = centerX + maxRadius * sin(angleRad);
    int y = centerY + maxRadius * cos(angleRad);
    lcd.drawLine(centerX, centerY, x, y, TFT_BLACK);
    lcd.setCursor(x, y);
    lcd.printf("%d", angle);
  }
  
  gridNeedsUpdate = false;
  prevZoomFactor = zoomFactor;
}

uint16_t getColorFromValue(int value, int minVal, int maxVal) {
  float ratio = (float)(value - minVal) / (maxVal - minVal);
  
  if (ratio < 0.5) {
    return lcd.color565(0, 255 * (ratio * 2), 0); // Yeşil → Sarı
  } else {
    return lcd.color565(255 * ((ratio - 0.5) * 2), 255 - (255 * ((ratio - 0.5) * 2)), 0); // Sarı → Kırmızı
  }
}

void drawPolarPlot(int minVal, int maxVal) {
  static std::vector<int> prevDistances(360, 0); 

  int centerX = lcd.width() / 2;
  int centerY = lcd.height() / 2;
  float maxRadius = 215.0;
  float scale = (maxRadius / 2000.0) * zoomFactor;
  
  for (int i = 0; i < 360; i++) {
    if (distances[i] != prevDistances[i] || gridNeedsUpdate) {
      float adjustedAngle = (i + 180) % 360;
      float angleRad = adjustedAngle * DEG_TO_RAD;

      if (!gridNeedsUpdate && prevDistances[i] > 0) {
        int prevR = prevDistances[i] * scale;
        if (prevR <= maxRadius) {
          int prevX = centerX + prevR * cos(angleRad);
          int prevY = centerY + prevR * sin(angleRad);
          
          lcd.fillCircle(prevX, prevY, 2, TFT_WHITE);
          for (int r = 400; r <= 2000; r += 400) {
            int radius = r * scale;
            if (radius <= maxRadius) {
              float dist = sqrt(pow(prevX - centerX, 2) + pow(prevY - centerY, 2));
              if (abs(dist - radius) < 3) {
                float angle = atan2(prevY - centerY, prevX - centerX);
                int x1 = centerX + radius * cos(angle - 0.03);
                int y1 = centerY + radius * sin(angle - 0.03);
                int x2 = centerX + radius * cos(angle + 0.03);
                int y2 = centerY + radius * sin(angle + 0.03);
                lcd.drawLine(x1, y1, x2, y2, TFT_BLACK);
              }
            }
          }
          
          for (int angle = 0; angle < 360; angle += 30) {
            float adjustedAngleGrid = (angle - 270) % 360;
            float angleRadGrid = adjustedAngleGrid * DEG_TO_RAD;
            int x = centerX + maxRadius * sin(angleRadGrid);
            int y = centerY + maxRadius * cos(angleRadGrid);
            
            float dist = pointToLineDistance(prevX, prevY, centerX, centerY, x, y);
            if (dist < 3) {
              lcd.drawLine(centerX, centerY, x, y, TFT_BLACK);
            }
          }
        }
      }

      if (distances[i] > 0) {
        int r = distances[i] * scale;
        if (r <= maxRadius) {
          int x = centerX + r * cos(angleRad);
          int y = centerY + r * sin(angleRad);
          uint16_t color = getColorFromValue(distances[i], maxVal, minVal);
          lcd.fillCircle(x, y, 2, color);
        }
      }
      
      prevDistances[i] = distances[i];
    }
  }
}

void drawRPM() {
  if (abs(rpm - prevRpm) > 0.2) {
    lcd.fillRect(650, 10, 140, 40, TFT_WHITE);
    lcd.setTextColor(TFT_BLACK, TFT_WHITE);
    lcd.setTextSize(2);
    lcd.setCursor(660, 20);
    lcd.printf("RPM: %.1f", rpm);
    lcd.setTextSize(1);
    prevRpm = rpm;
  }
}

void drawSlider() {
  lcd.fillRect(SLIDER_X, SLIDER_Y, SLIDER_WIDTH, SLIDER_HEIGHT, SLIDER_COLOR);
    int thumbPosition = SLIDER_X + (zoomFactor - 0.5) * (SLIDER_WIDTH - SLIDER_THUMB_WIDTH) / 2.0;
  lcd.fillRect(thumbPosition, SLIDER_Y, SLIDER_THUMB_WIDTH, SLIDER_HEIGHT, SLIDER_THUMB_COLOR);
    lcd.fillRect(SLIDER_X, SLIDER_Y - 30, SLIDER_WIDTH, 25, TFT_WHITE);
  lcd.setTextColor(TFT_BLACK, TFT_WHITE);
  lcd.setTextSize(2);
  lcd.setCursor(SLIDER_X + 5, SLIDER_Y - 25);
  lcd.printf("Zoom: %.1fx", zoomFactor);
  lcd.setTextSize(1);
}

void handleTouch() {
  uint16_t x, y;
  if (lcd.getTouch(&x, &y)) {
    if (y >= SLIDER_Y && y <= SLIDER_Y + SLIDER_HEIGHT &&
        x >= SLIDER_X && x <= SLIDER_X + SLIDER_WIDTH) {
      sliderActive = true;
    }
    
    if (sliderActive) {
      float newZoomFactor = 0.5 + ((x - SLIDER_X) * 2.0) / SLIDER_WIDTH;
      newZoomFactor = constrain(newZoomFactor, 0.5, 2.5);
            if (abs(newZoomFactor - zoomFactor) > 0.05) {
        zoomFactor = newZoomFactor;
        drawSlider();
        gridNeedsUpdate = true;  
      }
    }
  } else {
    sliderActive = false;
  }
}

void setup() {
  lcd.init();
  lcd.setRotation(2);
  lcd.fillScreen(TFT_WHITE); 
  lcd.setTextColor(TFT_BLACK, TFT_WHITE);

  lcd.setCursor(10, 10);
  lcd.setTextSize(2);
  lcd.println("Xiaomi LIDAR");
  
  
  drawSlider();
  drawGrid();  
  
  Serial.begin(115200);
  LidarSerial.begin(115200, SERIAL_8N1, LIDAR_RX, LIDAR_TX);

  Serial.println("ESP32 LIDAR Başlatıldı");
}

void loop() {
  static int packet_index = 0;
  static int minVal = 2000, maxVal = 0;
  static unsigned long lastRefreshTime = 0;
  static unsigned long lastSlowRefreshTime = 0;
  static unsigned long lastGridRefreshTime = 0;
  
  handleTouch();

  while (LidarSerial.available()) {
    uint8_t byte = LidarSerial.read();

    if (byte == 0xFA && packet_index == 0) { 
      packet[packet_index++] = byte;
    }
    else if (packet_index > 0) {
      packet[packet_index++] = byte;

      if (packet_index == PACKET_SIZE) {
        Measurement measurements[4];
        decode_packet(packet, measurements);

        for (int i = 0; i < 4; i++) {
          if (measurements[i].distance > 0) { 
            distances[measurements[i].angle] = measurements[i].distance;
          }
        }

        packet_index = 0;
      }
    }
  }

  unsigned long currentTime = millis();
  if (currentTime - lastSlowRefreshTime > 100) {
    minVal = 2000;
    maxVal = 0;
    for (int i = 0; i < 360; i++) {
      if (distances[i] > 0) {
        minVal = min(minVal, distances[i]);
        maxVal = max(maxVal, distances[i]);
      }
    }
    
    if (abs(minVal - prevMinVal) > 50 || abs(maxVal - prevMaxVal) > 50) {
      prevMinVal = minVal;
      prevMaxVal = maxVal;
    }
    
    drawRPM();
    
    lastSlowRefreshTime = currentTime;
  }
  
  if (currentTime - lastRefreshTime > 30) {
    if (gridNeedsUpdate) {
      drawGrid();
    }
    
    drawPolarPlot(minVal, maxVal);
    
    lastRefreshTime = currentTime;
  }
  
  if (currentTime - lastGridRefreshTime > 5000) {
    drawGrid();
    lastGridRefreshTime = currentTime;
  }
}