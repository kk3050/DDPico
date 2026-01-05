#!/usr/bin/env python3
"""
DDP Serial Bridge - UDP to USB Serial proxy with web dashboard
Version 2.0 - Refactored with separate web files

Forwards DDP packets from UDP to Pico via USB Serial with COBS framing.
Includes real-time web dashboard at http://localhost:4000
"""

import serial
import socket
import sys
import threading
import time
import webbrowser
import os
from collections import deque
from http.server import HTTPServer, BaseHTTPRequestHandler
from datetime import datetime
import json
import mimetypes

# COBS encode/decode (fixed implementation)
def cobs_encode(data: bytes) -> bytes:
    """COBS encode with 0x00 delimiter"""
    result = bytearray()
    code_idx = 0
    code = 1
    
    result.append(0)  # Placeholder for first code byte
    
    for byte in data:
        if byte == 0:
            result[code_idx] = code
            code_idx = len(result)
            result.append(0)
            code = 1
        else:
            result.append(byte)
            code += 1
            if code == 0xFF:
                result[code_idx] = code
                code_idx = len(result)
                result.append(0)
                code = 1
    
    result[code_idx] = code
    result.append(0x00)  # Frame delimiter
    return bytes(result)


def cobs_decode(data: bytes) -> bytes:
    """COBS decode"""
    result = bytearray()
    i = 0
    
    while i < len(data):
        code = data[i]
        i += 1
        
        if code == 0:
            break
        
        for _ in range(code - 1):
            if i < len(data):
                result.append(data[i])
                i += 1
        
        if code < 0xFF and i < len(data) and data[i] != 0:
            result.append(0)
    
    return bytes(result)


