#!/usr/bin/env python3
"""
Doki OS Sprite Converter
Converts image sequences to .spr format for animation system

Usage:
    python sprite_converter.py input_folder output.spr --fps 30
    python sprite_converter.py animation.gif output.spr --fps 24
"""

import os
import sys
import struct
import argparse
from PIL import Image
from pathlib import Path

# Magic number "DOKI" in ASCII
SPRITE_MAGIC = 0x444F4B49

# Color formats
COLOR_FORMAT_INDEXED_8BIT = 0
COLOR_FORMAT_RGB565 = 1
COLOR_FORMAT_RGB888 = 2

# Compression formats
COMPRESSION_NONE = 0
COMPRESSION_RLE = 1
COMPRESSION_LZ4 = 2

class SpriteConverter:
    def __init__(self):
        self.frames = []
        self.width = 0
        self.height = 0
        self.fps = 30
        self.color_format = COLOR_FORMAT_INDEXED_8BIT
        self.compression = COMPRESSION_NONE
        self.palette = []

    def load_from_folder(self, folder_path):
        """Load frames from a folder of images"""
        print(f"Loading frames from folder: {folder_path}")

        # Get all image files
        image_files = sorted([
            f for f in os.listdir(folder_path)
            if f.lower().endswith(('.png', '.jpg', '.jpeg', '.bmp'))
        ])

        if not image_files:
            raise ValueError(f"No image files found in {folder_path}")

        print(f"Found {len(image_files)} frames")

        # Load frames
        for idx, filename in enumerate(image_files):
            filepath = os.path.join(folder_path, filename)
            img = Image.open(filepath)

            # Convert to RGB if needed
            if img.mode != 'RGB':
                img = img.convert('RGB')

            # Check dimensions
            if idx == 0:
                self.width, self.height = img.size
                print(f"Frame size: {self.width}×{self.height}")
            else:
                if img.size != (self.width, self.height):
                    raise ValueError(f"Frame {idx} has different size: {img.size}")

            self.frames.append(img)

        print(f"Loaded {len(self.frames)} frames")

    def load_from_gif(self, gif_path, target_width=None, target_height=None):
        """Load frames from animated GIF with optional resizing"""
        print(f"Loading frames from GIF: {gif_path}")

        gif = Image.open(gif_path)

        # Get original GIF properties
        orig_width, orig_height = gif.size

        # Determine final dimensions
        if target_width and target_height:
            self.width = target_width
            self.height = target_height
            print(f"Resizing from {orig_width}×{orig_height} to {self.width}×{self.height}")
        else:
            self.width = orig_width
            self.height = orig_height

        # Try to get FPS from GIF
        try:
            duration = gif.info.get('duration', 100)  # ms per frame
            gif_fps = int(1000 / duration)
            # Only use GIF FPS if we haven't set a custom one
            if self.fps == 30:  # Default value
                self.fps = gif_fps
            print(f"GIF FPS: {gif_fps}, Using FPS: {self.fps}")
        except:
            pass

        # Extract frames
        frame_idx = 0
        try:
            while True:
                gif.seek(frame_idx)
                frame = gif.copy()

                # Convert to RGB
                if frame.mode != 'RGB':
                    frame = frame.convert('RGB')

                # Resize if target dimensions specified
                if target_width and target_height:
                    frame = frame.resize((target_width, target_height), Image.Resampling.LANCZOS)

                self.frames.append(frame)
                frame_idx += 1
        except EOFError:
            pass

        print(f"Loaded {len(self.frames)} frames from GIF")
        print(f"Frame size: {self.width}×{self.height}")

    def generate_palette(self):
        """Generate 256-color palette from all frames"""
        print("Generating 256-color palette...")

        # Combine all frames
        combined = Image.new('RGB', (self.width * len(self.frames), self.height))
        for idx, frame in enumerate(self.frames):
            combined.paste(frame, (idx * self.width, 0))

        # Quantize to 256 colors
        quantized = combined.quantize(colors=256, method=2)

        # Get palette (RGB values)
        palette_data = quantized.getpalette()
        self.palette = []

        for i in range(256):
            r = palette_data[i * 3]
            g = palette_data[i * 3 + 1]
            b = palette_data[i * 3 + 2]
            a = 255  # Fully opaque
            self.palette.append((r, g, b, a))

        print(f"Generated palette with {len(self.palette)} colors")

        # Convert frames to indexed color
        print("Converting frames to indexed color...")
        indexed_frames = []

        for idx, frame in enumerate(self.frames):
            # Quantize this frame using the same palette
            quantized_frame = frame.quantize(palette=quantized)
            indexed_frames.append(quantized_frame)

        self.frames = indexed_frames
        print("Conversion complete")

    def write_sprite(self, output_path):
        """Write sprite to .spr file"""
        print(f"Writing sprite to: {output_path}")

        with open(output_path, 'wb') as f:
            # Write header (64 bytes)
            self._write_header(f)

            # Write palette (256 × 4 bytes = 1024 bytes)
            self._write_palette(f)

            # NOTE: NOT writing frame offset table for uncompressed data
            # C++ loader expects frame data immediately after palette
            # Frame offsets are only needed for compressed/variable-size frames
            # (Removing lines 162-164 to fix duplicate animation bug)

            # Write frame data
            self._write_frames(f)

        # Print file info
        file_size = os.path.getsize(output_path)
        print(f"\nSprite file created successfully!")
        print(f"File size: {file_size:,} bytes ({file_size / 1024:.1f} KB)")
        print(f"Frames: {len(self.frames)}")
        print(f"Dimensions: {self.width}×{self.height}")
        print(f"FPS: {self.fps}")
        print(f"Color format: 8-bit indexed")

    def _write_header(self, f):
        """Write 64-byte header"""
        header = struct.pack(
            '<I H H H H B B B 49s',
            SPRITE_MAGIC,           # magic (4 bytes)
            1,                      # version (2 bytes)
            len(self.frames),       # frameCount (2 bytes)
            self.width,             # frameWidth (2 bytes)
            self.height,            # frameHeight (2 bytes)
            self.fps,               # fps (1 byte)
            self.color_format,      # colorFormat (1 byte)
            self.compression,       # compression (1 byte)
            b'\x00' * 49           # reserved (49 bytes)
        )
        f.write(header)

    def _write_palette(self, f):
        """Write 256-color palette"""
        for r, g, b, a in self.palette:
            f.write(struct.pack('BBBB', r, g, b, a))

    def _calculate_frame_offsets(self):
        """Calculate byte offset for each frame"""
        header_size = 64
        palette_size = 256 * 4
        offset_table_size = len(self.frames) * 4

        # First frame starts after header, palette, and offset table
        base_offset = header_size + palette_size + offset_table_size

        offsets = []
        current_offset = base_offset

        for frame in self.frames:
            offsets.append(current_offset)
            frame_size = self.width * self.height  # 1 byte per pixel
            current_offset += frame_size

        return offsets

    def _write_frames(self, f):
        """Write frame pixel data"""
        for idx, frame in enumerate(self.frames):
            # Get pixel data as bytes
            pixels = frame.tobytes()
            f.write(pixels)

