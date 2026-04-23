const express = require('express');
const mongoose = require('mongoose');
const cors = require('cors');
const axios = require('axios');

const app = express();
app.use(express.json());
app.use(cors());
app.use(express.static('.'));

// 1. Connect to Database
const uri = "mongodb+srv://huyquang_assignment:9TS4gBbogxoGVqQB@iotassignment.gjnib14.mongodb.net/MyIotProject";
mongoose.connect(uri).then(() => console.log("✅ MongoDB Connected"));

// 2. Schema to store data from CoreIOT
const CoreIOTDataSchema = new mongoose.Schema({
    deviceId: String,
    attributes: mongoose.Schema.Types.Mixed,
    telemetry: mongoose.Schema.Types.Mixed,
    timestamp: { type: Date, default: Date.now }
});
const CoreIOTData = mongoose.model('CoreIOTData', CoreIOTDataSchema);

// 3. CoreIOT Configuration
const COREIOT_DEVICE_ACCESS_TOKEN = "Fbc0XVl2O5DxOLcFq45a";
const COREIOT_API_V1 = "https://app.coreiot.io/api/v1";

// Configuration to PULL data via JWT
const COREIOT_BASE_URL = "https://app.coreiot.io";
const COREIOT_DEVICE_ID = "82ceca10-3df8-11f1-9981-cffbb69f5b14";
const COREIOT_JWT = "Bearer eyJhbGciOiJIUzUxMiJ9.eyJzdWIiOiJxdWFuZy52b2h1eWt0bXRAaGNtdXQuZWR1LnZuIiwidXNlcklkIjoiMWNkNzRhZjAtMzMxNC0xMWYxLTk5ODEtY2ZmYmI2OWY1YjE0Iiwic2NvcGVzIjpbIlRFTkFOVF9BRE1JTiJdLCJzZXNzaW9uSWQiOiI2YjcyZDY1YS02MzkzLTQ4OGUtYjBjNi1jYjJlMmZlODk2NDAiLCJleHAiOjE3NzY5NDkwNjksImlzcyI6ImNvcmVpb3QuaW8iLCJpYXQiOjE3NzY5NDAwNjksImZpcnN0TmFtZSI6IlFVQU5HIiwibGFzdE5hbWUiOiJWw5UgSFVZIiwiZW5hYmxlZCI6dHJ1ZSwiaXNQdWJsaWMiOmZhbHNlLCJ0ZW5hbnRJZCI6IjFjYjIzNmMwLTMzMTQtMTFmMS05OTgxLWNmZmJiNjlmNWIxNCIsImN1c3RvbWVySWQiOiIxMzgxNDAwMC0xZGQyLTExYjItODA4MC04MDgwODA4MDgwODAifQ.gZTiXjqd3HMaTtS3DbifMzKnMTHYBYi1bVrqy1fQLrwroma5OricOnukgFGxluxJ4V0-3R_rpR501U4ao_jwYA";

// 4. API Endpoints

// Send Telemetry (all 3 parameters) to CoreIOT
app.post('/api/send-telemetry', async (req, res) => {
    try {
        // Get data from request body or use default data if called empty
        const payload = req.body && Object.keys(req.body).length > 0 ? req.body : {
            temperature: 32.54,
            humidity: 57.47,
            anomaly_score: 0.34
        };
        
        const response = await axios.post(`${COREIOT_API_V1}/${COREIOT_DEVICE_ACCESS_TOKEN}/telemetry`, payload);
        res.json({ success: true, message: "Gửi telemetry thành công!", data: response.data });
    } catch (error) {
        res.status(500).json({ error: error.message });
    }
});

// Fetch data from CoreIOT and save to MongoDB
app.get('/api/sync-coreiot', async (req, res) => {
    try {
        const headers = {
            'X-Authorization': COREIOT_JWT,
            'Content-Type': 'application/json'
        };

        // Get latest Telemetry from CoreIOT
        // If there are more parameters, add them to the keys variable (comma separated)
        const keys = "temperature,humidity,anomaly_score";
        const telemetryRes = await axios.get(
            `${COREIOT_BASE_URL}/api/plugins/telemetry/DEVICE/${COREIOT_DEVICE_ID}/values/timeseries?keys=${keys}`, 
            { headers }
        );
        
        // Get Attributes from CoreIOT (optional)
        const attributesRes = await axios.get(
            `${COREIOT_BASE_URL}/api/plugins/telemetry/DEVICE/${COREIOT_DEVICE_ID}/values/attributes`, 
            { headers }
        );

        // CoreIOT/Thingsboard returns a complex format, we need to map it for easier DB storage
        // CoreIOT return format: { temperature: [{ value: '25', ts: 123... }] }
        const parsedTelemetry = {};
        if (telemetryRes.data) {
            for (const key in telemetryRes.data) {
                if (telemetryRes.data[key].length > 0) {
                    parsedTelemetry[key] = parseFloat(telemetryRes.data[key][0].value);
                }
            }
        }

        // CoreIOT attributes usually return an array of objects: [{ key: 'active', value: true }]
        const parsedAttributes = {};
        if (attributesRes.data && Array.isArray(attributesRes.data)) {
            attributesRes.data.forEach(attr => {
                parsedAttributes[attr.key] = attr.value;
            });
        }

        const data = new CoreIOTData({
            deviceId: COREIOT_DEVICE_ID,
            attributes: parsedAttributes,
            telemetry: parsedTelemetry
        });
        
        await data.save();
        res.json({ 
            success: true, 
            telemetry: parsedTelemetry,
            attributes: parsedAttributes 
        });
    } catch (error) {
        console.error("Lỗi khi Sync từ CoreIOT:", error.response ? error.response.data : error.message);
        res.status(500).json({ 
            error: error.message,
            details: error.response ? error.response.data : null 
        });
    }
});

// Get historical data from MongoDB
app.get('/api/data-history', async (req, res) => {
    try {
        const data = await CoreIOTData.find().sort({ timestamp: -1 }).limit(50);
        res.json(data);
    } catch (error) {
        res.status(500).json({ error: error.message });
    }
});

// Get latest data
app.get('/api/latest-data', async (req, res) => {
    try {
        const data = await CoreIOTData.findOne().sort({ timestamp: -1 });
        res.json(data);
    } catch (error) {
        res.status(500).json({ error: error.message });
    }
});

app.listen(3000, () => console.log("🚀 Server running on http://localhost:3000"));