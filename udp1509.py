import socket
import threading
import time
from datetime import datetime
import json

class HeartbeatReceiver:
    def __init__(self, port=1509):
        self.port = port
        self.socket = None
        self.running = False
        self.esp_devices = {}  # Dictionary để lưu thông tin ESP
        
    def start_server(self):
        """Khởi tạo UDP server"""
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.socket.bind(('0.0.0.0', self.port))
            self.socket.settimeout(1.0)  # Timeout 1 giây
            self.running = True
            
            print(f"[HEARTBEAT_SERVER] Đang lắng nghe trên port {self.port}")
            print(f"[HEARTBEAT_SERVER] Server khởi tạo thành công!")
            print("=" * 60)
            
            return True
            
        except Exception as e:
            print(f"[HEARTBEAT_SERVER] Lỗi khởi tạo server: {e}")
            return False
    
    def stop_server(self):
        """Dừng UDP server"""
        self.running = False
        if self.socket:
            self.socket.close()
        print("[HEARTBEAT_SERVER] Server đã dừng")
    
    def parse_heartbeat_message(self, message):
        """Parse heartbeat message"""
        try:
            # Format: "HEARTBEAT:ESP32_A1B2C3D4E5F6,IP:192.168.0.100,HELLO"
            if message.startswith("HEARTBEAT:"):
                parts = message[10:].split(',')  # Bỏ "HEARTBEAT:"
                
                esp_id = parts[0] if len(parts) > 0 else "UNKNOWN"
                esp_ip = parts[1].split(':')[1] if len(parts) > 1 and ':' in parts[1] else "UNKNOWN"
                esp_status = parts[2] if len(parts) > 2 else "HELLO"
                
                return {
                    'type': 'HEARTBEAT',
                    'esp_id': esp_id,
                    'ip': esp_ip,
                    'status': esp_status,
                    'timestamp': datetime.now()
                }
                
            # Format: "STATUS:ESP32_A1B2C3D4E5F6,IP:192.168.0.100,READY"
            elif message.startswith("STATUS:"):
                parts = message[7:].split(',')  # Bỏ "STATUS:"
                
                esp_id = parts[0] if len(parts) > 0 else "UNKNOWN"
                esp_ip = parts[1].split(':')[1] if len(parts) > 1 and ':' in parts[1] else "UNKNOWN"
                esp_status = parts[2] if len(parts) > 2 else "UNKNOWN"
                
                return {
                    'type': 'STATUS',
                    'esp_id': esp_id,
                    'ip': esp_ip,
                    'status': esp_status,
                    'timestamp': datetime.now()
                }
            else:
                return {
                    'type': 'UNKNOWN',
                    'raw_message': message,
                    'timestamp': datetime.now()
                }
                
        except Exception as e:
            print(f"[PARSE_ERROR] Lỗi parse message: {e}")
            return None
    
    def update_esp_device(self, data):
        """Cập nhật thông tin ESP device"""
        if not data or 'esp_id' not in data:
            return
            
        esp_id = data['esp_id']
        current_time = datetime.now()
        
        if esp_id not in self.esp_devices:
            self.esp_devices[esp_id] = {
                'esp_id': esp_id,
                'ip': data.get('ip', 'UNKNOWN'),
                'status': data.get('status', 'UNKNOWN'),
                'first_seen': current_time,
                'last_seen': current_time,
                'heartbeat_count': 1,
                'online': True
            }
            print(f"[NEW_ESP] 🆕 ESP mới: {esp_id} - IP: {data.get('ip', 'UNKNOWN')}")
        else:
            # Cập nhật thông tin ESP hiện có
            self.esp_devices[esp_id].update({
                'ip': data.get('ip', self.esp_devices[esp_id]['ip']),
                'status': data.get('status', 'HELLO'),
                'last_seen': current_time,
                'heartbeat_count': self.esp_devices[esp_id]['heartbeat_count'] + 1,
                'online': True
            })
    
    def check_offline_devices(self):
        """Kiểm tra các ESP offline (không gửi heartbeat trong 5 giây)"""
        current_time = datetime.now()
        offline_threshold = 5  # 5 giây
        
        for esp_id, device in self.esp_devices.items():
            time_diff = (current_time - device['last_seen']).total_seconds()
            
            if time_diff > offline_threshold and device['online']:
                device['online'] = False
                print(f"[OFFLINE] ❌ ESP {esp_id} đã offline (không nhận heartbeat trong {time_diff:.1f}s)")
            elif time_diff <= offline_threshold and not device['online']:
                device['online'] = True
                print(f"[ONLINE] ✅ ESP {esp_id} đã online trở lại")
    
    def print_device_summary(self):
        """In tóm tắt các ESP devices"""
        print("\n" + "=" * 80)
        print(f"📊 TỔNG QUAN ESP DEVICES - {datetime.now().strftime('%H:%M:%S')}")
        print("=" * 80)
        
        if not self.esp_devices:
            print("❌ Chưa có ESP nào kết nối")
            return
        
        online_count = sum(1 for device in self.esp_devices.values() if device['online'])
        total_count = len(self.esp_devices)
        
        print(f"🔢 Tổng số ESP: {total_count} | Online: {online_count} | Offline: {total_count - online_count}")
        print("-" * 80)
        
        for esp_id, device in self.esp_devices.items():
            status_icon = "🟢" if device['online'] else "🔴"
            last_seen = device['last_seen'].strftime('%H:%M:%S')
            time_diff = (datetime.now() - device['last_seen']).total_seconds()
            
            print(f"{status_icon} {esp_id:<20} | IP: {device['ip']:<15} | Status: {device['status']:<10} | "
                  f"Last: {last_seen} ({time_diff:.1f}s) | Count: {device['heartbeat_count']}")
        
        print("=" * 80)
    
    def listen_for_heartbeats(self):
        """Lắng nghe heartbeat messages"""
        while self.running:
            try:
                data, addr = self.socket.recvfrom(1024)
                timestamp = datetime.now().strftime('%H:%M:%S.%f')[:-3]
                
                # ✅ IN DỮ LIỆU THÔ UDP
                print(f"[{timestamp}] 📥 RAW UDP từ {addr[0]}:{addr[1]}")
                print(f"  ├─ Bytes nhận: {len(data)} bytes")
                print(f"  ├─ Raw bytes: {data}")
                print(f"  └─ Decoded: '{data.decode('utf-8').strip()}'")
                print("-" * 60)
                
                message = data.decode('utf-8').strip()
                
                # Parse message
                parsed_data = self.parse_heartbeat_message(message)
                
                if parsed_data:
                    # Cập nhật ESP device
                    self.update_esp_device(parsed_data)
                    
                    # In thông tin parsed
                    if parsed_data['type'] == 'HEARTBEAT':
                        print(f"[{timestamp}] 💓 PARSED: {parsed_data['esp_id']} ({parsed_data['ip']}) - {parsed_data['status']}")
                    elif parsed_data['type'] == 'STATUS':
                        print(f"[{timestamp}] 📋 PARSED: {parsed_data['esp_id']} ({parsed_data['ip']}) - STATUS: {parsed_data['status']}")
                    else:
                        print(f"[{timestamp}] ❓ PARSED: Unknown message type")
                else:
                    print(f"[{timestamp}] ❌ PARSE FAILED: Không parse được message")
                
                print("=" * 60)
                
            except socket.timeout:
                # Timeout bình thường, tiếp tục loop
                continue
            except UnicodeDecodeError as e:
                timestamp = datetime.now().strftime('%H:%M:%S.%f')[:-3]
                print(f"[{timestamp}] 🔴 DECODE ERROR từ {addr}: {e}")
                print(f"  └─ Raw bytes: {data}")
                print("-" * 60)
            except Exception as e:
                if self.running:
                    timestamp = datetime.now().strftime('%H:%M:%S.%f')[:-3]
                    print(f"[{timestamp}] 🔴 LISTEN ERROR: {e}")
                    print("-" * 60)
    
    def monitor_devices(self):
        """Monitor thread để kiểm tra offline devices và in summary"""
        summary_interval = 30  # In summary mỗi 30 giây (tăng từ 10s để ít spam hơn)
        last_summary_time = 0
        
        while self.running:
            current_time = time.time()
            
            # Kiểm tra offline devices
            self.check_offline_devices()
            
            # In summary định kỳ
            if current_time - last_summary_time >= summary_interval:
                self.print_device_summary()
                last_summary_time = current_time
            
            time.sleep(1)  # Kiểm tra mỗi giây
    
    def run(self):
        """Chạy heartbeat receiver"""
        if not self.start_server():
            return
        
        try:
            # Tạo thread cho listening
            listen_thread = threading.Thread(target=self.listen_for_heartbeats, daemon=True)
            listen_thread.start()
            
            # Tạo thread cho monitoring
            monitor_thread = threading.Thread(target=self.monitor_devices, daemon=True)
            monitor_thread.start()
            
            print("🎯 Server đang chạy. Nhấn Ctrl+C để dừng...")
            print("🔍 Sẽ hiển thị dữ liệu UDP thô nhận được...")
            print("=" * 60)
            
            # Chạy cho đến khi bị ngắt
            while self.running:
                time.sleep(1)
                
        except KeyboardInterrupt:
            print("\n[SHUTDOWN] Đang dừng server...")
            self.stop_server()
        except Exception as e:
            print(f"[ERROR] Lỗi không mong đợi: {e}")
            self.stop_server()

def main():
    print("🚀 ESP32 Heartbeat Receiver v1.1 - RAW UDP Mode")
    print("📡 Nhận và hiển thị dữ liệu UDP thô từ ESP32 trên port 1509")
    print("-" * 50)
    
    receiver = HeartbeatReceiver(port=1509)
    receiver.run()

if __name__ == "__main__":
    main()