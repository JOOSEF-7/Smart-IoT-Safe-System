// App.js
import React, { useState } from "react";
import axios from "axios";
import "./App.css";

function App() {
  const [msg, setMsg] = useState("Ready");
  const [statusType, setStatusType] = useState("");
  const [password, setPassword] = useState("");

  const handleOpen = async () => {
    if (!password) {
      setMsg("Please enter the password first!");
      setStatusType("error");
      return;
    }

    try {
      setMsg("Sending Request...");
      setStatusType("loading");
      const res = await axios.post("http://localhost:3001/api/open", {
        password,
      });

      if (res.data.status === "error") {
        setMsg(`${res.data.message}`);
        setStatusType("error");
      } else {
        setMsg("Unlocked Successfully");
        setStatusType("success");
        setPassword("");
      }
    } catch (err) {
      setMsg("❌ Connection Error");
      setStatusType("error");
    }
  };

  const handleLock = async () => {
    try {
      setMsg("Locking...");
      setStatusType("loading");
      const res = await axios.post("http://localhost:3001/api/lock");

      if (res.data.status === "error") {
        setMsg(` ${res.data.message}`);
        setStatusType("error");
      } else {
        setMsg(" Locked Successfully");
        setStatusType("success");
      }
    } catch (err) {
      setMsg(" Connection Error");
      setStatusType("error");
    }
  };

  return (
    <div className="App">
      <div className="overlay">
        <div className="safe-card">
          <div className="header">
            <span className="icon">🛡️</span>
            <h1>Secure Safe System</h1>
          </div>

          <div className={`status-display ${statusType}`}>{msg}</div>

          <div className="control-panel">
            <div className="input-group">
              <label>Safe Password</label>
              <input
                type="password"
                placeholder="Enter pin to unlock..."
                value={password}
                onChange={(e) => setPassword(e.target.value)}
                maxLength={10}
              />
            </div>

            <div className="actions">
              <button className="btn btn-open" onClick={handleOpen}>
                OPEN SAFE
              </button>
              <button className="btn btn-lock" onClick={handleLock}>
                LOCK SAFE
              </button>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}

export default App;
