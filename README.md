
# ESP32-based Lidar Visualization with Sunton 7" Touch Display

This project visualizes real-time Lidar data from a **Xiaomi LDS Lidar sensor** using an **ESP32-based Sunton 7-inch touch display**. The visual interface includes zoom controls and a polar plot to represent 360° distance measurements.

> ⚠️ This project was primarily developed using PlatformIO

## 📷 Preview


![Lidar Visualization](images/lidar_visualization.jpg)
![Xiaomi Lidar Sensor](images/xiaomi_lidar.jpg)

---

## 🚀 Features

- Real-time 360° polar plotting of lidar distance data
- Touch-based zoom slider for scaling view from 0.5x to 2.5x
- Circular and radial grid rendering with dynamic color-coded distance dots
- RPM (rotation speed) display extracted from lidar data packets
- Efficient redraw system for optimized rendering

---

## 🧠 AI Assistance Disclosure

> This project was developed with substantial assistance from AI tools (ChatGPT by OpenAI) for writing and optimizing code, documentation, and design concepts.

---

## 🛠️ Hardware Used

| Component             | Model / Details                     |
|----------------------|--------------------------------------|
| Microcontroller      | ESP32 (Sunton 7" Touch Board)        |
| Display              | 7-inch IPS Touch LCD (800x480)       |
| Lidar Sensor         | Xiaomi LDS (used in robot vacuums)   |
| Power Supply         | 5V/2A USB or battery pack            |

---

## 🧰 Software & Libraries

- **PlatformIO** as the build environment
- **LovyanGFX** (`lovyan03/LovyanGFX@^1.1.7`) for display rendering

---

## 📁 Folder Structure

```
lidar-visualization-esp32/
├── src/
│   └── main.cpp               # Main application code
├── include/
│   └── display_conf.h         # LovyanGFX display configuration
├── images/
│   ├── lidar_visualization.gif
│   └── xiaomi_lidar.jpg
└── README.md
```

---

## 📝 How It Works

1. The ESP32 reads 22-byte packets from the Xiaomi Lidar sensor using UART.
2. Each packet contains 4 measurements with angle and distance data.
3. The system decodes and stores the data in a 360-element buffer.
4. The display visualizes this data in a polar coordinate system.
5. Touch input adjusts the zoom level in real-time.

---

## 📦 Future Work

- Add logging feature via SD card
- Integrate obstacle detection zones
- Improve UI with more touch gestures
- Add Lidar point cloud export (CSV)

---

## 🔍 Lidar Data Frame Format

Each frame from the lidar is 22 bytes:

| Byte Index | Description |
|------------|-------------|
| 0          | Header (`0xFA`) |
| 1          | Packet Index (0xA0–0xA5) |
| 2–3        | Motor speed (RPM, 2 bytes) |
| 4–21       | 4 measurement blocks (4 bytes each) |

Each measurement block contains:
- 2 bytes of distance (10-bit valid data)
- Flags for quality and validity
- Angle derived using the index and measurement offset


## 🔌 Wiring Guide

| Lidar Pin     | ESP32 Pin |
|---------------|-----------|
| TX            | GPIO17    |
| Vcc(Motor)    | 3.3V      |
| GND           | GND       |
| VCC (5V)      | VIN / 5V  |

Make sure Lidar RX/TX voltages are compatible with ESP32 logic levels.

---
## 📃 License

This project is open-source and available under the MIT License.
## 📫 Contact

For feedback, improvements, or collaboration:

- GitHub: [ozknyusuf](https://github.com/ozknyusuf)
- LinkedIn: [Yusuf ÖZKAN](https://www.linkedin.com/in/yusuf-özkan-a216a7249/)