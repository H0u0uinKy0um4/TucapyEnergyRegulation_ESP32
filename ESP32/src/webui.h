#pragma once
#include <Arduino.h>

namespace WebUI {

String getDashboardHTML(float battery_P, float battery_I, float grid_I, float battery_soc, String status_msg, String initial_logs, String version) {
    // Escape logs for JS
    String escapedLogs = initial_logs;
    escapedLogs.replace("\n", "\\n");
    escapedLogs.replace("\"", "\\\"");

    String html = R"raw(
<!DOCTYPE html>
<html lang='cs'>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>Tucapy Energy Monitor</title>
    <style>
        :root {
            --bg: #0f172a;
            --card: rgba(30, 41, 59, 0.7);
            --primary: #38bdf8;
            --text: #f8fafc;
            --accent: #fbbf24;
            --success: #10b981;
            --error: #ef4444;
        }
        body {
            font-family: 'Segoe UI', system-ui, sans-serif;
            background: radial-gradient(circle at top right, #1e293b, #0f172a);
            color: var(--text);
            margin: 0;
            padding: 20px;
            display: flex;
            flex-direction: column;
            align-items: center;
            min-height: 100vh;
        }
        .container { width: 100%; max-width: 900px; display: flex; flex-direction: column; gap: 20px; }
        h1 { font-weight: 300; letter-spacing: 2px; margin-bottom: 30px; }
        
        .dashboard {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 15px;
        }
        .card {
            background: var(--card);
            backdrop-filter: blur(10px);
            border: 1px solid rgba(255,255,255,0.1);
            border-radius: 20px;
            padding: 25px;
            text-align: center;
            box-shadow: 0 10px 25px rgba(0,0,0,0.3);
            transition: 0.3s;
        }
        .card:hover { transform: translateY(-3px); border-color: var(--primary); }
        .card h3 { margin: 0; font-size: 13px; color: var(--primary); text-transform: uppercase; letter-spacing: 1px; }
        .card .value { font-size: 38px; font-weight: 700; margin: 15px 0; color: #fff; }
        .card .unit { font-size: 16px; opacity: 0.6; margin-left: 5px; }
        
        .status-msg {
            text-align: center;
            font-weight: 500;
            color: var(--accent);
            background: rgba(251, 191, 36, 0.1);
            padding: 10px;
            border-radius: 10px;
        }
        
        .console-container {
            background: #000;
            border-radius: 15px;
            overflow: hidden;
            box-shadow: 0 20px 50px rgba(0,0,0,0.5);
            border: 1px solid #334155;
        }
        .console-header {
            background: #1e293b;
            padding: 10px 20px;
            display: flex;
            justify-content: space-between;
            align-items: center;
            font-size: 12px;
            font-weight: 600;
            color: #94a3b8;
        }
        #console {
            height: 300px;
            padding: 20px;
            margin: 0;
            overflow-y: auto;
            font-family: 'Consolas', 'Monaco', monospace;
            font-size: 13px;
            line-height: 1.6;
            color: var(--success);
            white-space: pre-wrap;
        }
        #ws-status { padding: 4px 8px; border-radius: 4px; font-size: 10px; }
        .status-online { background: rgba(16, 185, 129, 0.2); color: var(--success); }
        .status-offline { background: rgba(239, 68, 68, 0.2); color: var(--error); }
        .version { position: absolute; top: 20px; right: 20px; font-size: 10px; opacity: 0.4; pointer-events: none; }
    </style>
</head>
<body>
    <div class='version'>)raw" + version + R"raw(</div>
    <div class='container'>
        <h1>Tucapy Energy</h1>
        <div class='dashboard'>
            <div class='card'><h3>Battery Power</h3><div class='value'>)raw" + String(battery_P, 2) + R"raw(<span class='unit'>kW</span></div></div>
            <div class='card'><h3>Battery Current</h3><div class='value'>)raw" + String(battery_I, 1) + R"raw(<span class='unit'>A</span></div></div>
            <div class='card'><h3>Grid Current</h3><div class='value'>)raw" + String(grid_I, 1) + R"raw(<span class='unit'>A</span></div></div>
            <div class='card'><h3>SOC</h3><div class='value'>)raw" + String(battery_soc, 0) + R"raw(<span class='unit'>%</span></div></div>
        </div>
        
        <div class='status-msg'>)raw" + status_msg + R"raw(</div>

        <div class='console-container'>
            <div class='console-header'>
                <span>WEBSOCKET CONSOLE</span>
                <span id='ws-status' class='status-offline'>OFFLINE</span>
            </div>
            <pre id='console'></pre>
        </div>
    </div>

    <script>
        const consoleEl = document.getElementById('console');
        const statusEl = document.getElementById('ws-status');
        
        // Load initial logs
        consoleEl.textContent = ")raw" + escapedLogs + R"raw(";
        consoleEl.scrollTop = consoleEl.scrollHeight;

        function connect() {
            const socket = new WebSocket('ws://' + window.location.hostname + ':81/');
            
            socket.onopen = () => {
                statusEl.innerText = 'ONLINE';
                statusEl.className = 'status-online';
                console.log('WS Connected');
            };

            socket.onclose = () => {
                statusEl.innerText = 'OFFLINE';
                statusEl.className = 'status-offline';
                setTimeout(connect, 2000); // Auto reconnect
            };

            socket.onmessage = (event) => {
                consoleEl.textContent += event.data + '\n';
                consoleEl.scrollTop = consoleEl.scrollHeight;
            };
            
            socket.onerror = (err) => {
                console.error('WS Error:', err);
            };
        }

        connect();
        setInterval(() => { if(statusEl.innerText === 'OFFLINE') location.reload(); }, 30000);
    </script>
</body>
</html>
)raw";
    return html;
}

} // namespace WebUI
