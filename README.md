# ESP32 Motion Detection (WiFi CSI)

> Experimental real-time motion detection using WiFi Channel State Information (CSI) on the ESP32.

![Status](https://img.shields.io/badge/status-WIP-orange)
![Platform](https://img.shields.io/badge/platform-ESP32-blue)
![Language](https://img.shields.io/badge/language-C%2B%2B%20%7C%20Python-green)
![License](https://img.shields.io/badge/license-MIT-brightgreen)

---

## Overview

ESP32 Motion Detection uses **WiFi Channel State Information (CSI)** instead of traditional motion sensors such as PIR modules or cameras.

By analyzing changes in wireless signals, the project can detect movement within the environment in real time.

This project is intended for experimentation, research, and learning about WiFi sensing using inexpensive ESP32 hardware.

---

## Features

- 📡 Real-time CSI capture
- 🚶 Motion detection using wireless signal variations
- 💻 Python control interface
- 📈 Live CSI visualization
- ⚡ Serial command interface
- 🔧 Adjustable detection parameters
- 🧪 Experimental detection algorithms

---

## How It Works

```
           WiFi Packets
                 │
                 ▼
        ESP32 CSI Capture
                 │
                 ▼
          Serial Connection
                 │
                 ▼
         Python Controller
                 │
        Signal Processing
                 │
                 ▼
        Motion Detection
```

The ESP32 continuously captures CSI packets from nearby WiFi traffic.

The Python application receives these measurements, processes them, filters noise, and determines whether movement has occurred.

---

## Hardware Requirements

- ESP32 Development Board
- USB Cable
- Computer running Python
- WiFi Access Point or Router

Optional:

- External antenna ESP32
- Two ESP32 boards for custom experiments

---

## Software Requirements

### ESP32

- Arduino IDE
- ESP32 Board Package

### Python

- Python 3.10+
- Required libraries

Example:

```bash
pip install pyserial numpy matplotlib
```

---

## Installation

### Clone the repository

```bash
git clone https://github.com/ymvnby-alt/ESP32MotiondetectionWIP.git
cd ESP32MotiondetectionWIP
```

---

### Upload the firmware

1. Open the Arduino project.
2. Select your ESP32 board.
3. Choose the correct COM port.
4. Upload.

---

### Start the controller

```bash
python csi_control.py
```

---

## Serial Commands

| Command | Description |
|----------|-------------|
| `SCAN` | Scan nearby WiFi networks |
| `CONNECT <SSID> <PASSWORD>` | Connect ESP32 to WiFi |
| `START` | Begin CSI capture |
| `STOP` | Stop CSI capture |
| `STATUS` | Show current status |
| `HELP` | Display available commands |

*(Update this list if additional commands are added.)*

---

## Project Structure

```
ESP32MotiondetectionWIP/

├── firmware/
│   ├── esp32_motion.ino
│
├── python/
│   ├── csi_control.py
│   ├── processing.py
│   └── visualization.py
│
├── docs/
│
├── README.md
└── LICENSE
```

*(Adjust to match the actual repository structure.)*

---

## Detection Pipeline

```
Raw CSI Packets
        │
        ▼
Amplitude Extraction
        │
        ▼
Noise Filtering
        │
        ▼
Baseline Calculation
        │
        ▼
Variance / Signal Analysis
        │
        ▼
Motion Decision
```

The current implementation focuses on reliable motion detection.

Future versions may include more advanced signal processing and machine learning.

---

## Example Output

```
Connecting...

Connected!

Starting CSI Capture...

Motion Score: 0.07

Motion Score: 0.08

Motion Score: 0.63
Motion Detected!

Motion Score: 0.11
```

---

## Current Limitations

- Work in Progress
- Detection quality depends on environment
- Requires WiFi traffic
- Not designed for multiple-person tracking
- Algorithm is still being refined

---

## Roadmap

### Core

- [x] CSI Capture
- [x] Python Controller
- [x] Motion Detection
- [ ] Better filtering
- [ ] Automatic calibration
- [ ] Configuration file

### Visualization

- [ ] Live graphs
- [ ] Heat maps
- [ ] Detection history

### Detection

- [ ] Occupancy detection
- [ ] Multi-person detection
- [ ] Gesture recognition
- [ ] Machine learning classifier

### Quality of Life

- [ ] GUI
- [ ] Logging
- [ ] Save datasets
- [ ] Home Assistant integration

---

## Performance

Performance depends on:

- WiFi signal strength
- Router placement
- ESP32 antenna
- Sampling rate
- Environmental interference

Results may vary between locations.

---

## Contributing

Contributions are welcome.

If you discover bugs or have ideas for improvements:

1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Open a Pull Request

Bug reports and suggestions are also appreciated.

---

## Disclaimer

This project is experimental and intended for research and educational purposes.

Detection accuracy is not guaranteed and should not be relied upon for security-critical applications.

---

## License

This project is licensed under the MIT License.

---

## Author

Created by **ymvnby-alt**

If you find this project useful, consider giving the repository a ⭐.
