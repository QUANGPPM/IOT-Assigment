// ══════════════════════════════════════════════════════════
// script.js — IoT Base Station Local Web UI
// ══════════════════════════════════════════════════════════

// ─── WebSocket ────────────────────────────────────────────
var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
var gaugeTemp, gaugeHumi;

window.addEventListener('load', () => {
    initWebSocket();

    // ⚠️ Đăng ký form listeners TRƯỚC initGauges
    // để tránh lỗi thư viện (JustGage/Raphael) làm hỏng form handler
    initSettingsForm();
    initThresholdsForm();

    try {
        initGauges();
    } catch (e) {
        console.warn('[Gauges] Không thể khởi tạo JustGage:', e);
    }
});

function initWebSocket() {
    console.log('[WS] Connecting…');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage;
}

function onOpen() {
    console.log('[WS] Connected');
    setWsStatus(true);
}

function onClose() {
    console.log('[WS] Disconnected — retrying in 2s');
    setWsStatus(false);
    setTimeout(initWebSocket, 2000);
}

function setWsStatus(connected) {
    const el = document.getElementById('wsStatus');
    if (!el) return;
    el.innerHTML = connected
        ? '<span class="dot connected"></span> Đã kết nối'
        : '<span class="dot disconnected"></span> Mất kết nối...';
}

function sendData(data) {
    if (websocket && websocket.readyState === WebSocket.OPEN) {
        websocket.send(data);
        console.log('[WS] Sent:', data);
    } else {
        console.warn('[WS] Not ready');
        alert('⚠️ WebSocket chưa kết nối!');
    }
}

// ─── Message Handler ─────────────────────────────────────
function onMessage(event) {
    console.log('[WS] Recv:', event.data);
    try {
        const data = JSON.parse(event.data);

        // Cập nhật giá trị cảm biến realtime
        if (data.type === 'update') {
            if (gaugeTemp) gaugeTemp.refresh(data.temp);
            if (gaugeHumi) gaugeHumi.refresh(data.humi);

            // Status badge
            const statusEl = document.getElementById('systemStatus');
            if (statusEl) {
                statusEl.innerText = data.status;
                statusEl.className = 'status-badge ' + data.status.toLowerCase();
            }

            // Anomaly score
            const scoreEl = document.getElementById('anomalyScore');
            if (scoreEl && data.score !== undefined) {
                scoreEl.innerText = parseFloat(data.score).toFixed(2);
            }
        }

        // Phản hồi từ lệnh thresholds
        if (data.status === 'ok' && data.page === 'thresholds') {
            showFeedback('thr-feedback', '✅ Ngưỡng đã được lưu vào Flash!', 'ok');
        }

        // Phản hồi diagnostic
        if (data.status === 'ok' && data.page === 'diagnostic_led') {
            document.getElementById('led-result').innerText = '✅ Lệnh đã được gửi đến ESP32';
        }
        if (data.status === 'ok' && data.page === 'diagnostic_neo') {
            document.getElementById('neo-result').innerText = '✅ Lệnh màu đã được gửi đến ESP32';
        }

        // Phản hồi lỗi
        if (data.status === 'error') {
            showFeedback('thr-feedback', `❌ Lỗi: ${data.reason || 'unknown'}`, 'err');
        }

    } catch (e) {
        console.warn('[WS] Non-JSON message:', event.data);
    }
}

// ─── UI Navigation ────────────────────────────────────────
function showSection(id, navEl) {
    document.querySelectorAll('.section').forEach(s => {
        s.style.display = 'none';
        s.classList.remove('fade-in');
    });
    const target = document.getElementById(id);
    if (target) {
        target.style.display = (id === 'settings') ? 'flex' : 'block';
        // Trigger reflow for animation
        void target.offsetWidth;
        target.classList.add('fade-in');
    }
    document.querySelectorAll('.nav-item').forEach(i => i.classList.remove('active'));
    if (navEl) navEl.classList.add('active');
}

// ─── HOME Gauges ─────────────────────────────────────────
function initGauges() {
    gaugeTemp = new JustGage({
        id: 'gauge_temp',
        value: 0, min: -10, max: 50,
        donut: true, pointer: false,
        gaugeWidthScale: 0.25,
        gaugeColor: 'transparent',
        levelColorsGradient: true,
        levelColors: ['#00BCD4', '#4CAF50', '#FFC107', '#F44336'],
        label: '°C',
        valueFontColor: '#e6edf3',
        labelFontColor: '#7d8590'
    });
    gaugeHumi = new JustGage({
        id: 'gauge_humi',
        value: 0, min: 0, max: 100,
        donut: true, pointer: false,
        gaugeWidthScale: 0.25,
        gaugeColor: 'transparent',
        levelColorsGradient: true,
        levelColors: ['#FFD700', '#42A5F5', '#0288D1'],
        label: '%',
        valueFontColor: '#e6edf3',
        labelFontColor: '#7d8590'
    });
}