class DDPBridge:
    def __init__(self, serial_port, baud=921600, udp_port=4048, web_port=4000, num_leds=43):
        self.serial_port = serial_port
        self.baud = baud
        self.udp_port = udp_port
        self.web_port = web_port
        self.num_leds = num_leds
        self.running = True
        self.ser = None
        self.sock = None

        # Tweening settings
        self.tweening_enabled = False
        self.tweening_steps = 4  # Number of interpolated frames between keyframes
        self.target_fps = 60     # Target frame rate for output
        self.settings_lock = threading.Lock()

        # LED state for tweening
        self.led_state = bytearray(num_leds * 3)  # Current RGB state
        self.prev_led_state = None  # Previous state for interpolation
        self.frame_buffer = deque(maxlen=10)  # Buffer for incoming frames
        self.frame_lock = threading.Lock()

        # Stats
        self.packets_rx = 0
        self.packets_tx = 0
        self.bytes_rx = 0
        self.bytes_tx = 0
        self.last_activity = time.time()

        # Sequence counter for DDP packets
        self.sequence = 0

        # Log buffer for web dashboard
        self.log_buffer = deque(maxlen=100)
        self.log_lock = threading.Lock()
        
    def log(self, message, to_console=True):
        """Add log entry"""
        with self.log_lock:
            self.log_buffer.append({
                'type': 'log',
                'message': message,
                'timestamp': datetime.now().isoformat()
            })
        if to_console:
            print(message)
    
    def connect_serial(self):
        """Open serial connection"""
        try:
            self.log(f"[SERIAL] Attempting to connect to {self.serial_port} @ {self.baud} baud...")
            self.ser = serial.Serial(
                port=self.serial_port,
                baudrate=self.baud,
                timeout=0.1,
                write_timeout=1.0
            )
            self.log(f"[SERIAL] ✓ Connected to {self.serial_port} @ {self.baud} baud")
            self.log(f"[SERIAL] Port is {'OPEN' if self.ser.is_open else 'CLOSED'}")
            self.log(f"[SERIAL] Write timeout: {self.ser.write_timeout}s, Read timeout: {self.ser.timeout}s")
            
            # Test write capability
            try:
                self.ser.write(b'\x00')  # Send a null byte as test
                self.ser.flush()
                self.log(f"[SERIAL] ✓ Write test successful - port is ready for transmission")
            except Exception as test_err:
                self.log(f"[WARN] Serial write test failed: {test_err}")
            
            return True
        except Exception as e:
            self.log(f"[ERROR] Serial connection failed: {e}")
            return False
    
    def connect_udp(self):
        """Open UDP socket"""
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.sock.bind(('0.0.0.0', self.udp_port))
            self.sock.settimeout(0.1)
            self.log(f"[UDP] Listening on 0.0.0.0:{self.udp_port}")
            return True
        except Exception as e:
            self.log(f"[ERROR] UDP bind failed: {e}")
            return False
    
    def serial_rx_thread(self):
        """Read serial data"""
        buffer = bytearray()
        
        while self.running:
            try:
                if self.ser and self.ser.in_waiting > 0:
                    data = self.ser.read(self.ser.in_waiting)
                    buffer.extend(data)
                    self.bytes_rx += len(data)
                    
                    # Process complete lines (Pico debug messages)
                    while b'\n' in buffer:
                        line_end = buffer.index(b'\n')
                        line = bytes(buffer[:line_end]).decode('utf-8', errors='ignore').strip()
                        buffer = buffer[line_end + 1:]
                        
                        if line:
                            # Only log lines that start with [DDPico] prefix
                            if line.startswith('[DDPico]'):
                                self.packets_rx += 1
                                # Forward Pico messages to web app
                                self.log(line, to_console=False)
                                self.last_activity = time.time()
                    
                    # Process COBS frames silently (DDP data packets)
                    while b'\x00' in buffer:
                        delim_idx = buffer.index(b'\x00')
                        buffer = buffer[delim_idx + 1:]
                
                time.sleep(0.001)
            except Exception as e:
                self.log(f"[ERROR] Serial RX: {e}")
                time.sleep(0.1)
    
    def udp_rx_thread(self):
        """Receive UDP packets and buffer for processing"""
        consecutive_errors = 0
        max_consecutive_errors = 5

        while self.running:
            try:
                data, addr = self.sock.recvfrom(4096)

                # Log received UDP packet details periodically
                if self.packets_rx % 100 == 0:  # Log every 100th packet
                    self.log(f"[UDP] Received {len(data)} bytes from {addr[0]}:{addr[1]}", to_console=False)

                # Log first few packets with FULL RAW HEX for debugging
                if self.packets_rx < 5 and len(data) >= 10:
                    flags = data[0]
                    seq = data[1]
                    dtype = data[2]
                    dest_id = data[3]
                    # 32-bit offset at bytes 4-7
                    offset = (data[4] << 24) | (data[5] << 16) | (data[6] << 8) | data[7]
                    # 16-bit length at bytes 8-9
                    length = (data[8] << 8) | data[9]

                    # Show first 20 bytes in hex
                    hex_dump = ' '.join(f'{b:02X}' for b in data[:min(20, len(data))])
                    self.log(f"[DDP] Packet #{self.packets_rx + 1} RAW: {hex_dump}")
                    self.log(f"[DDP] Parsed: flags=0x{flags:02X}, seq={seq}, type=0x{dtype:02X}, dest={dest_id}, offset={offset}, len={length}")

                # Buffer the frame for processing
                with self.frame_lock:
                    self.frame_buffer.append({
                        'timestamp': time.time(),
                        'data': data,
                        'addr': addr
                    })

                self.packets_rx += 1
                self.bytes_rx += len(data)
                self.last_activity = time.time()
                consecutive_errors = 0

            except socket.timeout:
                pass
            except Exception as e:
                self.log(f"[ERROR] UDP RX: {e}")
                time.sleep(0.1)

    def parse_ddp_and_update_state(self, data):
        """Parse DDP packet and update LED state"""
        if len(data) < 10:
            return False

        flags = data[0]
        seq = data[1]
        dtype = data[2]
        dest_id = data[3]
        offset = (data[4] << 24) | (data[5] << 16) | (data[6] << 8) | data[7]
        length = (data[8] << 8) | data[9]

        if len(data) < 10 + length:
            return False

        pixel_data = data[10:10 + length]

        # Only handle RGB data type and full frames (offset 0, length = num_leds * 3)
        if dtype != 0x01 or offset != 0 or length != self.num_leds * 3:
            return False

        # Update LED state
        self.prev_led_state = self.led_state[:]
        self.led_state = bytearray(pixel_data)

        return True

    def send_ddp_packet(self, pixel_data, push=False):
        """Send a DDP packet with given pixel data"""
        flags = 0x40 | (0x01 if push else 0x00)  # Version 1 + push if requested
        sequence = self.sequence & 0x0F
        self.sequence += 1
        data_type = 0x01  # RGB
        dest_id = 0x01
        offset = 0
        length = len(pixel_data)

        packet = bytes([
            flags,
            sequence,
            data_type,
            dest_id,
            (offset >> 24) & 0xFF, (offset >> 16) & 0xFF, (offset >> 8) & 0xFF, offset & 0xFF,
            (length >> 8) & 0xFF, length & 0xFF,
        ]) + pixel_data

        encoded = cobs_encode(packet)

        try:
            bytes_written = self.ser.write(encoded)
            self.ser.flush()
            self.packets_tx += 1
            self.bytes_tx += len(packet)
            return True
        except Exception as e:
            self.log(f"[ERROR] Serial write failed: {e}")
            return False

    def tweening_thread(self):
        """Process frames and handle tweening"""
        frame_interval = 1.0 / self.target_fps

        while self.running:
            with self.frame_lock:
                if self.frame_buffer:
                    frame = self.frame_buffer.popleft()
                    data = frame['data']

                    # Try to parse and update state
                    if self.parse_ddp_and_update_state(data):
                        if self.tweening_enabled and self.prev_led_state:
                            # Interpolate between prev and current
                            for step in range(self.tweening_steps + 1):
                                t = step / (self.tweening_steps + 1)
                                interpolated = bytearray()
                                for i in range(0, len(self.led_state), 3):
                                    r = int(self.prev_led_state[i] + (self.led_state[i] - self.prev_led_state[i]) * t)
                                    g = int(self.prev_led_state[i+1] + (self.led_state[i+1] - self.prev_led_state[i+1]) * t)
                                    b = int(self.prev_led_state[i+2] + (self.led_state[i+2] - self.prev_led_state[i+2]) * t)
                                    interpolated.extend([r, g, b])

                                push = (step == self.tweening_steps)  # Push on last frame
                                self.send_ddp_packet(interpolated, push)
                                time.sleep(frame_interval)
                        else:
                            # No tweening, send directly
                            self.send_ddp_packet(self.led_state, True)
                    else:
                        # Not a full RGB frame, send as is
                        encoded = cobs_encode(data)
                        try:
                            self.ser.write(encoded)
                            self.ser.flush()
                            self.packets_tx += 1
                            self.bytes_tx += len(data)
                        except Exception as e:
                            self.log(f"[ERROR] Serial write failed: {e}")

            time.sleep(0.001)  # Small delay to prevent tight loop

    def stats_thread(self):
        """Print stats periodically"""
        while self.running:
            time.sleep(10)
            elapsed = time.time() - self.last_activity
            
            # Check serial port status
            serial_status = "OPEN" if self.ser and self.ser.is_open else "CLOSED"
            
            # Log comprehensive stats
            self.log(f"[STATS] RX: {self.packets_rx} pkts, {self.bytes_rx} bytes | TX: {self.packets_tx} pkts, {self.bytes_tx} bytes | Serial: {serial_status} | Idle: {elapsed:.1f}s")
            
            # Warn if no packets are being sent despite receiving them
            if self.packets_tx > 0 and elapsed > 5:
                self.log(f"[INFO] No activity for {elapsed:.1f}s - waiting for DDP packets on UDP port {self.udp_port}", to_console=False)
    
    def send_test_sequence(self):
        """Send a 5-second test sequence to verify LED functionality"""
        self.log("[TEST] Starting 5-second LED test sequence...")
        
        num_leds = 100
        colors = [
            (255, 0, 0),    # Red
            (0, 255, 0),    # Green
            (0, 0, 255),    # Blue
            (255, 255, 0),  # Yellow
            (255, 0, 255),  # Magenta
        ]
        
        for color_idx, (r, g, b) in enumerate(colors):
            # Create DDP packet (10-byte header)
            # Header: Flags, Sequence, Type, DestID, Offset32, Length16
            flags = 0x41  # Version 1 (0x40) + Push (0x01) - correct DDP v1 flags
            sequence = color_idx & 0x0F
            data_type = 0x01  # RGB
            dest_id = 0x01
            
            # Create pixel data (all LEDs same color)
            pixel_data = bytes([r, g, b] * num_leds)
            data_len = len(pixel_data)
            
            # Build DDP packet (10-byte header)
            packet = bytes([
                flags,
                sequence,
                data_type,
                dest_id,
                0, 0, 0, 0,  # 32-bit data offset (big-endian) = 0
                (data_len >> 8) & 0xFF, data_len & 0xFF,  # 16-bit data length (big-endian)
            ]) + pixel_data
            
            # Encode and send
            encoded = cobs_encode(packet)
            self.ser.write(encoded)
            self.ser.flush()
            
            self.log(f"[TEST] Sent {['Red', 'Green', 'Blue', 'Yellow', 'Magenta'][color_idx]} to {num_leds} LEDs (flags: 0x{flags:02X})")
            time.sleep(1.0)
        
        # Clear LEDs
        pixel_data = bytes([0, 0, 0] * num_leds)
        data_len = len(pixel_data)
        packet = bytes([
            0x41, 0x0F, 0x01, 0x01,  # Correct DDP v1 flags: 0x41 = version 1 + push
            0, 0, 0, 0,  # 32-bit offset = 0
            (data_len >> 8) & 0xFF, data_len & 0xFF,  # 16-bit length
        ]) + pixel_data
        encoded = cobs_encode(packet)
        self.ser.write(encoded)
        self.ser.flush()
        
        self.log("[TEST] Test sequence complete - LEDs cleared")
    
    def run(self):
        """Main loop"""
        if not self.connect_serial():
            return
        if not self.connect_udp():
            return
        
        self.log("[BRIDGE] DDP Serial Bridge started")
        self.log("[CONFIG] Configure DDP software to send to 127.0.0.1:4048")
        
        # Start threads
        threads = [
            threading.Thread(target=self.serial_rx_thread, daemon=True),
            threading.Thread(target=self.udp_rx_thread, daemon=True),
            threading.Thread(target=self.tweening_thread, daemon=True),
            threading.Thread(target=self.stats_thread, daemon=True),
        ]
        
        for t in threads:
            t.start()
        
        # Wait a moment for threads to start
        time.sleep(0.5)
        
        # Send test sequence
        self.send_test_sequence()
        
        try:
            while self.running:
                time.sleep(0.1)
        except KeyboardInterrupt:
            print("\n[BRIDGE] Shutting down...")
            self.running = False
        
        self.cleanup()
    
    def cleanup(self):
        """Clean up resources"""
        if self.ser:
            self.ser.close()
        if self.sock:
            self.sock.close()
        self.log("[BRIDGE] Closed")