def generate_test_pattern(width, height, num_frames, pattern_type='circle'):
    """Generate test pattern animation with anti-aliasing via supersampling"""
    from PIL import ImageDraw

    print(f"Generating {pattern_type} test pattern with anti-aliasing...")
    print(f"Size: {width}×{height}, Frames: {num_frames}")

    # Anti-aliasing: render at 4x resolution, then downscale
    AA_SCALE = 4
    aa_width = width * AA_SCALE
    aa_height = height * AA_SCALE

    frames = []

    if pattern_type == 'circle':
        # Spinning circle with anti-aliasing
        for i in range(num_frames):
            # Render at 4x resolution
            img = Image.new('RGB', (aa_width, aa_height), 'black')
            draw = ImageDraw.Draw(img)

            # Calculate circle position (scaled)
            angle = (i / num_frames) * 2 * 3.14159
            cx = aa_width // 2
            cy = aa_height // 2
            radius = min(aa_width, aa_height) // 3

            # Draw rotating circle (scaled)
            x = cx + int(radius * 0.7 * cos(angle))
            y = cy + int(radius * 0.7 * sin(angle))

            draw.ellipse(
                (x - 10*AA_SCALE, y - 10*AA_SCALE, x + 10*AA_SCALE, y + 10*AA_SCALE),
                fill='red',
                outline='white',
                width=2*AA_SCALE
            )

            # Downscale with high-quality LANCZOS filter
            img = img.resize((width, height), Image.Resampling.LANCZOS)
            frames.append(img)

    elif pattern_type == 'bounce':
        # Bouncing ball with anti-aliasing
        for i in range(num_frames):
            # Render at 4x resolution
            img = Image.new('RGB', (aa_width, aa_height), 'black')
            draw = ImageDraw.Draw(img)

            # Calculate bounce position (scaled)
            t = i / num_frames
            y = int(aa_height * 0.2 + abs(sin(t * 3.14159 * 2)) * aa_height * 0.6)
            x = int(aa_width * t)

            draw.ellipse(
                (x - 15*AA_SCALE, y - 15*AA_SCALE, x + 15*AA_SCALE, y + 15*AA_SCALE),
                fill='blue',
                outline='cyan',
                width=2*AA_SCALE
            )

            # Downscale with high-quality LANCZOS filter
            img = img.resize((width, height), Image.Resampling.LANCZOS)
            frames.append(img)

    elif pattern_type == 'spinner':
        # Loading spinner with anti-aliasing
        for i in range(num_frames):
            # Render at 4x resolution
            img = Image.new('RGB', (aa_width, aa_height), 'black')
            draw = ImageDraw.Draw(img)

            cx = aa_width // 2
            cy = aa_height // 2
            radius = min(aa_width, aa_height) // 2 - 5*AA_SCALE

            # Draw spinner arc (scaled)
            angle = (i / num_frames) * 360
            draw.arc(
                (cx - radius, cy - radius, cx + radius, cy + radius),
                start=angle,
                end=angle + 270,
                fill='green',
                width=5*AA_SCALE
            )

            # Downscale with high-quality LANCZOS filter
            img = img.resize((width, height), Image.Resampling.LANCZOS)
            frames.append(img)

    elif pattern_type == 'progress':
        # Progress bar with anti-aliasing
        for i in range(num_frames):
            # Render at 4x resolution
            img = Image.new('RGB', (aa_width, aa_height), 'black')
            draw = ImageDraw.Draw(img)

            # Calculate progress (0 to 100%)
            progress = i / (num_frames - 1) if num_frames > 1 else 1.0
            bar_width = int(aa_width * 0.9)
            bar_height = int(aa_height * 0.4)
            bar_x = (aa_width - bar_width) // 2
            bar_y = (aa_height - bar_height) // 2

            # Draw background bar (dark gray)
            draw.rectangle(
                (bar_x, bar_y, bar_x + bar_width, bar_y + bar_height),
                fill=(50, 50, 50),
                outline=(100, 100, 100),
                width=2*AA_SCALE
            )

            # Draw progress fill (green)
            fill_width = int(bar_width * progress)
            if fill_width > 0:
                draw.rectangle(
                    (bar_x, bar_y, bar_x + fill_width, bar_y + bar_height),
                    fill='green'
                )

            # Downscale with high-quality LANCZOS filter
            img = img.resize((width, height), Image.Resampling.LANCZOS)
            frames.append(img)

    elif pattern_type == 'checkmark':
        # Animated checkmark with anti-aliasing
        for i in range(num_frames):
            # Render at 4x resolution
            img = Image.new('RGB', (aa_width, aa_height), 'black')
            draw = ImageDraw.Draw(img)

            # Calculate draw progress
            progress = i / (num_frames - 1) if num_frames > 1 else 1.0

            cx = aa_width // 2
            cy = aa_height // 2
            size = min(aa_width, aa_height) * 0.4

            # Checkmark coordinates (two lines) - scaled
            # Line 1: bottom-left to middle
            x1, y1 = cx - size * 0.5, cy
            x2, y2 = cx - size * 0.1, cy + size * 0.5

            # Line 2: middle to top-right
            x3, y3 = cx + size * 0.6, cy - size * 0.6

            # Draw animated checkmark (scaled)
            if progress < 0.5:
                # Draw first half (bottom-left to middle)
                t = progress * 2
                draw.line(
                    (x1, y1, x1 + (x2 - x1) * t, y1 + (y2 - y1) * t),
                    fill='green',
                    width=5*AA_SCALE
                )
            else:
                # Draw full first half
                draw.line((x1, y1, x2, y2), fill='green', width=5*AA_SCALE)

                # Draw second half (middle to top-right)
                t = (progress - 0.5) * 2
                draw.line(
                    (x2, y2, x2 + (x3 - x2) * t, y2 + (y3 - y2) * t),
                    fill='green',
                    width=5*AA_SCALE
                )

            # Downscale with high-quality LANCZOS filter
            img = img.resize((width, height), Image.Resampling.LANCZOS)
            frames.append(img)

    elif pattern_type == 'pulse':
        # Pulsing circle with anti-aliasing
        for i in range(num_frames):
            # Render at 4x resolution
            img = Image.new('RGB', (aa_width, aa_height), 'black')
            draw = ImageDraw.Draw(img)

            # Calculate pulse (sine wave)
            t = i / num_frames
            scale = 0.5 + 0.5 * abs(sin(t * 3.14159 * 2))  # Pulse between 0.5 and 1.0

            cx = aa_width // 2
            cy = aa_height // 2
            max_radius = min(aa_width, aa_height) // 2 - 10*AA_SCALE
            radius = int(max_radius * scale)

            # Draw pulsing circle with gradient effect (scaled)
            # Outer glow
            for r_offset in range(3):
                opacity = int(100 * (1 - r_offset / 3))
                color = (0, opacity + 100, opacity + 155)
                draw.ellipse(
                    (cx - radius - r_offset * 3*AA_SCALE, cy - radius - r_offset * 3*AA_SCALE,
                     cx + radius + r_offset * 3*AA_SCALE, cy + radius + r_offset * 3*AA_SCALE),
                    outline=color,
                    width=2*AA_SCALE
                )

            # Main circle
            draw.ellipse(
                (cx - radius, cy - radius, cx + radius, cy + radius),
                fill=(0, 150, 255),
                outline=(0, 200, 255),
                width=2*AA_SCALE
            )

            # Downscale with high-quality LANCZOS filter
            img = img.resize((width, height), Image.Resampling.LANCZOS)
            frames.append(img)

    return frames

