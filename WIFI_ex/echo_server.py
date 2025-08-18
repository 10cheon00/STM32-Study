#!/usr/bin/env python3
"""
Simple TCP Echo Server (ESP8266-friendly)

Usage:
  python3 echo_server.py --host 172.30.1.19 --port 5001

- Accepts multiple clients (threaded)
- Echos back exactly what it receives (binary-safe)
- Prints basic connection and data logs
"""
import argparse
import socketserver
from datetime import datetime

class EchoHandler(socketserver.BaseRequestHandler):
    def handle(self):
        peer = f"{self.client_address[0]}:{self.client_address[1]}"
        print(f"[{ts()}] + CONNECT {peer}")
        try:
            while True:
                data = self.request.recv(4096)
                if not data:
                    print(f"[{ts()}] - DISCONNECT {peer}")
                    break
                # Log (show both len and a safe preview)
                preview = data.decode("utf-8", errors="ignore")
                print(f"[{ts()}] < {peer} ({len(data)}B): {preview!r}")
                # Echo back
                self.request.sendall(data)
        except ConnectionResetError:
            print(f"[{ts()}] ! RESET {peer}")
        except Exception as e:
            print(f"[{ts()}] ! ERROR {peer}: {e}")

class ThreadedTCPServer(socketserver.ThreadingMixIn, socketserver.TCPServer):
    daemon_threads = True
    allow_reuse_address = True

def ts():
    return datetime.now().strftime("%H:%M:%S")

def main():
    ap = argparse.ArgumentParser(description="Threaded TCP Echo Server")
    ap.add_argument("--host", default="0.0.0.0", help="Bind address (e.g., 172.30.1.19)")
    ap.add_argument("--port", type=int, default=5001, help="TCP port (e.g., 5001)")
    args = ap.parse_args()

    with ThreadedTCPServer((args.host, args.port), EchoHandler) as server:
        bind_ip, bind_port = server.server_address
        print(f"[{ts()}] Echo server listening on {bind_ip}:{bind_port}")
        print(f"[{ts()}] Press Ctrl+C to stop.")
        try:
            server.serve_forever()
        except KeyboardInterrupt:
            print(f"\n[{ts()}] Shutting down...")
        finally:
            server.shutdown()
            server.server_close()

if __name__ == "__main__":
    main()
