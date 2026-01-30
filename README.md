# Smart IoT Safe System

A full-stack IoT security system that transforms a traditional safe into a smart, connected device. This project integrates an embedded system with a web application to allow remote control, real-time monitoring, and advanced security features like OTP authentication and intruder alerts.

## Project Overview

The system bridges the gap between hardware and software using Node.js as a gateway. It allows the user to control a physical safe via a React web dashboard while maintaining high security through Telegram integration and local sensor logic.

## Key Features

- **Remote Control:** Lock and unlock the safe globally using a secure web dashboard.
- **Two-Factor Authentication (OTP):** Generates a One-Time Password sent via Telegram Bot if the physical PIN is forgotten.
- **Intruder Lockout System:** Automatically locks the system for 120 seconds after 3 incorrect password attempts and triggers a security alert.
- **Duress Mode (SOS):** A hidden panic sequence triggers a silent alarm to the owner via Telegram without alerting the intruder.
- **Smart Sensor Logic:** Uses an IR sensor to prevent locking if the safe is empty.
- **Real-time Feedback:** The dashboard reflects the live status of the safe (Locked, Unlocked, Error).

## Tech Stack

### Hardware

- **Microcontroller:** Arduino Uno
- **Actuators:** Servo Motor
- **Sensors:** IR Sensor, 4x4 Matrix Keypad
- **Display:** LCD 16x2 with I2C module

### Backend

- **Runtime:** Node.js
- **Framework:** Express.js
- **Communication:** SerialPort (UART communication with Arduino)
- **Integrations:** Telegram Bot API

### Frontend

- **Framework:** React.js (Vite)
- **Styling:** CSS3 (Custom Dark UI)
- **HTTP Client:** Axios

## Installation and Setup

### 1. Hardware Configuration

1.  Connect the components to the Arduino board (Servo on Pin 3, IR on Pin 2).
2.  Upload the `.ino` file located in the `arduino` folder to the board.

### 2. Backend Setup

Navigate to the server directory:

```bash
cd backend
npm install
```

### 3. Create a .env file in the backend folder with the following variables:
```bash
PORT=3001
TELEGRAM_TOKEN=YOUR_TELEGRAM_BOT_TOKEN
MY_CHAT_ID=YOUR_CHAT_ID
SERIAL_PORT_PATH=COM3
```

### 4. Start the server
```bash
node server.js
```
### 5. Frontend Setup
```bash
cd frontend
npm install
```

### Start the application:
```bash
npm run dev
```

## Team
### Developed by:

1. Youssef Mohamed EL-Tabie Shetaia
2. Ammar Mohamed EL-Dessouqi EL-Qazaz
3. Mariam EL-Saied Gaber Kareem EL-Deen