def cos(x):
    import math
    return math.cos(x)

def sin(x):
    import math
    return math.sin(x)

def main():
    parser = argparse.ArgumentParser(
        description='Convert image sequences to Doki OS .spr format'
    )

    parser.add_argument(
        'input',
        help='Input folder (images) or GIF file, or "test" to generate test pattern'
    )

    parser.add_argument(
        'output',
        help='Output .spr file path'
    )

    parser.add_argument(
        '--fps',
        type=int,
        default=30,
        help='Frames per second (default: 30)'
    )

    parser.add_argument(
        '--pattern',
        choices=['circle', 'bounce', 'spinner', 'progress', 'checkmark', 'pulse'],
        default='spinner',
        help='Test pattern type (for test mode only)'
    )

    parser.add_argument(
        '--width',
        type=int,
        default=64,
        help='Width for test pattern (default: 64)'
    )

    parser.add_argument(
        '--height',
        type=int,
        default=64,
        help='Height for test pattern (default: 64)'
    )

    parser.add_argument(
        '--frames',
        type=int,
        default=20,
        help='Number of frames for test pattern (default: 20)'
    )

    args = parser.parse_args()

    converter = SpriteConverter()
    converter.fps = args.fps

    # Load frames
    if args.input.lower() == 'test':
        # Generate test pattern
        converter.frames = generate_test_pattern(
            args.width,
            args.height,
            args.frames,
            args.pattern
        )
        converter.width = args.width
        converter.height = args.height
    elif args.input.lower().endswith('.gif'):
        # Load from GIF
        # Pass target dimensions if specified (not default test values)
        target_w = args.width if args.width != 64 else None
        target_h = args.height if args.height != 64 else None
        converter.load_from_gif(args.input, target_w, target_h)
    else:
        # Load from folder
        converter.load_from_folder(args.input)

    # Generate palette and convert to indexed color
    converter.generate_palette()

    # Write sprite file
    converter.write_sprite(args.output)

    print("\n✓ Conversion complete!")

if __name__ == '__main__':
    main()
