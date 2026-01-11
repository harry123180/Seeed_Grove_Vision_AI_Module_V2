#!/usr/bin/env python3
"""
Test script to flash the new Vela model using command-line xmodem.
This helps isolate whether the issue is with the GUI or the model itself.
"""
import os
import sys
import math
import serial
import time

# Add parent xmodem directory to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'xmodem'))
import xmodem

# Configuration
MODEL_PATH = 'output_correct/0_palm_det_0x400000_vela.tflite'
FLASH_ADDR = 0x400000
BAUDRATE = 921600  # Required by Grove Vision AI V2 bootloader
TIMEOUT = 300  # Longer timeout for large model

send_bin_total_packets = 0

def callback(total_packets, success_count, error_count):
    global send_bin_total_packets
    if send_bin_total_packets > 0:
        pct = total_packets / send_bin_total_packets * 100
        bar_cnt = int(pct / 100 * 30)
        bar = "█" * bar_cnt + " " * (30 - bar_cnt)
        seq_num = total_packets % 256
        print(f"\r[{bar}] {pct:.1f}% {total_packets}/{send_bin_total_packets} seq={seq_num} err={error_count}", end="")

def create_preamble(flash_addr, offset=0, packet_size=128):
    """Create model preamble"""
    header = [0xC0, 0x5A]
    header += list(flash_addr.to_bytes(4, 'little'))
    header += list(offset.to_bytes(4, 'little'))
    header += [0x5A, 0xC0]
    header += [0xFF] * (packet_size - 12)
    return bytes(header)

def main():
    global send_bin_total_packets

    if len(sys.argv) < 2:
        print("Usage: python test_flash_model.py <COM_PORT>")
        print("Example: python test_flash_model.py COM3")
        sys.exit(1)

    port = sys.argv[1]
    protocol = sys.argv[2] if len(sys.argv) > 2 else 'xmodem'  # Use 128-byte packets by default for testing

    packet_size = 1024 if protocol == 'xmodem1k' else 128

    print(f"=== Model Flash Test ===")
    print(f"Port: {port}")
    print(f"Baud: {BAUDRATE}")
    print(f"Protocol: {protocol} ({packet_size} bytes/packet)")
    print(f"Model: {MODEL_PATH}")
    print(f"Flash addr: 0x{FLASH_ADDR:X}")
    print()

    if not os.path.exists(MODEL_PATH):
        print(f"ERROR: Model file not found: {MODEL_PATH}")
        sys.exit(1)

    file_size = os.path.getsize(MODEL_PATH)
    print(f"Model size: {file_size} bytes ({file_size/1024/1024:.2f} MB)")
    print(f"Total packets: {math.ceil(file_size / packet_size)}")
    print()

    # Open serial port
    print(f"Opening {port} at {BAUDRATE} baud...")
    ser = serial.Serial()
    ser.port = port
    ser.baudrate = BAUDRATE
    ser.timeout = TIMEOUT
    ser.bytesize = serial.EIGHTBITS
    ser.stopbits = serial.STOPBITS_ONE
    ser.parity = serial.PARITY_NONE
    ser.xonxoff = False
    ser.rtscts = False
    ser.open()
    ser.flushInput()
    ser.flushOutput()
    print("Serial port opened successfully")
    print()

    def getc(size, timeout=1):
        return ser.read(size)

    def putc(data, timeout=1):
        return ser.write(data)

    print("=" * 50)
    print(">>> Press RESET button on the device NOW! <<<")
    print("=" * 50)

    # Wait for bootloader
    while True:
        ser.write(b'1')
        response = ser.readline()
        if response:
            try:
                msg = response.decode('utf-8', errors='ignore').strip()
                print(f"[RX] {msg}")
            except:
                pass
            if b'xmodem protocol' in response.lower():
                break
        time.sleep(0.05)

    time.sleep(1)
    ser.flushInput()
    ser.write(b'1\r')  # Confirm xmodem mode
    time.sleep(0.5)
    ser.flushInput()

    print("\n--- Phase 1: Send Preamble ---")
    # Send preamble (always use xmodem 128-byte)
    preamble_modem = xmodem.XMODEM(getc=getc, putc=putc, mode='xmodem')
    preamble = create_preamble(FLASH_ADDR, 0, 128)

    import io
    preamble_stream = io.BytesIO(preamble)
    send_bin_total_packets = 1

    if not preamble_modem.send(preamble_stream, callback=callback):
        print("\nPreamble send FAILED!")
        ser.close()
        sys.exit(1)
    print("\nPreamble sent successfully")

    # Wait for reboot prompt
    print("Waiting for device response...")
    while True:
        response = ser.readline()
        if response:
            try:
                msg = response.decode('utf-8', errors='ignore').strip()
                print(f"[RX] {msg}")
            except:
                pass
            if b'reboot' in response.lower() or b'(y)' in response:
                break
        time.sleep(0.1)

    time.sleep(1)
    ser.flushInput()
    ser.write(b'n\r')  # Continue with model
    time.sleep(1)
    ser.flushInput()

    print("\n--- Phase 2: Send Model ---")
    model_modem = xmodem.XMODEM(getc=getc, putc=putc, mode=protocol)
    send_bin_total_packets = math.ceil(file_size / packet_size)

    with open(MODEL_PATH, 'rb') as f:
        result = model_modem.send(f, callback=callback, retry=32)

    print()
    if result:
        print("=" * 50)
        print("Model flashed SUCCESSFULLY!")
        print("=" * 50)
    else:
        print("=" * 50)
        print("Model flash FAILED!")
        print("=" * 50)

    # Wait for reboot prompt and reboot
    while True:
        response = ser.readline()
        if response:
            try:
                msg = response.decode('utf-8', errors='ignore').strip()
                print(f"[RX] {msg}")
            except:
                pass
            if b'reboot' in response.lower() or b'(y)' in response:
                ser.write(b'y\r')
                print("Rebooting device...")
                break
        time.sleep(0.1)

    ser.close()
    return 0 if result else 1

if __name__ == '__main__':
    sys.exit(main())