class WebHandler(BaseHTTPRequestHandler):
    bridge = None
    web_dir = None
    
    def log_message(self, format, *args):
        pass  # Suppress HTTP logs
    
    def do_GET(self):
        try:
            if self.path == '/':
                self.serve_file('index.html', 'text/html')

            elif self.path == '/settings':
                self.serve_settings()

            elif self.path == '/events':
                self.serve_events()

            elif self.path.startswith('/css/') or self.path.startswith('/js/'):
                # Serve static files
                file_path = self.path[1:]  # Remove leading slash
                mime_type, _ = mimetypes.guess_type(file_path)
                if mime_type:
                    self.serve_file(file_path, mime_type)
                else:
                    self.send_error(404)
            else:
                self.send_error(404)

        except (ConnectionAbortedError, ConnectionResetError, BrokenPipeError):
            pass  # Client disconnected during response

    def do_POST(self):
        try:
            if self.path == '/settings':
                self.update_settings()
            else:
                self.send_error(404)
        except (ConnectionAbortedError, ConnectionResetError, BrokenPipeError):
            pass
    
    def serve_file(self, filename, content_type):
        """Serve a static file from web directory"""
        try:
            file_path = os.path.join(self.web_dir, filename)
            with open(file_path, 'rb') as f:
                content = f.read()
            
            self.send_response(200)
            self.send_header('Content-type', content_type)
            self.send_header('Content-Length', len(content))
            self.end_headers()
            self.wfile.write(content)
        except FileNotFoundError:
            self.send_error(404, f"File not found: {filename}")
        except Exception as e:
            self.send_error(500, f"Server error: {str(e)}")
    
    def serve_events(self):
        """Serve Server-Sent Events stream"""
        self.send_response(200)
        self.send_header('Content-type', 'text/event-stream')
        self.send_header('Cache-Control', 'no-cache')
        self.send_header('Connection', 'keep-alive')
        self.end_headers()

        # Send initial status
        last_status = {
            'type': 'status',
            'connected': self.bridge.ser is not None and (self.bridge.ser.is_open if self.bridge.ser else False),
            'serial_port': self.bridge.serial_port
        }
        status_data = json.dumps(last_status)
        self.wfile.write(f"data: {status_data}\n\n".encode())

        # Send initial stats
        last_stats = {
            'type': 'stats',
            'packets_rx': self.bridge.packets_rx,
            'packets_tx': self.bridge.packets_tx,
            'bytes_rx': self.bridge.bytes_rx,
            'bytes_tx': self.bridge.bytes_tx
        }
        stats_data = json.dumps(last_stats)
        self.wfile.write(f"data: {stats_data}\n\n".encode())

        last_log_idx = 0

        try:
            while self.bridge.running:
                # Send new logs
                with self.bridge.log_lock:
                    while last_log_idx < len(self.bridge.log_buffer):
                        entry = self.bridge.log_buffer[last_log_idx]
                        data = json.dumps(entry)
                        self.wfile.write(f"data: {data}\n\n".encode())
                        last_log_idx += 1

                # Send status update only if changed
                current_status = {
                    'type': 'status',
                    'connected': self.bridge.ser is not None and (self.bridge.ser.is_open if self.bridge.ser else False),
                    'serial_port': self.bridge.serial_port
                }
                if current_status != last_status:
                    status_data = json.dumps(current_status)
                    self.wfile.write(f"data: {status_data}\n\n".encode())
                    last_status = current_status

                # Send stats update only if changed
                current_stats = {
                    'type': 'stats',
                    'packets_rx': self.bridge.packets_rx,
                    'packets_tx': self.bridge.packets_tx,
                    'bytes_rx': self.bridge.bytes_rx,
                    'bytes_tx': self.bridge.bytes_tx
                }
                if current_stats != last_stats:
                    stats_data = json.dumps(current_stats)
                    self.wfile.write(f"data: {stats_data}\n\n".encode())
                    last_stats = current_stats

                self.wfile.flush()
                time.sleep(0.1)
        except (ConnectionAbortedError, ConnectionResetError, BrokenPipeError):
            pass  # Client disconnected

    def serve_settings(self):
        """Serve current tweening settings as JSON"""
        with self.bridge.settings_lock:
            settings = {
                'tweening_enabled': self.bridge.tweening_enabled,
                'tweening_steps': self.bridge.tweening_steps,
                'target_fps': self.bridge.target_fps
            }

        self.send_response(200)
        self.send_header('Content-type', 'application/json')
        self.end_headers()
        self.wfile.write(json.dumps(settings).encode())

    def update_settings(self):
        """Update tweening settings from POST data"""
        try:
            content_length = int(self.headers['Content-Length'])
            post_data = self.rfile.read(content_length)
            settings = json.loads(post_data.decode())

            with self.bridge.settings_lock:
                if 'tweening_enabled' in settings:
                    self.bridge.tweening_enabled = bool(settings['tweening_enabled'])
                if 'tweening_steps' in settings:
                    self.bridge.tweening_steps = max(1, min(20, int(settings['tweening_steps'])))
                if 'target_fps' in settings:
                    self.bridge.target_fps = max(10, min(120, int(settings['target_fps'])))

            self.send_response(200)
            self.end_headers()
            self.wfile.write(b'OK')
        except Exception as e:
            self.send_error(400, f"Invalid settings: {str(e)}")


