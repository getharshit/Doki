# Doki OS - HTTP REST API Reference

Complete reference for interacting with Doki OS over HTTP from web apps, mobile apps, or command-line tools.

---

## Table of Contents

1. [Getting Started](#getting-started)
2. [App Management](#app-management)
3. [Display Control](#display-control)
4. [Media Upload](#media-upload)
5. [Animation Upload](#animation-upload)
6. [Custom JavaScript](#custom-javascript)
7. [Dashboard](#dashboard)
8. [Complete Examples](#complete-examples)

---

## Getting Started

### Base URL

```
http://<ESP32_IP_ADDRESS>
```

**Finding Your ESP32's IP Address:**
1. Check Serial Monitor after boot
2. Look for: `[SimpleHTTP] ✓ Server started at http://192.168.x.x`

**Example:**
```
http://192.168.1.100
http://10.104.170.213
```

### CORS

All endpoints support CORS with:
```
Access-Control-Allow-Origin: *
```

This allows requests from any web page.

### Authentication

**Current version:** No authentication required

**Production:** Add API key authentication for security

---

## App Management

### Get Available Apps

**Endpoint:** `GET /api/apps`

Get list of all registered apps.

**Request:**
```bash
curl http://192.168.1.100/api/apps
```

**Response:**
```json
{
  "apps": [
    {
      "id": "clock",
      "name": "Clock",
      "description": "Digital clock with NTP sync"
    },
    {
      "id": "weather",
      "name": "Weather",
      "description": "Live weather display"
    },
    {
      "id": "animation_chain",
      "name": "Animation Chain",
      "description": "Demo of sequential animation playback"
    },
    {
      "id": "stress_test",
      "name": "Stress Test",
      "description": "Animation system stress testing"
    }
  ]
}
```

**JavaScript Example:**
```javascript
async function getApps() {
    const response = await fetch('http://192.168.1.100/api/apps');
    const data = await response.json();

    console.log('Available apps:', data.apps);

    data.apps.forEach(app => {
        console.log(`${app.name} (${app.id}): ${app.description}`);
    });
}
```

---

### Load App on Display

**Endpoint:** `POST /api/load`

Load an app on a specific display.

**Request Body:**
```json
{
  "displayId": 0,
  "appId": "clock"
}
```

**Parameters:**
- `displayId` (number): Display ID (0, 1, or 2)
- `appId` (string): App ID from `/api/apps`

**Request:**
```bash
curl -X POST http://192.168.1.100/api/load \
  -H "Content-Type: application/json" \
  -d '{"displayId":0,"appId":"clock"}'
```

**Response (Success):**
```json
{
  "success": true,
  "message": "App loaded successfully",
  "displayId": 0,
  "appId": "clock"
}
```

**Response (Error):**
```json
{
  "success": false,
  "message": "Invalid app ID"
}
```

**JavaScript Example:**
```javascript
async function loadApp(displayId, appId) {
    const response = await fetch('http://192.168.1.100/api/load', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify({
            displayId: displayId,
            appId: appId
        })
    });

    const result = await response.json();

    if (result.success) {
        console.log(`Loaded ${appId} on display ${displayId}`);
    } else {
        console.error('Failed to load app:', result.message);
    }
}

// Usage
loadApp(0, 'clock');          // Load clock on display 0
loadApp(1, 'weather');        // Load weather on display 1
```

**Python Example:**
```python
import requests

def load_app(esp32_ip, display_id, app_id):
    url = f"http://{esp32_ip}/api/load"
    data = {
        "displayId": display_id,
        "appId": app_id
    }

    response = requests.post(url, json=data)
    result = response.json()

    if result['success']:
        print(f"Loaded {app_id} on display {display_id}")
    else:
        print(f"Error: {result['message']}")

# Usage
load_app("192.168.1.100", 0, "clock")
```

---

## Display Control

### Get System Status

**Endpoint:** `GET /api/status`

Get status of all displays and system info.

**Request:**
```bash
curl http://192.168.1.100/api/status
```

**Response:**
```json
{
  "displays": [
    {
      "id": 0,
      "currentApp": "clock",
      "appName": "Clock"
    },
    {
      "id": 1,
      "currentApp": "weather",
      "appName": "Weather"
    }
  ],
  "system": {
    "freeHeap": 156432,
    "uptime": 3600,
    "wifi": {
      "connected": true,
      "ssid": "MyNetwork",
      "ip": "192.168.1.100",
      "rssi": -45
    }
  }
}
```

**JavaScript Example:**
```javascript
async function getStatus() {
    const response = await fetch('http://192.168.1.100/api/status');
    const status = await response.json();

    console.log('System Info:');
    console.log(`Free Heap: ${status.system.freeHeap} bytes`);
    console.log(`Uptime: ${status.system.uptime} seconds`);
    console.log(`WiFi: ${status.system.wifi.ssid} (${status.system.wifi.rssi} dBm)`);

    console.log('\nDisplays:');
    status.displays.forEach(display => {
        console.log(`Display ${display.id}: ${display.appName}`);
    });
}
```

---

## Media Upload

### Upload Image/GIF

**Endpoint:** `POST /api/media/upload`

Upload image (PNG, JPG, GIF) to SPIFFS.

**Content-Type:** `multipart/form-data`

**Request (curl):**
```bash
curl -X POST \
  -F "file=@myimage.png" \
  http://192.168.1.100/api/media/upload
```

**Response:**
```json
{
  "success": true,
  "message": "Media uploaded",
  "filename": "myimage.png",
  "size": 45678,
  "path": "/media/myimage.png"
}
```

**JavaScript Example:**
```html
<!DOCTYPE html>
<html>
<body>
    <input type="file" id="fileInput" accept="image/*">
    <button onclick="uploadMedia()">Upload</button>

    <script>
        async function uploadMedia() {
            const fileInput = document.getElementById('fileInput');
            const file = fileInput.files[0];

            if (!file) {
                alert('Please select a file');
                return;
            }

            const formData = new FormData();
            formData.append('file', file);

            try {
                const response = await fetch('http://192.168.1.100/api/media/upload', {
                    method: 'POST',
                    body: formData
                });

                const result = await response.json();

                if (result.success) {
                    alert(`Uploaded! Saved to: ${result.path}`);
                } else {
                    alert(`Error: ${result.message}`);
                }
            } catch (error) {
                alert('Upload failed: ' + error);
            }
        }
    </script>
</body>
</html>
```

**Python Example:**
```python
import requests

def upload_media(esp32_ip, file_path):
    url = f"http://{esp32_ip}/api/media/upload"

    with open(file_path, 'rb') as f:
        files = {'file': (file_path, f, 'image/png')}
        response = requests.post(url, files=files)

    result = response.json()

    if result['success']:
        print(f"Uploaded: {result['path']} ({result['size']} bytes)")
    else:
        print(f"Error: {result['message']}")

# Usage
upload_media("192.168.1.100", "photo.png")
```

---

### Get Media Info

**Endpoint:** `GET /api/media/info`

List all uploaded media files.

**Request:**
```bash
curl http://192.168.1.100/api/media/info
```

**Response:**
```json
{
  "files": [
    {
      "name": "photo.png",
      "size": 45678,
      "path": "/media/photo.png"
    },
    {
      "name": "animation.gif",
      "size": 123456,
      "path": "/media/animation.gif"
    }
  ],
  "totalSize": 169134,
  "count": 2
}
```

---

### Delete Media

**Endpoint:** `DELETE /api/media/delete`

Delete a media file.

**Query Parameters:**
- `filename` (string): Filename to delete

**Request:**
```bash
curl -X DELETE "http://192.168.1.100/api/media/delete?filename=photo.png"
```

**Response:**
```json
{
  "success": true,
  "message": "File deleted",
  "filename": "photo.png"
}
```

**JavaScript Example:**
```javascript
async function deleteMedia(filename) {
    const url = `http://192.168.1.100/api/media/delete?filename=${encodeURIComponent(filename)}`;

    const response = await fetch(url, {
        method: 'DELETE'
    });

    const result = await response.json();

    if (result.success) {
        console.log(`Deleted: ${filename}`);
    } else {
        console.error(`Error: ${result.message}`);
    }
}
```

---

## Animation Upload

### Upload Animation Sprite

**Endpoint:** `POST /api/animations/upload`

Upload `.spr` animation sprite file.

**Content-Type:** `multipart/form-data`

**File Requirements:**
- Format: `.spr` (Doki OS sprite format)
- Max size: 1 MB
- Magic number: `0x444F4B49` ("DOKI")

**Request (curl):**
```bash
curl -X POST \
  -F "file=@spinner.spr" \
  http://192.168.1.100/api/animations/upload
```

**Response:**
```json
{
  "success": true,
  "message": "Animation uploaded"
}
```

**Validation:**
- File is validated for correct `.spr` format
- Saved to `/animations/<filename>`
- Immediately available for use in apps

**JavaScript Example:**
```html
<!DOCTYPE html>
<html>
<body>
    <h2>Upload Animation</h2>
    <input type="file" id="animFile" accept=".spr">
    <button onclick="uploadAnimation()">Upload</button>

    <script>
        async function uploadAnimation() {
            const fileInput = document.getElementById('animFile');
            const file = fileInput.files[0];

            if (!file) {
                alert('Select a .spr file');
                return;
            }

            if (!file.name.endsWith('.spr')) {
                alert('File must be .spr format');
                return;
            }

            const formData = new FormData();
            formData.append('file', file);

            try {
                const response = await fetch('http://192.168.1.100/api/animations/upload', {
                    method: 'POST',
                    body: formData
                });

                const result = await response.json();

                if (result.success) {
                    alert('Animation uploaded! Path: /animations/' + file.name);
                } else {
                    alert('Upload failed');
                }
            } catch (error) {
                alert('Error: ' + error);
            }
        }
    </script>
</body>
</html>
```

**Python Example:**
```python
import requests

def upload_animation(esp32_ip, spr_file_path):
    url = f"http://{esp32_ip}/api/animations/upload"

    with open(spr_file_path, 'rb') as f:
        files = {'file': (spr_file_path, f, 'application/octet-stream')}
        response = requests.post(url, files=files)

    result = response.json()

    if result['success']:
        print(f"Animation uploaded: /animations/{spr_file_path}")
    else:
        print("Upload failed")

# Usage
upload_animation("192.168.1.100", "spinner.spr")
```

**Serial Monitor Output:**
```
[SimpleHTTP] Starting animation upload: spinner.spr
[SimpleHTTP] Animation upload progress: 196608 bytes
[SimpleHTTP] Animation upload complete: 196608 bytes total
[SimpleHTTP] ✓ Valid sprite file detected
[SimpleHTTP] ✓ Animation saved: /animations/spinner.spr (196608 bytes)
```

**See Also:** [ANIMATION_UPLOAD_API.md](ANIMATION_UPLOAD_API.md) for more details on animation format and generation.

---

## Custom JavaScript

### Upload Custom JS App

**Endpoint:** `POST /api/upload-js`

Upload and run custom JavaScript code as an app.

**Content-Type:** `application/x-www-form-urlencoded`

**Request Parameters:**
- `code` (string): JavaScript source code (max 16 KB)

**Request (curl):**
```bash
curl -X POST http://192.168.1.100/api/upload-js \
  -d 'code=function onCreate() { log("Hello"); createLabel("Test", 120, 160); } function onUpdate() {}'
```

**Response:**
```json
{
  "success": true,
  "message": "Custom JS code uploaded",
  "app_id": "custom_js"
}
```

**JavaScript Example:**
```javascript
async function uploadCustomJS(code) {
    const formData = new FormData();
    formData.append('code', code);

    const response = await fetch('http://192.168.1.100/api/upload-js', {
        method: 'POST',
        body: formData
    });

    const result = await response.json();

    if (result.success) {
        console.log('Code uploaded! App ID:', result.app_id);

        // Now load it on display 0
        await loadApp(0, result.app_id);
    }
}

// Example: Upload simple clock
const clockCode = `
var timeLabel = -1;
var lastUpdate = 0;

function onCreate() {
    setBackgroundColor(0x000000);
    timeLabel = createLabel("--:--:--", 120, 160);
    setLabelSize(timeLabel, 24);
}

function onUpdate() {
    var now = millis();
    if (now - lastUpdate >= 1000) {
        lastUpdate = now;
        var timestamp = getTime();
        var date = new Date(timestamp * 1000);
        var timeStr = date.getHours() + ":" +
                      date.getMinutes() + ":" +
                      date.getSeconds();
        updateLabel(timeLabel, timeStr);
    }
}
`;

uploadCustomJS(clockCode);
```

**Web-Based Code Editor Example:**
```html
<!DOCTYPE html>
<html>
<body>
    <h2>Doki OS - Custom JS Editor</h2>

    <textarea id="codeEditor" rows="20" cols="80">
function onCreate() {
    setBackgroundColor(0x000000);
    createLabel("Hello from custom JS!", 120, 160);
}

function onUpdate() {
    // Your code here
}
    </textarea>

    <br>
    <button onclick="uploadAndRun()">Upload & Run</button>

    <script>
        async function uploadAndRun() {
            const code = document.getElementById('codeEditor').value;

            // Upload code
            const formData = new FormData();
            formData.append('code', code);

            const uploadResponse = await fetch('http://192.168.1.100/api/upload-js', {
                method: 'POST',
                body: formData
            });

            const uploadResult = await uploadResponse.json();

            if (!uploadResult.success) {
                alert('Upload failed');
                return;
            }

            // Load app on display 0
            const loadResponse = await fetch('http://192.168.1.100/api/load', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                    displayId: 0,
                    appId: uploadResult.app_id
                })
            });

            const loadResult = await loadResponse.json();

            if (loadResult.success) {
                alert('App is now running on display 0!');
            }
        }
    </script>
</body>
</html>
```

---

## Dashboard

### Web Dashboard

**Endpoint:** `GET /`

Web-based dashboard for managing Doki OS.

**URL:**
```
http://192.168.1.100/
```

**Features:**
- View available apps
- Load apps on displays
- View system status
- Upload media files
- Upload custom JavaScript

**Access:** Open in any web browser on the same network.

---

## Complete Examples

### Example 1: React App Controller

```javascript
import React, { useState, useEffect } from 'react';

const ESP32_IP = '192.168.1.100';

function DokiOSController() {
    const [apps, setApps] = useState([]);
    const [status, setStatus] = useState(null);

    useEffect(() => {
        loadApps();
        loadStatus();
    }, []);

    async function loadApps() {
        const response = await fetch(`http://${ESP32_IP}/api/apps`);
        const data = await response.json();
        setApps(data.apps);
    }

    async function loadStatus() {
        const response = await fetch(`http://${ESP32_IP}/api/status`);
        const data = await response.json();
        setStatus(data);
    }

    async function loadAppOnDisplay(displayId, appId) {
        const response = await fetch(`http://${ESP32_IP}/api/load`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ displayId, appId })
        });

        const result = await response.json();
        if (result.success) {
            loadStatus();  // Refresh status
        }
    }

    return (
        <div>
            <h1>Doki OS Controller</h1>

            <h2>System Status</h2>
            {status && (
                <div>
                    <p>Free Heap: {status.system.freeHeap} bytes</p>
                    <p>Uptime: {Math.floor(status.system.uptime / 60)} minutes</p>
                    <p>WiFi: {status.system.wifi.ssid} ({status.system.wifi.rssi} dBm)</p>
                </div>
            )}

            <h2>Displays</h2>
            {status && status.displays.map(display => (
                <div key={display.id}>
                    <h3>Display {display.id}</h3>
                    <p>Current App: {display.appName}</p>

                    <select onChange={(e) => loadAppOnDisplay(display.id, e.target.value)}>
                        <option value="">-- Select App --</option>
                        {apps.map(app => (
                            <option key={app.id} value={app.id}>{app.name}</option>
                        ))}
                    </select>
                </div>
            ))}
        </div>
    );
}

export default DokiOSController;
```

### Example 2: Python Dashboard Script

```python
#!/usr/bin/env python3
import requests
import json
from time import sleep

ESP32_IP = "192.168.1.100"
BASE_URL = f"http://{ESP32_IP}"

def get_apps():
    response = requests.get(f"{BASE_URL}/api/apps")
    return response.json()['apps']

def get_status():
    response = requests.get(f"{BASE_URL}/api/status")
    return response.json()

def load_app(display_id, app_id):
    data = {"displayId": display_id, "appId": app_id}
    response = requests.post(f"{BASE_URL}/api/load", json=data)
    return response.json()

def upload_animation(file_path):
    with open(file_path, 'rb') as f:
        files = {'file': f}
        response = requests.post(f"{BASE_URL}/api/animations/upload", files=files)
    return response.json()

def main():
    print("=== Doki OS Controller ===\n")

    # Get system status
    status = get_status()
    print(f"System Info:")
    print(f"  Free Heap: {status['system']['freeHeap']} bytes")
    print(f"  WiFi: {status['system']['wifi']['ssid']}")
    print(f"  IP: {status['system']['wifi']['ip']}")
    print()

    # List apps
    apps = get_apps()
    print("Available Apps:")
    for app in apps:
        print(f"  [{app['id']}] {app['name']}")
    print()

    # Load clock on display 0
    print("Loading clock on display 0...")
    result = load_app(0, "clock")
    if result['success']:
        print("✓ Loaded successfully")
    else:
        print(f"✗ Error: {result['message']}")

    # Wait 5 seconds
    print("\nWaiting 5 seconds...")
    sleep(5)

    # Load weather on display 0
    print("Loading weather on display 0...")
    result = load_app(0, "weather")
    if result['success']:
        print("✓ Loaded successfully")

if __name__ == "__main__":
    main()
```

### Example 3: Mobile App (Flutter/Dart)

```dart
import 'package:http/http.dart' as http;
import 'dart:convert';

class DokiOSClient {
  final String esp32Ip;
  String get baseUrl => 'http://$esp32Ip';

  DokiOSClient(this.esp32Ip);

  Future<List<dynamic>> getApps() async {
    final response = await http.get(Uri.parse('$baseUrl/api/apps'));
    final data = json.decode(response.body);
    return data['apps'];
  }

  Future<Map<String, dynamic>> getStatus() async {
    final response = await http.get(Uri.parse('$baseUrl/api/status'));
    return json.decode(response.body);
  }

  Future<bool> loadApp(int displayId, String appId) async {
    final response = await http.post(
      Uri.parse('$baseUrl/api/load'),
      headers: {'Content-Type': 'application/json'},
      body: json.encode({
        'displayId': displayId,
        'appId': appId,
      }),
    );

    final result = json.decode(response.body);
    return result['success'];
  }

  Future<bool> uploadAnimation(String filePath) async {
    var request = http.MultipartRequest(
      'POST',
      Uri.parse('$baseUrl/api/animations/upload'),
    );

    request.files.add(await http.MultipartFile.fromPath('file', filePath));

    var response = await request.send();
    var responseBody = await response.stream.bytesToString();
    var result = json.decode(responseBody);

    return result['success'];
  }
}

// Usage
void main() async {
  var client = DokiOSClient('192.168.1.100');

  // Get available apps
  var apps = await client.getApps();
  print('Available apps: $apps');

  // Load clock on display 0
  var success = await client.loadApp(0, 'clock');
  print('Load result: $success');
}
```

---

## Error Handling

### HTTP Status Codes

- **200 OK**: Request successful
- **400 Bad Request**: Invalid request parameters
- **404 Not Found**: Endpoint or resource not found
- **500 Internal Server Error**: Server error

### Common Errors

**Invalid Display ID:**
```json
{
  "success": false,
  "message": "Invalid display ID (must be 0-2)"
}
```

**App Not Found:**
```json
{
  "success": false,
  "message": "App ID not found: invalid_app"
}
```

**File Too Large:**
```json
{
  "success": false,
  "message": "File too large (max 1MB)"
}
```

**Invalid File Format:**
```json
{
  "success": false,
  "message": "Invalid sprite file format"
}
```

---

## Rate Limiting

**Current version:** No rate limiting

**Best Practices:**
- Avoid rapid repeated requests (< 100ms apart)
- Implement client-side throttling
- Use WebSocket for real-time updates instead of polling

---

## Network Requirements

- ESP32 and client must be on same network (or accessible via routing)
- No internet required (local network only)
- Supports WiFi 2.4 GHz only
- For iPhone hotspots: Enable "Maximize Compatibility"

---

## Security Considerations

### Current Implementation

- No authentication
- No HTTPS/TLS
- CORS allows all origins (`*`)

### For Production

**Add API Key Authentication:**
```javascript
// Client side
const API_KEY = 'your-secret-key';

fetch('http://192.168.1.100/api/load', {
    method: 'POST',
    headers: {
        'Content-Type': 'application/json',
        'X-API-Key': API_KEY
    },
    body: JSON.stringify({...})
});
```

**Enable HTTPS** (requires SSL certificate)

**Restrict CORS** to specific domains

**Add Rate Limiting** to prevent abuse

---

## Troubleshooting

### Connection Refused

**Problem:** `curl: (7) Failed to connect`

**Solutions:**
1. Verify ESP32 IP address (check Serial Monitor)
2. Ping ESP32: `ping 192.168.1.100`
3. Ensure ESP32 is on same network
4. Check firewall settings

### Slow Uploads

**Problem:** Upload takes >30 seconds

**Solutions:**
1. Check WiFi signal strength (RSSI)
2. Reduce file size
3. Move closer to WiFi router
4. Use 2.4 GHz network (not 5 GHz)

### CORS Errors in Browser

**Problem:** `CORS policy: No 'Access-Control-Allow-Origin' header`

**Solutions:**
1. This should not happen (Doki OS allows all origins)
2. Clear browser cache
3. Try different browser
4. Use `curl` to test if server is responding

---

## See Also

- **JavaScript API**: [JAVASCRIPT_API.md](JAVASCRIPT_API.md)
- **Animation Upload**: [ANIMATION_UPLOAD_API.md](ANIMATION_UPLOAD_API.md)
- **Sprite Generation**: `tools/sprite_converter.py`

---

## Support

**Serial Monitor Debugging:**
```
[SimpleHTTP] ✓ Server started at http://192.168.1.100
[SimpleHTTP] GET /api/apps
[SimpleHTTP] POST /api/load - Display: 0, App: clock
[SimpleHTTP] Starting animation upload: spinner.spr
```

**Test Endpoint:**
```bash
curl http://192.168.1.100/api/status
```

If this works, the server is running correctly.
