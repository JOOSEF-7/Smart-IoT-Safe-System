require("dotenv").config();
const express = require("express");
const { SerialPort } = require("serialport");
const { ReadlineParser } = require("@serialport/parser-readline");
const TelegramBot = require("node-telegram-bot-api");
const cors = require("cors");
const http = require("http");

const PORT = process.env.PORT || 3001;
const SERIAL_PORT_PATH = process.env.SERIAL_PORT_PATH || "COM3";
const BAUD_RATE = 9600;
const TELEGRAM_TOKEN = process.env.TELEGRAM_TOKEN;
const MY_CHAT_ID = process.env.MY_CHAT_ID;

const app = express();
app.use(cors());
app.use(express.json());

const server = http.createServer(app);
const bot = new TelegramBot(TELEGRAM_TOKEN, { polling: true });

const port = new SerialPort({ path: SERIAL_PORT_PATH, baudRate: BAUD_RATE });
const parser = port.pipe(new ReadlineParser({ delimiter: "\r\n" }));

let lastArduinoResponse = "";

parser.on("data", (data) => {
  const cleanData = data.trim();
  console.log("Arduino:", cleanData);
  lastArduinoResponse = cleanData;

  if (cleanData.includes("REQ_OTP")) {
    const allowedDigits = "0123456789";
    let otp = "";
    for (let i = 0; i < 6; i++) {
      otp += allowedDigits.charAt(
        Math.floor(Math.random() * allowedDigits.length),
      );
    }
    console.log(`Sending Smart OTP: ${otp}`);
    bot.sendMessage(MY_CHAT_ID, ` OTP Code Request: ${otp}`);
    port.write(`OTP:${otp}\n`);
  } else if (cleanData.includes("ALERT_INTRUDER")) {
    bot.sendMessage(
      MY_CHAT_ID,
      `SECURITY ALERT! Someone entered wrong password 3 times! Safe is LOCKED OUT.`,
    );
  } else if (cleanData.includes("ALERT_DURESS")) {
    bot.sendMessage(
      MY_CHAT_ID,
      `Danger ALERT! User is under DURESS and forced to open the safe!`,
    );
  }
});

app.post("/api/open", (req, res) => {
  const { password } = req.body;

  if (!password) {
    return res
      .status(400)
      .json({ status: "error", message: "Password Required" });
  }

  lastArduinoResponse = "";

  port.write(`CMD_OPEN:${password}\n`);

  setTimeout(() => {
    if (lastArduinoResponse.includes("ERR_LOCKED_OUT")) {
      res.json({
        status: "error",
        message: "System is in Lockdown! Wait timer.",
      });
    } else if (lastArduinoResponse.includes("ERR_WRONG_PASS")) {
      res.json({ status: "error", message: "Wrong Password!" });
    } else {
      res.json({ status: "success", message: "Request Sent. Check Safe." });
    }
  }, 500);
});

app.post("/api/lock", (req, res) => {
  lastArduinoResponse = "";
  port.write("CMD_LOCK\n");

  setTimeout(() => {
    if (lastArduinoResponse.includes("ERR_EMPTY")) {
      res.json({ status: "error", message: "Safe is Empty! Cannot Lock." });
    } else if (lastArduinoResponse.includes("ERR_NO_PASS")) {
      res.json({ status: "error", message: "Setup Password on Keypad First!" });
    } else {
      res.json({ status: "success", message: "Lock Command Sent" });
    }
  }, 500);
});

server.listen(PORT, () => {
  console.log(`Server running on port ${PORT}`);
});