def auto_detect_serial():
    """Auto-detect Pico serial port"""
    try:
        import serial.tools.list_ports
        ports = list(serial.tools.list_ports.comports())
        
        # Look for Pico
        for port in ports:
            if 'Pico' in port.description or 'USB Serial' in port.description:
                return port.device
        
        # Fallback to first available
        if ports:
            return ports[0].device
    except:
        pass
    
    return None


if __name__ == '__main__':
    port = auto_detect_serial()
    if not port:
        print("[ERROR] Could not auto-detect serial port")
        sys.exit(1)
    
    bridge = DDPBridge(port)
    
    # Set up web handler
    WebHandler.bridge = bridge
    script_dir = os.path.dirname(os.path.abspath(__file__))
    WebHandler.web_dir = os.path.join(script_dir, 'web')
    
    # Check if web directory exists
    if not os.path.exists(WebHandler.web_dir):
        print(f"[ERROR] Web directory not found: {WebHandler.web_dir}")
        sys.exit(1)
    
    # Start web server with custom error handling
    class QuietHTTPServer(HTTPServer):
        def handle_error(self, request, client_address):
            """Override to suppress connection errors"""
            import sys
            exc_type, exc_value = sys.exc_info()[:2]
            if isinstance(exc_value, (ConnectionAbortedError, ConnectionResetError, BrokenPipeError)):
                pass  # Silently ignore connection errors
            else:
                super().handle_error(request, client_address)
    
    httpd = QuietHTTPServer(('0.0.0.0', bridge.web_port), WebHandler)
    web_thread = threading.Thread(target=httpd.serve_forever, daemon=True)
    web_thread.start()
    
    print(f"[WEB] Dashboard available at http://localhost:{bridge.web_port}")
    
    # Open browser
    threading.Timer(1.0, lambda: webbrowser.open(f'http://localhost:{bridge.web_port}')).start()
    
    bridge.run()
    httpd.shutdown()
