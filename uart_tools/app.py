#!/usr/bin/env python3
import serial
import time
import sys
import threading
import argparse
import os

def get_key():
    """Windows용 msvcrt 또는 Linux/WSL용 termios+tty로 1바이트 읽기"""
    try:
        # Windows
        import msvcrt
        return msvcrt.getch()
    except ImportError:
        # Linux / WSL
        import tty, termios
        fd = sys.stdin.fileno()
        old = termios.tcgetattr(fd)
        try:
            tty.setraw(fd)
            ch = sys.stdin.read(1)
            return ch.encode('utf-8', errors='ignore')
        finally:
            termios.tcsetattr(fd, termios.TCSADRAIN, old)

def key_listener(ser, stop_event):
    print("키를 누르면 해당 키(1바이트)를 즉시 전송합니다. '1' 키를 누르면 프로그램이 종료됩니다.")
    while not stop_event.is_set():
        k = get_key()
        if not k:
            continue
        if k == b'1':
            print("종료 키 '1' 입력 감지, 프로그램을 종료합니다.")
            stop_event.set()
            break
        ser.write(k)
        try:
            display_char = k.decode('utf-8')
        except UnicodeDecodeError:
            display_char = str(k)
        print(f"송신: {display_char}")

def main():
    parser = argparse.ArgumentParser(description="UART 키-스터디 프로그램")
    parser.add_argument('-p', '--port', default='/dev/ttyS3',
                        help="시리얼 포트 이름 (예: COM3 또는 /dev/ttyUSB0)")
    parser.add_argument('-s', '--speed', type=int, default=9600,
                        help="보레이트 (baud rate), 예: 115200")
    args = parser.parse_args()

    # 1) Serial 객체 생성 및 설정
    ser = serial.Serial()
    ser.port       = args.port
    ser.baudrate   = args.speed
    ser.bytesize   = serial.EIGHTBITS
    ser.parity     = serial.PARITY_NONE
    ser.stopbits   = serial.STOPBITS_ONE
    ser.timeout    = 1
    ser.xonxoff    = False
    ser.rtscts     = False
    ser.dsrdtr     = False

    try:
        ser.open()
    except serial.SerialException as e:
        print(f"포트 열기 실패: {e}")
        sys.exit(1)

    print(f"포트 {ser.port} 열림: {ser.baudrate} bps")

    stop_event = threading.Event()
    listener = threading.Thread(target=key_listener, args=(ser, stop_event), daemon=True)
    listener.start()

    # 4) 메인 루프: 수신 처리 및 종료 대기
    try:
        while not stop_event.is_set():
            if ser.in_waiting:
                data = ser.read(ser.in_waiting)
                try:
                    print("수신:", data.decode('utf-8', errors='ignore').strip(), end='\r\n')
                except:
                    print("수신 (raw):", data)
            time.sleep(0.1)
    except KeyboardInterrupt:
        print("\n키보드 인터럽트 감지, 종료 중...")
    finally:
        ser.close()
        print("포트 닫힘, 프로그램 종료.")
        os._exit(0)

if __name__ == "__main__":
    main()

