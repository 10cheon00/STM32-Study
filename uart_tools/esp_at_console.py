#!/usr/bin/env python3
import serial
import time
import sys
import threading
import argparse
import os

def line_sender(ser: serial.Serial, stop_event: threading.Event, append_crlf: bool):
    """
    사용자로부터 한 줄씩 입력받아 그대로 전송.
    '/quit' 또는 Ctrl+D(Unix)/Ctrl+Z+Enter(Windows)로 종료.
    """
    print("입력한 한 줄을 그대로 전송합니다. '/quit'을 입력하면 종료합니다.")
    while not stop_event.is_set():
        try:
            line = input("> ")
        except EOFError:
            print("\n입력 스트림 종료 감지, 프로그램을 종료합니다.")
            stop_event.set()
            break

        if line.strip().lower() == "/quit":
            print("종료 명령 감지, 프로그램을 종료합니다.")
            stop_event.set()
            break

        data = line.encode("utf-8", errors="ignore")
        if append_crlf:
            data += b"\r\n"
        try:
            ser.write(data)
            try:
                # 표시용
                disp = data.decode("utf-8", errors="ignore").rstrip("\r\n")
            except Exception:
                disp = str(data)
            print(f"송신: {disp}")
        except serial.SerialException as e:
            print(f"전송 실패: {e}")
            stop_event.set()
            break


def main():
    parser = argparse.ArgumentParser(description="AT 커맨드 콘솔 (한 줄 입력 -> 그대로 전송)")
    parser.add_argument("-p", "--port", default="/dev/ttyS3",
                        help="시리얼 포트 이름 (예: COM3, /dev/ttyUSB0, /dev/tty.usbmodemXXXX)")
    parser.add_argument("-s", "--speed", type=int, default=115200,
                        help="보레이트 (baud rate), 기본: 115200")
    parser.add_argument("--no-crlf", action="store_true",
                        help="라인 끝에 CRLF(\\r\\n)를 붙이지 않습니다.")
    args = parser.parse_args()

    ser = serial.Serial()
    ser.port       = args.port
    ser.baudrate   = args.speed
    ser.bytesize   = serial.EIGHTBITS
    ser.parity     = serial.PARITY_NONE
    ser.stopbits   = serial.STOPBITS_ONE
    ser.timeout    = 0.1          # 논블로킹 수신
    ser.xonxoff    = False
    ser.rtscts     = False
    ser.dsrdtr     = False

    try:
        ser.open()
    except serial.SerialException as e:
        print(f"포트 열기 실패: {e}")
        sys.exit(1)

    print(f"포트 {ser.port} 열림: {ser.baudrate} bps (Ctrl+C 또는 '/quit'로 종료)")

    stop_event = threading.Event()
    sender = threading.Thread(
        target=line_sender,
        args=(ser, stop_event, not args.no_crlf),
        daemon=True
    )
    sender.start()

    # 메인 루프: 수신 처리
    try:
        while not stop_event.is_set():
            try:
                if ser.in_waiting:
                    data = ser.read(ser.in_waiting)
                    try:
                        text = data.decode("utf-8", errors="ignore")
                        print(text, end="", flush=True)
                    except Exception:
                        print(f"[raw] {data}")
            except serial.SerialException as e:
                print(f"수신 실패: {e}")
                break
            time.sleep(0.05)
    except KeyboardInterrupt:
        print("\n키보드 인터럽트 감지, 종료 중...")
    finally:
        stop_event.set()
        try:
            sender.join(timeout=1.0)
        except Exception:
            pass
        try:
            ser.close()
        except Exception:
            pass
        print("포트 닫힘, 프로그램 종료.")


if __name__ == "__main__":
    main()
