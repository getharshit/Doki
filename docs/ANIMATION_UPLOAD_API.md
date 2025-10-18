# Animation Upload API

## Overview
Upload `.spr` animation sprite files to Doki OS over HTTP. The system validates the file format and saves it to the `/animations/` directory.

## Endpoint

```
POST /api/animations/upload
```

**Content-Type**: `multipart/form-data`

## Request Format

Upload a `.spr` file as multipart form data with the file field name (any name works).

### Example: curl

```bash
# Replace <ESP32_IP> with your device's IP address
curl -X POST \
  -F "file=@/path/to/animation.spr" \
  http://<ESP32_IP>/api/animations/upload
```

### Example: Python

```python
import requests

ESP32_IP = "192.168.1.100"  # Replace with your ESP32's IP
url = f"http://{ESP32_IP}/api/animations/upload"

with open("animation.spr", "rb") as f:
    files = {"file": ("animation.spr", f, "application/octet-stream")}
    response = requests.post(url, files=files)

print(response.json())
```

### Example: JavaScript (Browser)

```javascript
async function uploadAnimation(file) {
    const ESP32_IP = "192.168.1.100";  // Replace with your ESP32's IP
    const url = `http://${ESP32_IP}/api/animations/upload`;

    const formData = new FormData();
    formData.append("file", file);

    try {
        const response = await fetch(url, {
            method: "POST",
            body: formData
        });

        const result = await response.json();
        console.log("Upload success:", result);
        return result;
    } catch (error) {
        console.error("Upload failed:", error);
        throw error;
    }
}

// Usage with file input
document.getElementById("fileInput").addEventListener("change", (e) => {
    const file = e.target.files[0];
    if (file && file.name.endsWith(".spr")) {
        uploadAnimation(file);
    } else {
        alert("Please select a .spr file");
    }
});
```

### Example: Swift (iOS)

```swift
import Foundation

func uploadAnimation(fileURL: URL, to esp32IP: String, completion: @escaping (Bool) -> Void) {
    let url = URL(string: "http://\(esp32IP)/api/animations/upload")!

    var request = URLRequest(url: url)
    request.httpMethod = "POST"

    let boundary = UUID().uuidString
    request.setValue("multipart/form-data; boundary=\(boundary)", forHTTPHeaderField: "Content-Type")

    var body = Data()

    // Add file data
    body.append("--\(boundary)\r\n".data(using: .utf8)!)
    body.append("Content-Disposition: form-data; name=\"file\"; filename=\"\(fileURL.lastPathComponent)\"\r\n".data(using: .utf8)!)
    body.append("Content-Type: application/octet-stream\r\n\r\n".data(using: .utf8)!)

    if let fileData = try? Data(contentsOf: fileURL) {
        body.append(fileData)
    }

    body.append("\r\n--\(boundary)--\r\n".data(using: .utf8)!)

    request.httpBody = body

    let task = URLSession.shared.dataTask(with: request) { data, response, error in
        if let error = error {
            print("Upload error: \(error)")
            completion(false)
            return
        }

        if let httpResponse = response as? HTTPURLResponse,
           httpResponse.statusCode == 200 {
            print("Upload successful")
            completion(true)
        } else {
            completion(false)
        }
    }

    task.resume()
}

// Usage
let fileURL = URL(fileURLWithPath: "/path/to/animation.spr")
uploadAnimation(fileURL: fileURL, to: "192.168.1.100") { success in
    print("Upload completed: \(success)")
}
```

## Response

### Success (HTTP 200)

```json
{
    "success": true,
    "message": "Animation uploaded"
}
```

### Error Responses

The endpoint validates files server-side and logs errors to Serial Monitor:

- **Empty file**: "Error: Empty animation file"
- **File too large** (> 1MB): "Error: Animation file too large"
- **Invalid format**: "Error: Invalid sprite file" (magic number mismatch)
- **Save failure**: "Failed to save animation"

Client receives HTTP 200 even on validation errors (errors logged to Serial Monitor only).

## File Validation

Uploaded files must:
1. Be ≤ 1MB in size
2. Have magic number `0x444F4B49` ("DOKI" in ASCII) at offset 0
3. Be valid `.spr` sprite format

## Storage

Files are saved to:
```
/animations/<filename>
```

Example: Uploading `spinner.spr` saves to `/animations/spinner.spr`

## Usage Notes

1. **No duplicate detection**: Uploading the same filename overwrites existing file
2. **No file listing**: Use known animation names (app-driven approach)
3. **Maximum size**: 1MB per animation
4. **Memory pool**: Total animation pool is 1024 KB (multiple animations can be loaded simultaneously)
5. **Immediate availability**: Once uploaded, animations can be loaded via JavaScript API:

```javascript
var animId = loadAnimation("/animations/myanimation.spr");
```

## Generating Animation Files

Use the `sprite_converter.py` tool to create `.spr` files:

```bash
cd tools

# From PNG sequence
python sprite_converter.py \
  --input frames/*.png \
  --output animation.spr \
  --fps 30

# From GIF
python sprite_converter.py \
  --input animation.gif \
  --output animation.spr

# Generate test pattern
python sprite_converter.py \
  --generate spinner \
  --width 100 \
  --height 100 \
  --frames 20 \
  --fps 30 \
  --output spinner.spr
```

## Security Considerations

**Current Implementation**: No authentication required.

**For Production**: Consider adding:
- API key authentication
- Rate limiting
- File size quotas per client
- Filename sanitization (currently accepted as-is)

## Troubleshooting

### Upload appears successful but animation doesn't load

1. Check Serial Monitor for validation errors
2. Verify file has correct magic number: `xxd -l 4 animation.spr` should show `49 4b 4f 44` (DOKI in little-endian)
3. Ensure SPIFFS has enough free space
4. Verify file saved: Check SPIFFS contents via Serial Monitor or file listing endpoint

### Connection refused

1. Verify ESP32 is connected to WiFi (check Serial Monitor for IP)
2. Ensure client is on same network
3. Check firewall settings
4. Verify HTTP server started: Look for `[SimpleHTTP] ✓ Server started at http://...`

### Upload timeout

1. Large files (>200 KB) may take 5-10 seconds
2. Check WiFi signal strength
3. Verify ESP32 isn't in power-saving mode

## Example: Complete Upload Flow

```bash
# 1. Generate animation
cd tools
python sprite_converter.py --generate pulse --width 100 --height 100 --frames 20 --fps 20 --output pulse.spr

# 2. Check ESP32 IP (from Serial Monitor)
# Example output: [SimpleHTTP] ✓ Server started at http://192.168.1.100

# 3. Upload to ESP32
curl -X POST -F "file=@pulse.spr" http://192.168.1.100/api/animations/upload

# 4. Response
{"success":true,"message":"Animation uploaded"}

# 5. Serial Monitor shows:
[SimpleHTTP] Starting animation upload: pulse.spr
[SimpleHTTP] Animation upload progress: 196608 bytes
[SimpleHTTP] Animation upload complete: 196608 bytes total
[SimpleHTTP] ✓ Valid sprite file detected
[SimpleHTTP] ✓ Animation saved: /animations/pulse.spr (196608 bytes)
```

## Future Enhancements

Potential additions (not currently implemented):
- `GET /api/animations` - List uploaded animations
- `DELETE /api/animations/{name}` - Remove animation
- `GET /api/animations/{name}/info` - Get animation metadata (dimensions, frames, fps)
- WebSocket notifications when animations are uploaded
- Chunked upload progress reporting
