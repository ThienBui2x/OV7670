import serial
from pathlib import Path
from PIL import Image

PORT = "COM5"
BAUD = 115200

START = b"START"
END = b"END"

WIDTH = 160
HEIGHT = 120
FRAME_SIZE = WIDTH * HEIGHT * 2

OUTPUT_DIR = Path(__file__).resolve().parent

ser = serial.Serial(PORT, BAUD, timeout=1)


def wait_for_marker(marker):
    """Wait until the specified marker is received."""
    buffer = bytearray()

    while True:
        data = ser.read(1)

        if not data:
            continue

        buffer.extend(data)

        if buffer.endswith(marker):
            return


def receive_frame():
    """Receive exactly one frame."""
    frame = bytearray()

    while len(frame) < FRAME_SIZE:
        chunk = ser.read(FRAME_SIZE - len(frame))

        if chunk:
            frame.extend(chunk)
            print(f"RX {len(frame)}/{FRAME_SIZE} bytes")

    return frame



def convert_rgb565(frame):
    image = Image.new("RGB", (WIDTH, HEIGHT))

    # Same loop order as your C++ code
    for x in range(WIDTH):
        for y in range(HEIGHT):

            # Same index calculation as C++
            i = (y * WIDTH + x) << 1

            # Read one RGB565 pixel (little-endian)
            pixel = frame[i] | (frame[i + 1] << 8)

            # Convert RGB565 to RGB888
            r5 = (pixel >> 11) & 0x1F
            g6 = (pixel >> 5) & 0x3F
            b5 = pixel & 0x1F

            r = (r5 * 255) // 31
            g = (g6 * 255) // 63
            b = (b5 * 255) // 31

            # Write this pixel directly to its coordinates
            image.putpixel((x, y), (r, g, b))

    return image



try:
    while True:
        # 1. Wait for START from the STM32
        print("Waiting for START...")
        wait_for_marker(START)
        print("START detected")

        # 2. Receive the raw frame
        frame = receive_frame()

        # 3. Wait for END
        wait_for_marker(END)
        print("END detected")
        print("FRAME OK:", len(frame), "bytes")

        # 4. Save original binary data
        raw_path = OUTPUT_DIR / "raw_data.bin"
        raw_path.write_bytes(frame)

        # 5. Convert and save image
        image = convert_rgb565(frame)

        png_path = OUTPUT_DIR / "frame.png"
        image.save(png_path)

        print("Image saved:", png_path)

        # Stop after one image.
        break

except KeyboardInterrupt:
    print("Stopped.")

finally:
    ser.close()