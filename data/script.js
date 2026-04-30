// ==================== WEBSOCKET ====================
var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
// Khai báo biến gauge toàn cục để có thể truy cập từ onMessage
var gaugeTemp, gaugeHumi;

window.addEventListener('load', onLoad);

function onLoad(event) {
    initWebSocket();
    initGauges(); // Khởi tạo gauge khi trang tải xong
}

function onOpen(event) {
    console.log('Connection opened');
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

function initWebSocket() {
    console.log('Trying to open a WebSocket connection…');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

function Send_Data(data) {
    if (websocket && websocket.readyState === WebSocket.OPEN) {
        websocket.send(data);
        console.log("📤 Gửi:", data);
    } else {
        console.warn("⚠️ WebSocket chưa sẵn sàng!");
        alert("⚠️ WebSocket chưa kết nối!");
    }
}

function onMessage(event) {
    console.log("📩 Nhận:", event.data);
    try {
        const data = JSON.parse(event.data);
        // Kiểm tra nếu đây là tin nhắn cập nhật cảm biến
        if (data.type === 'update') {
            // Cập nhật gauge với dữ liệu thật từ ESP32
            if (gaugeTemp) gaugeTemp.refresh(data.temp);
            if (gaugeHumi) gaugeHumi.refresh(data.humi);
            
            const statusEl = document.getElementById('systemStatus');
            if (statusEl) {
                statusEl.innerText = data.status;
                statusEl.className = 'status-badge ' + data.status.toLowerCase();
            }
        }
        // Xử lý các loại tin nhắn khác nếu cần
    } catch (e) {
        console.warn("Không phải JSON hợp lệ:", event.data);
    }
}


// ==================== UI NAVIGATION ====================
let relayList = [];
let deleteTarget = null;

function showSection(id, event) {
    document.querySelectorAll('.section').forEach(sec => sec.style.display = 'none');
    document.getElementById(id).style.display = id === 'settings' ? 'flex' : 'block';
    document.querySelectorAll('.nav-item').forEach(i => i.classList.remove('active'));
    event.currentTarget.classList.add('active');
}


// ==================== HOME GAUGES ====================
function initGauges() {
    gaugeTemp = new JustGage({
        id: "gauge_temp",
        value: 0, // Bắt đầu từ 0
        min: -10,
        max: 50,
        donut: true,
        pointer: false,
        gaugeWidthScale: 0.25,
        gaugeColor: "transparent",
        levelColorsGradient: true,
        levelColors: ["#00BCD4", "#4CAF50", "#FFC107", "#F44336"]
    });

    gaugeHumi = new JustGage({
        id: "gauge_humi",
        value: 0, // Bắt đầu từ 0
        min: 0,
        max: 100,
        donut: true,
        pointer: false,
        gaugeWidthScale: 0.25,
        gaugeColor: "transparent",
        levelColorsGradient: true,
        levelColors: ["#42A5F5", "#00BCD4", "#0288D1"]
    });

    // Đã xóa `setInterval` tạo dữ liệu ngẫu nhiên.
    // Gauge giờ sẽ được cập nhật bởi tin nhắn WebSocket.
}


// ==================== DEVICE FUNCTIONS ====================
let ledStates = [false, false, false, false];

function toggleLED(id) {
    ledStates[id] = !ledStates[id];
    const btn = document.getElementById(`btn-led-${id}`);
    
    if (ledStates[id]) {
        btn.innerText = "ON";
        btn.classList.add("on");
    } else {
        btn.innerText = "OFF";
        btn.classList.remove("on");
    }

    const payload = JSON.stringify({
        page: "device",
        value: {
            id: id,
            status: ledStates[id] ? "ON" : "OFF"
        }
    });
    Send_Data(payload);
}

function changePWM(id, value) {
    const payload = JSON.stringify({
        page: "device",
        value: {
            id: id,
            pwm: parseInt(value)
        }
    });
    Send_Data(payload);
}


// ==================== SETTINGS FORM (BỔ SUNG) ====================
document.getElementById("settingsForm").addEventListener("submit", function (e) {
    e.preventDefault();

    const ssid = document.getElementById("ssid").value.trim();
    const password = document.getElementById("password").value.trim();
    const token = document.getElementById("token").value.trim();
    const server = document.getElementById("server").value.trim();
    const port = document.getElementById("port").value.trim();

    const settingsJSON = JSON.stringify({
        page: "setting",
        value: {
            ssid: ssid,
            password: password,
            token: token,
            server: server,
            port: port
        }
    });

    Send_Data(settingsJSON);
    alert("✅ Cấu hình đã được gửi đến thiết bị!");
});