// ─── DIAGNOSTIC — LED Test ────────────────────────────────
function testLED(state) {
    const payload = JSON.stringify({
        page: 'diagnostic',
        value: { target: 'led', state: state }
    });
    sendData(payload);
    document.getElementById('led-result').innerText = `⏳ Đã gửi lệnh: LED ${state ? 'BẬT' : 'TẮT'}`;
}

// ─── DIAGNOSTIC — NeoPixel Test ──────────────────────────
function testNeo(color) {
    const payload = JSON.stringify({
        page: 'diagnostic',
        value: { target: 'neo', color: color }
    });
    sendData(payload);
    const labels = { red:'Đỏ', green:'Xanh lá', blue:'Xanh dương', white:'Trắng', off:'Tắt' };
    document.getElementById('neo-result').innerText = `⏳ Đã gửi lệnh: NeoPixel → ${labels[color] || color}`;
}

// ─── THRESHOLDS Form ─────────────────────────────────────
function initThresholdsForm() {
    document.getElementById('thresholdsForm').addEventListener('submit', saveThresholds);
}

function saveThresholds(e) {
    e.preventDefault();

    const led_warn   = parseFloat(document.getElementById('led_warn').value);
    const led_danger = parseFloat(document.getElementById('led_danger').value);
    const neo_dry    = parseFloat(document.getElementById('neo_dry').value);
    const neo_wet    = parseFloat(document.getElementById('neo_wet').value);
    const ano_warn   = parseFloat(document.getElementById('ano_warn').value);
    const ano_danger = parseFloat(document.getElementById('ano_danger').value);

    // Validate: warn phải nhỏ hơn danger
    if (led_warn >= led_danger) {
        showFeedback('thr-feedback', '❌ Ngưỡng LED Warning phải nhỏ hơn Danger!', 'err');
        return;
    }
    if (neo_dry >= neo_wet) {
        showFeedback('thr-feedback', '❌ Ngưỡng NeoPixel Dry phải nhỏ hơn Wet!', 'err');
        return;
    }
    if (ano_warn >= ano_danger) {
        showFeedback('thr-feedback', '❌ Ngưỡng Anomaly Warning phải nhỏ hơn Danger!', 'err');
        return;
    }

    const payload = JSON.stringify({
        page: 'thresholds',
        value: { led_warn, led_danger, neo_dry, neo_wet, ano_warn, ano_danger }
    });
    sendData(payload);
    showFeedback('thr-feedback', '⏳ Đang gửi cấu hình đến thiết bị...', 'ok');
}

function resetThresholds() {
    document.getElementById('led_warn').value   = 30;
    document.getElementById('led_danger').value = 35;
    document.getElementById('neo_dry').value    = 40;
    document.getElementById('neo_wet').value    = 70;
    document.getElementById('ano_warn').value   = 60;
    document.getElementById('ano_danger').value = 80;
    showFeedback('thr-feedback', 'ℹ️ Đã khôi phục giá trị mặc định. Nhấn "Lưu" để áp dụng.', 'ok');
}

// ─── SETTINGS Form ───────────────────────────────────────
function initSettingsForm() {
    document.getElementById('settingsForm').addEventListener('submit', function(e) {
        e.preventDefault();
        const payload = JSON.stringify({
            page: 'setting',
            value: {
                ssid:     document.getElementById('ssid').value.trim(),
                password: document.getElementById('password').value.trim(),
                token:    document.getElementById('token').value.trim(),
                server:   document.getElementById('server').value.trim(),
                port:     document.getElementById('port').value.trim()
            }
        });
        sendData(payload);
        alert('✅ Cấu hình đã gửi. Thiết bị sẽ khởi động lại...');
    });
}

// ─── Helper: feedback message ────────────────────────────
function showFeedback(elId, msg, type) {
    const el = document.getElementById(elId);
    if (!el) return;
    el.innerText = msg;
    el.className = 'form-feedback ' + type;
    clearTimeout(el._timer);
    el._timer = setTimeout(() => { el.className = 'form-feedback'; }, 5000);
}
