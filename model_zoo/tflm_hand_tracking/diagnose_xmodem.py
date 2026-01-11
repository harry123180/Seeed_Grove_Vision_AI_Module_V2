#!/usr/bin/env python3
"""
Diagnostic script for XMODEM transfer issues.
Tests different configurations to identify the problem.
"""
import os
import sys
import math
import serial
import time
import logging

# Enable detailed xmodem logging
logging.basicConfig(level=logging.DEBUG,
                    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
                    filename='xmodem_debug.log')

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'xmodem'))
import xmodem

BAUDRATE = 921600
TIMEOUT = 300
FLASH_ADDR = 0x400000

# Test configurations
TEST_CONFIGS = [
    ('output/0_palm_det_0x400000_vela.tflite', 'Old model (wrong Vela config)'),
    ('output_correct/0_palm_det_0x400000_vela.tflite', 'New model (correct Vela config)'),
    ('test_flash/old_padded_to_new_size.tflite', 'Old model padded to new size'),
]

send_bin_total_packets = 0
error_details = []

def callback(total_packets, success_count, error_count):
    global send_bin_total_packets
    if send_bin_total_packets > 0:
        pct = total_packets / send_bin_total_packets * 100
        seq_num = total_packets % 256
        print(f"\rPacket {total_packets}/{send_bin_total_packets} ({pct:.1f}%) seq={seq_num} err={error_count}  ", end="", flush=True)

def create_preamble(flash_addr, offset=0, packet_size=128):
    header = [0xC0, 0x5A]
    header += list(flash_addr.to_bytes(4, 'little'))
    header += list(offset.to_bytes(4, 'little'))
    header += [0x5A, 0xC0]
    header += [0xFF] * (packet_size - 12)
    return bytes(header)

def test_flash(port, model_path, description, protocol='xmodem'):
    global send_bin_total_packets

    print(f"\n{'='*60}")
    print(f"Test: {description}")
    print(f"File: {model_path}")
    print(f"Protocol: {protocol}")
    print(f"{'='*60}")

    if not os.path.exists(model_path):
        print(f"ERROR: File not found")
        return False, "File not found"

    file_size = os.path.getsize(model_path)
    packet_size = 1024 if protocol == 'xmodem1k' else 128
    total_packets = math.ceil(file_size / packet_size)

    print(f"File size: {file_size} bytes ({file_size/1024/1024:.2f} MB)")
    print(f"Total packets: {total_packets}")

    # Open serial port
    ser = serial.Serial()
    ser.port = port
    ser.baudrate = BAUDRATE
    ser.timeout = TIMEOUT
    ser.bytesize = serial.EIGHTBITS
    ser.stopbits = serial.STOPBITS_ONE
    ser.parity = serial.PARITY_NONE
    ser.xonxoff = False
    ser.rtscts = False

    try:
        ser.open()
        ser.flushInput()
        ser.flushOutput()
    except Exception as e:
        return False, f"Serial port error: {e}"

    def getc(size, timeout=1):
        data = ser.read(size)
        if data:
            logging.debug(f"RX: {data.hex()}")
        return data

    def putc(data, timeout=1):
        logging.debug(f"TX: {data[:16].hex()}...")
        return ser.write(data)

    print("\n>>> Press RESET button NOW! <<<")

    # Wait for bootloader
    start_time = time.time()
    while time.time() - start_time < 60:
        ser.write(b'1')
        response = ser.readline()
        if response:
            try:
                msg = response.decode('utf-8', errors='ignore').strip()
                if msg:
                    print(f"[RX] {msg}")
            except:
                pass
            if b'xmodem protocol' in response.lower():
                break
        time.sleep(0.05)
    else:
        ser.close()
        return False, "Bootloader timeout"

    time.sleep(1)
    ser.flushInput()
    ser.write(b'1\r')
    time.sleep(0.5)
    ser.flushInput()

    # Send preamble
    print("\n--- Sending Preamble ---")
    preamble_modem = xmodem.XMODEM(getc=getc, putc=putc, mode='xmodem')
    preamble = create_preamble(FLASH_ADDR, 0, 128)

    import io
    send_bin_total_packets = 1
    if not preamble_modem.send(io.BytesIO(preamble), callback=callback):
        ser.close()
        return False, "Preamble failed"
    print("\nPreamble OK")

    # Wait for device response
    while True:
        response = ser.readline()
        if response:
            try:
                msg = response.decode('utf-8', errors='ignore').strip()
                if msg:
                    print(f"[RX] {msg}")
            except:
                pass
            if b'reboot' in response.lower() or b'(y)' in response:
                break
        time.sleep(0.1)

    time.sleep(1)
    ser.flushInput()
    ser.write(b'n\r')
    time.sleep(1)
    ser.flushInput()

    # Send model
    print(f"\n--- Sending Model ({protocol}) ---")
    model_modem = xmodem.XMODEM(getc=getc, putc=putc, mode=protocol)
    send_bin_total_packets = total_packets

    start_transfer = time.time()
    with open(model_path, 'rb') as f:
        result = model_modem.send(f, callback=callback, retry=32)
    transfer_time = time.time() - start_transfer

    print()

    if result:
        print(f"SUCCESS! Transfer time: {transfer_time:.1f}s")
        # Wait for reboot prompt
        while True:
            response = ser.readline()
            if response:
                if b'reboot' in response.lower() or b'(y)' in response:
                    ser.write(b'y\r')
                    break
            time.sleep(0.1)
    else:
        print(f"FAILED after {transfer_time:.1f}s")

    ser.close()
    return result, "OK" if result else f"Failed at ~{send_bin_total_packets} packets"

def main():
    if len(sys.argv) < 2:
        print("Usage: python diagnose_xmodem.py <COM_PORT> [test_num]")
        print("\nTests:")
        for i, (path, desc) in enumerate(TEST_CONFIGS):
            exists = "✓" if os.path.exists(path) else "✗"
            print(f"  {i}: {desc} [{exists}]")
        print("\nExample: python diagnose_xmodem.py COM3 1")
        sys.exit(1)

    port = sys.argv[1]
    test_num = int(sys.argv[2]) if len(sys.argv) > 2 else None

    print(f"Port: {port}")
    print(f"Baud: {BAUDRATE}")
    print(f"Logging to: xmodem_debug.log")

    if test_num is not None:
        if 0 <= test_num < len(TEST_CONFIGS):
            path, desc = TEST_CONFIGS[test_num]
            result, msg = test_flash(port, path, desc, 'xmodem')
            print(f"\nResult: {'PASS' if result else 'FAIL'} - {msg}")
        else:
            print(f"Invalid test number: {test_num}")
    else:
        print("\nNo test number specified. Use 0, 1, or 2.")
        print("Run with test_num argument to execute a specific test.")

if __name__ == '__main__':
    main()
