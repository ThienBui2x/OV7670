import serial
from pathlib import Path
from PIL import Image

PORT = "COM5"
BAUD = 115200

START = b"START"
END   = b"END"

FRAME_SIZE = 160*120*2

ser = serial.Serial(PORT, BAUD, timeout=1)

while True:

    # # -------------------------------------------------
    # # WAIT FOR REQUEST FROM USER
    # # -------------------------------------------------
    # command = input("Enter command: ")

    # if command.upper() == "EXIT":
    #     break

    # # Send request to board
    # ser.write((command + "\r\n").encode())


    # -------------------------------------------------
    # WAIT FOR START
    # -------------------------------------------------
    sync = bytearray()

    while True:
        sync += ser.read(1)

        if sync.endswith(START):
            print("START detected")
            break

    # -------------------------------------------------
    # RECEIVE EXACT FRAME DATA
    # -------------------------------------------------
    frame = bytearray()

    while len(frame) < FRAME_SIZE:
        remaining = FRAME_SIZE - len(frame)
        chunk = ser.read(remaining)

        if chunk:
            frame.extend(chunk)
            
            print(
                f"RX {len(chunk)} bytes "
                f"({len(frame)}/{FRAME_SIZE})"
            )

    # -------------------------------------------------
    # WAIT FOR END
    # -------------------------------------------------
    tail = bytearray()

    while True:
        tail += ser.read(1)

        if tail.endswith(END):
            print("END detected")
            break

    # -------------------------------------------------
    # FRAME COMPLETE
    # -------------------------------------------------
    print("FRAME OK:", len(frame))

    # -------------------------------------------------
    # SAW RAW DATA
    # -------------------------------------------------
    file_path = Path(__file__).parent / "raw_data.txt"
    with open(file_path, "w", encoding="utf-8") as file:
        file.write(frame.hex())

    # -------------------------------------------------
    # POST PROCESSING - CONVERT TO RGB FORMAT
    # -------------------------------------------------
    pixels = []

    for i in range(0, FRAME_SIZE, 2):

        # RGB565, little-endian
        pixel = frame[i] | (frame[i + 1] << 8)

        r5 = (pixel >> 11) & 0x1F
        g6 = (pixel >> 5) & 0x3F
        b5 = pixel & 0x1F

        # Convert 5/6/5-bit values to 8-bit RGB
        r = (r5 * 255) // 31
        g = (g6 * 255) // 63
        b = (b5 * 255) // 31

        pixels.append((r, g, b))
        # pixels.append(pixel)
    # -------------------------------------------------
    # SAW RGB DATA - IMAGE FORMAT
    # -------------------------------------------------
    image = Image.new("RGB", (160, 120))
    image.putdata(pixels)

    png_path = Path(__file__).parent / "frame.png"
    image.save(png_path)



    break