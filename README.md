# CoreIOT Dashboard Project

This is a small Web App project (including both Frontend and a Node.js Backend) used to connect, synchronize, and visualize Telemetry data and device Attributes from the **CoreIOT** platform (which is developed based on ThingsBoard).

## Key Features
- **Secure Data Synchronization:** Uses a Node.js Backend as an intermediary (Proxy) to call REST APIs to CoreIOT via a JWT Token, protecting the Token from being exposed on the user's browser.
- **Historical Storage (Database):** Parses CoreIOT's complex data format and saves data records directly into a **MongoDB** database via Mongoose.
- **Intuitive Dashboard UI:** Displays data as colorful Metric Cards for Sensors (Temperature, Humidity, Anomaly Score) and Network Information (Local IP, MAC Address, Active Status).
- **Auto-Update (Real-time Simulation):** The Frontend automatically calls the sync API every 10 seconds to fetch the latest data without requiring a page refresh (F5).
- **History Table:** Displays a summary of the 50 most recent records.

## Directory Structure
- `server.js`: The Backend Node.js file. Contains the Express Server logic, Mongoose schema setup, and handles `axios` functions to fetch data from CoreIOT.
- `index.html`: The User Interface (Frontend). Consists of static HTML/CSS and JavaScript that executes the logic to fetch data from the Backend and render it to the DOM.
- `package.json` / `package-lock.json`: Contains dependency declarations for npm libraries (`express`, `mongoose`, `cors`, `axios`).

## Installation & Running Guide

### 1. Install Dependencies
Ensure you have Node.js installed on your machine. Open the terminal at the project directory and run:
```bash
npm install
```

### 2. Configuration Setup
Before running the application, you need to review the `server.js` file and ensure the following parameters are correct:
- `uri`: The MongoDB connection string.
- `COREIOT_DEVICE_ID`: Your device ID (UUID) on CoreIOT.
- `COREIOT_JWT`: The JWT Token used for authentication.
  
**⚠️ IMPORTANT REGARDING THE TOKEN:**
By default, the JWT Token of the CoreIOT/ThingsBoard platform has an expiration time (usually 2.5 hours). If the server console logs a `Token has expired` or `401 Unauthorized` error, you must log in to CoreIOT on your browser again, retrieve the newly generated Token string, and update the `COREIOT_JWT` variable in `server.js`.

### 3. Run the Server
Start the Backend server by running the following command:
```bash
node server.js
```
The console will display:
```
🚀 Server running on http://localhost:3000
✅ MongoDB Connected
```

### 4. Access the Dashboard
Since the Backend is configured to serve static files (`express.static('.')`), you simply need to open your browser and navigate to:
👉 **[http://localhost:3000/index.html](http://localhost:3000/index.html)**
