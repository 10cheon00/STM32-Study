#!/usr/bin/env python3
"""
uart_echo_test.py

사용자가 키보드로 문자열을 입력하고, 엔터 키를 누를 때마다 해당 문자열을 MCU로 전송하며,
MCU로부터 수신된 데이터를 별도 스레드에서 처리하는 예제 스크립트입니다.

특징:
  - argparse로 포트, 보드레이트, 타임아웃 지정
  - 키 입력을 버퍼에 누적, Enter 시 한 번에 전송
  - 백틱(`) 입력 시 프로그램 종료
  - 별도 메인 루프에서 수신 데이터 처리

사용법:
    pip install pyserial
    python uart_echo_test.py -p /dev/tty.usbmodem1103 -s 115200 -t 1

옵션:
  -p, --port    시리얼 포트 (예: /dev/ttyUSB0)
  -s, --speed   보드레이트 (예: 115200)
  -t, --timeout 읽기 타임아웃(초, 기본 1초)
"""

import serial
import sys
import threading
import time
import argparse

# 플랫폼 독립 1바이트 키 리스너
def get_key():
    try:
        import msvcrt
        return msvcrt.getch()
    except ImportError:
        import tty, termios
        fd = sys.stdin.fileno()
        old = termios.tcgetattr(fd)
        try:
            tty.setraw(fd)
            ch = sys.stdin.read(1)
            return ch.encode('utf-8', errors='ignore')
        finally:
            termios.tcsetattr(fd, termios.TCSADRAIN, old)

# 키 입력 스레드: 엔터 시점에만 전송
def key_listener(ser, stop_event):
    print("문자열 입력 모드: Enter를 누르면 전송합니다. 백틱(`)을 누르면 종료합니다.")
    buffer = bytearray()
    while not stop_event.is_set():
        k = get_key()
        if not k:
            continue
        # 종료 키
        if k == b'`':
            print("\n종료 키 '`' 입력 감지, 프로그램을 종료합니다.")
            stop_event.set()
            break
        # 엔터 처리
        if k in (b'\r', b'\n'):
            if buffer:
                try:
                    ser.write(buffer)
                    data_str = buffer.decode('utf-8', errors='ignore')
                except Exception:
                    data_str = str(buffer)
                print(f"송신: {data_str}")
                buffer.clear()
            continue
        # 버퍼에 누적
        buffer.extend(k)
        # 로컬 에코
        try:
            sys.stdout.write(k.decode('utf-8', errors='ignore'))
        except Exception:
            sys.stdout.write('?')
        sys.stdout.flush()

# 메인 함수
def main():
    parser = argparse.ArgumentParser(description="UART 문자열 송수신 테스트")
    parser.add_argument('-p', '--port', required=True,
                        help='시리얼 포트 이름 (예: /dev/ttyUSB0 또는 COM3)')
    parser.add_argument('-s', '--speed', type=int, required=True,
                        help='보드레이트 (예: 9600, 115200)')
    parser.add_argument('-t', '--timeout', type=float, default=1,
                        help='읽기 타임아웃(초), 기본 1초')
    args = parser.parse_args()

    ser = serial.Serial()
    ser.port     = args.port
    ser.baudrate = args.speed
    ser.timeout  = args.timeout
    ser.bytesize = serial.EIGHTBITS
    ser.parity   = serial.PARITY_NONE
    ser.stopbits = serial.STOPBITS_ONE
    ser.xonxoff  = False
    ser.rtscts   = False
    ser.dsrdtr   = False

    try:
        ser.open()
    except serial.SerialException as e:
        print(f"포트 열기 실패: {e}")
        sys.exit(1)

    print(f"포트 {ser.port} 열림: {ser.baudrate} bps, timeout={ser.timeout}s")

    stop_event = threading.Event()
    listener = threading.Thread(target=key_listener, args=(ser, stop_event), daemon=True)
    listener.start()

    # 수신 처리 루프
    try:
        while not stop_event.is_set():
            if ser.in_waiting:
                data = ser.read(ser.in_waiting)
                try:
                    text = data.decode('utf-8', errors='ignore')
                    print(f"\n수신: {text}")
                except Exception:
                    print(f"\n수신 (raw): {data}")
            time.sleep(0.1)
    except KeyboardInterrupt:
        print("\n키보드 인터럽트 감지, 종료 중...")
        stop_event.set()
    finally:
        ser.close()
        print("포트 닫힘, 프로그램 종료.")
        sys.exit(0)

if __name__ == '__main__':
    main()

