#!/usr/bin/env python3
"""
Animation Upload Client for Doki OS

Simple command-line tool to upload .spr animation files to ESP32 over HTTP.

Usage:
    python upload_animation.py <esp32_ip> <animation_file.spr>

Example:
    python upload_animation.py 192.168.1.100 animations/spinner.spr
"""

import sys
import os
import requests
from pathlib import Path


def validate_spr_file(filepath):
    """Validate that file is a valid .spr sprite file"""
    if not os.path.exists(filepath):
        print(f"❌ Error: File not found: {filepath}")
        return False

    if not filepath.endswith('.spr'):
        print(f"⚠️  Warning: File doesn't have .spr extension: {filepath}")

    # Check magic number
    with open(filepath, 'rb') as f:
        magic_bytes = f.read(4)

    if len(magic_bytes) < 4:
        print(f"❌ Error: File too small to be valid sprite")
        return False

    # Magic number should be "DOKI" (0x444F4B49 in little-endian)
    expected_magic = b'IKOD'  # Little-endian representation
    if magic_bytes != expected_magic:
        print(f"❌ Error: Invalid sprite file format")
        print(f"   Expected magic: {expected_magic.hex()}")
        print(f"   Got magic:      {magic_bytes.hex()}")
        return False

    file_size = os.path.getsize(filepath)
    if file_size > 1024 * 1024:
        print(f"❌ Error: File too large ({file_size} bytes, max 1MB)")
        return False

    print(f"✓ Valid .spr file: {Path(filepath).name} ({file_size:,} bytes)")
    return True


def upload_animation(esp32_ip, filepath):
    """Upload animation file to ESP32"""
    url = f"http://{esp32_ip}/api/animations/upload"

    print(f"\n📤 Uploading to {url}...")

    try:
        with open(filepath, 'rb') as f:
            filename = Path(filepath).name
            files = {
                'file': (filename, f, 'application/octet-stream')
            }

            response = requests.post(url, files=files, timeout=30)

        if response.status_code == 200:
            result = response.json()
            print(f"✓ Upload successful!")
            print(f"  Response: {result}")
            print(f"\n📱 Animation saved to: /animations/{filename}")
            print(f"\n💡 To use in JavaScript app:")
            print(f"   var animId = loadAnimation(\"/animations/{filename}\");")
            print(f"   setAnimationPosition(animId, x, y);")
            print(f"   playAnimation(animId, loop);")
            return True
        else:
            print(f"❌ Upload failed: HTTP {response.status_code}")
            print(f"   Response: {response.text}")
            return False

    except requests.exceptions.Timeout:
        print(f"❌ Upload timeout (>30 seconds)")
        print(f"   Check WiFi connection and ESP32 status")
        return False

    except requests.exceptions.ConnectionError:
        print(f"❌ Connection failed")
        print(f"   - Is ESP32 powered on?")
        print(f"   - Is IP address correct? ({esp32_ip})")
        print(f"   - Are you on the same network?")
        print(f"   - Check Serial Monitor for actual IP address")
        return False

    except Exception as e:
        print(f"❌ Upload error: {e}")
        return False


def main():
    if len(sys.argv) != 3:
        print("Usage: python upload_animation.py <esp32_ip> <animation_file.spr>")
        print("\nExample:")
        print("  python upload_animation.py 192.168.1.100 animations/spinner.spr")
        print("\nGenerate animations with sprite_converter.py:")
        print("  python sprite_converter.py --generate spinner --width 100 --height 100 --frames 20 --fps 30 --output spinner.spr")
        sys.exit(1)

    esp32_ip = sys.argv[1]
    filepath = sys.argv[2]

    print("=" * 60)
    print("Doki OS - Animation Upload Client")
    print("=" * 60)

    # Validate file
    if not validate_spr_file(filepath):
        sys.exit(1)

    # Upload
    success = upload_animation(esp32_ip, filepath)

    print("=" * 60)
    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
