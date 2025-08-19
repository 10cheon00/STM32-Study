#!/usr/bin/env python3
import argparse
import struct
import wave
import sys
import time

try:
    import serial  # pip install pyserial
except Exception as e:
    serial = None

MAGIC = b'\x55\xAAS1'  # 0x55 0xAA 'S' '1'
HEADER_SIZE = 6        # 4-byte MAGIC + uint16 little-endian sample count

def find_sync_and_len(ser, timeout=5.0):
    """Scan the serial stream until a MAGIC header is found; return payload length (samples)."""
    deadline = time.time() + timeout
    buf = bytearray()
    while time.time() < deadline:
        b = ser.read(1)
        if not b:
            continue
        buf += b
        if len(buf) > 4:
            del buf[:-4]
        if bytes(buf) == MAGIC:
            ln = ser.read(2)
            if len(ln) < 2:
                # try again
                buf.clear()
                continue
            (N,) = struct.unpack('<H', ln)
            return N
    raise TimeoutError("Sync not found within timeout. Check wiring/baud/MAGIC.")

def record_to_wav(port, baud, seconds, fs, outfile, samples_per_frame_hint=256, verbose=True):
    if serial is None:
        print("pyserial is not installed. Install with: pip install pyserial", file=sys.stderr)
        sys.exit(1)

    total_samples_target = int(seconds * fs) if seconds > 0 else None
    frames_written = 0

    with serial.Serial(port, baudrate=baud, timeout=1) as ser, \
         wave.open(outfile, 'wb') as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)   # 16-bit PCM
        wav.setframerate(fs)

        if verbose:
            print(f"[+] Opened {port} at {baud} bps, writing {outfile} @ {fs} Hz")
            if total_samples_target:
                print(f"[+] Target: {seconds} s ({total_samples_target} samples)")
            print("[+] Waiting for sync (MAGIC=55 AA 53 31)...")

        try:
            while True:
                N = find_sync_and_len(ser, timeout=5.0)
                payload_bytes = ser.read(N * 2)
                if len(payload_bytes) != N * 2:
                    # resync on short read
                    if verbose:
                        print("[!] Short read; resyncing...")
                    continue

                wav.writeframesraw(payload_bytes)
                frames_written += N

                if verbose and frames_written % (fs // 2) < N:
                    # progress ~2 Hz
                    secs = frames_written / fs
                    print(f"\r[=] {secs:6.2f} s written", end="", flush=True)

                if total_samples_target and frames_written >= total_samples_target:
                    break

        except KeyboardInterrupt:
            if verbose:
                print("\n[!] Interrupted by user; finalizing WAV...")

        finally:
            # Make sure WAV header is finalized
            wav.close()
            if verbose:
                print(f"\n[✓] Done. Total samples: {frames_written} (~{frames_written/fs:.2f} s)")

def main():
    p = argparse.ArgumentParser(description="Record framed PCM from STM32 over UART to WAV")
    p.add_argument("--port", required=True, help="Serial port (e.g., COM7 or /dev/ttyACM0)")
    p.add_argument("--baud", type=int, default=921600, help="Baud rate (default: 921600)")
    p.add_argument("--seconds", type=float, default=10.0, help="Record duration seconds (<=0 for indefinite)")
    p.add_argument("--fs", type=int, default=16000, help="Sample rate (default: 16000 Hz)")
    p.add_argument("--outfile", default="capture.wav", help="Output WAV filename")
    args = p.parse_args()

    try:
        record_to_wav(args.port, args.baud, args.seconds, args.fs, args.outfile)
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(2)

if __name__ == "__main__":
    main()
