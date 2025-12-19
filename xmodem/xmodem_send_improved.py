#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Improved XMODEM sender for Grove Vision AI V2
Fixes the timing issue by continuously sending characters to catch bootloader
"""

import serial
import xmodem
import time
import os
import sys
import argparse
import math
import threading

DEF_TIMEOUT = 60
DEF_BAUDRATE = 115200
DEF_PROTOCOL = 'xmodem'

send_bin_total_packets = 0
ser = None
stop_sending = False


def callback(total_packets, success_count, error_count):
    """Progress callback for xmodem transfer"""
    if send_bin_total_packets != 0:
        bar_total = 30
        bar_cnt = int((total_packets / send_bin_total_packets) * bar_total)
        space_cnt = bar_total - bar_cnt
        percent = total_packets / send_bin_total_packets

        progress = "\r[{}{}] {:.1%} {}/{}".format(
            "=" * bar_cnt,
            " " * space_cnt,
            percent,
            total_packets,
            send_bin_total_packets
        )
        print(progress, end="", flush=True)

        if percent >= 1:
            print()


def send_keys_continuously():
    """Background thread to continuously send '1' to catch bootloader"""
    global stop_sending
    while not stop_sending:
        try:
            if ser and ser.is_open:
                ser.write(b'1')
        except:
            pass
        time.sleep(0.05)  # Send every 50ms


def uart_open(port, baudrate, timeout):
    global ser
    ser = serial.Serial()
    ser.port = port
    ser.timeout = timeout
    ser.baudrate = baudrate
    ser.bytesize = serial.EIGHTBITS
    ser.stopbits = serial.STOPBITS_ONE
    ser.xonxoff = 0
    ser.rtscts = 0
    ser.parity = serial.PARITY_NONE
    ser.open()
    ser.flushInput()
    ser.flushOutput()
    print(f"[OK] Serial port opened: {port} @ {baudrate}")


def getc_user(size, timeout=1):
    return ser.read(size)


def putc_user(data, timeout=1):
    return ser.write(data)


def send_at_command(command):
    ser.write(bytes(command + "\r", encoding='ascii'))


def wait_for_bootloader(timeout=60):
    """Wait for bootloader with continuous key sending"""
    global stop_sending
    stop_sending = False

    print("\n" + "=" * 50)
    print("  NOW PRESS THE RESET BUTTON ON THE BOARD!")
    print("=" * 50 + "\n")

    # Start background thread to send keys
    sender_thread = threading.Thread(target=send_keys_continuously, daemon=True)
    sender_thread.start()

    start_time = time.time()
    bootloader_detected = False

    while time.time() - start_time < timeout:
        try:
            if ser.in_waiting > 0:
                raw_data = ser.readline()

                # Try to decode
                try:
                    text = raw_data.decode('utf-8', errors='ignore').strip()
                except:
                    text = ""

                if text:
                    print(f"[RX] {text}")

                # Check for bootloader messages
                raw_str = str(raw_data).lower()
                if 'input any key' in raw_str or 'x-modem' in raw_str or '1. flash image update' in raw_str:
                    print("[OK] Bootloader detected!")
                    bootloader_detected = True

                # Check for xmodem ready
                if 'send data using the xmodem protocol' in str(raw_data):
                    print("[OK] XMODEM mode ready!")
                    stop_sending = True
                    time.sleep(0.3)
                    ser.flushInput()
                    return True

        except Exception as e:
            pass

        time.sleep(0.05)

    stop_sending = True
    print(f"\n[ERROR] Bootloader wait timeout ({timeout}s)")
    return False


def xmodem_send_file(file_path, protocol='xmodem'):
    """Send a file via XMODEM"""
    global send_bin_total_packets

    if not os.path.exists(file_path):
        print(f"[ERROR] File not found: {file_path}")
        return False

    packet_size = 128 if protocol == 'xmodem' else 1024
    file_size = os.path.getsize(file_path)
    send_bin_total_packets = math.ceil(file_size / packet_size)

    print(f"\n[INFO] Sending: {os.path.basename(file_path)}")
    print(f"[INFO] Size: {file_size} bytes, Packets: {send_bin_total_packets}")

    modem = xmodem.XMODEM(getc=getc_user, putc=putc_user, mode=protocol)

    with open(file_path, 'rb') as f:
        result = modem.send(f, callback=callback)

    if result:
        print("[OK] File sent successfully!")
    else:
        print("[ERROR] File transfer failed!")

    return result


def main():
    parser = argparse.ArgumentParser(description='Improved XMODEM sender for Grove Vision AI V2')
    parser.add_argument("--port", "-p", required=True, type=str, help="Serial port (e.g., COM3)")
    parser.add_argument("--file", "-f", required=True, type=str, help="Firmware file to send")
    parser.add_argument("--baudrate", "-b", default=DEF_BAUDRATE, type=int, help="Baudrate (default: 115200)")
    parser.add_argument("--protocol", default=DEF_PROTOCOL, choices=['xmodem', 'xmodem1k'], help="Protocol")
    parser.add_argument("--timeout", "-t", default=DEF_TIMEOUT, type=int, help="Timeout in seconds")

    args = parser.parse_args()

    # Check file exists
    if not os.path.exists(args.file):
        print(f"[ERROR] File not found: {args.file}")
        sys.exit(1)

    # Open serial port
    try:
        uart_open(args.port, args.baudrate, args.timeout)
    except Exception as e:
        print(f"[ERROR] Cannot open serial port: {e}")
        sys.exit(1)

    # Wait for bootloader
    if not wait_for_bootloader(args.timeout):
        print("\n[HINT] Make sure to press RESET button immediately after running this script")
        print("[HINT] The bootloader only waits 100ms for input")
        ser.close()
        sys.exit(1)

    # Send firmware
    time.sleep(0.5)
    ser.flushInput()
    send_at_command('1')
    time.sleep(0.3)

    if not xmodem_send_file(args.file, args.protocol):
        ser.close()
        sys.exit(1)

    # Wait for reboot prompt
    print("\n[INFO] Waiting for reboot prompt...")
    start_time = time.time()
    while time.time() - start_time < 30:
        try:
            if ser.in_waiting > 0:
                raw_data = ser.readline()
                try:
                    text = raw_data.decode('utf-8', errors='ignore').strip()
                    if text:
                        print(f"[RX] {text}")
                except:
                    pass

                if 'reboot' in str(raw_data).lower() or '(y)' in str(raw_data):
                    print("\n[OK] Sending reboot command...")
                    send_at_command('y')
                    time.sleep(1)
                    print("[OK] Firmware flash completed! Device is rebooting...")
                    break
        except:
            pass
        time.sleep(0.1)

    ser.close()
    print("\n[DONE] Flash process finished!")


if __name__ == '__main__':
    main()
