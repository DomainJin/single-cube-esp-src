#!/usr/bin/env python3
"""
ESP32 Detailed System Schematic Generator
Vẽ sơ đồ nguyên lý chi tiết với đầy đủ thông tin GPIO, module diagrams và bảng summary
Style tương tự ảnh tham khảo
"""

import cv2
import numpy as np
import os

# ============================================
# CONFIGURATION
# ============================================

ESP32_PINOUT_IMAGE = "esp_pinout.png"
OUTPUT_IMAGE = "ESP32_Detailed_System_Schematic.png"

WIDTH = 3800
HEIGHT = 2400
BG_COLOR = (248, 248, 250)  # Light gray background
MARGIN = 60

# Colors
COLOR_BORDER = (80, 80, 100)
COLOR_TEXT = (30, 30, 40)
COLOR_GPIO_LEFT = (220, 60, 220)   # Magenta for left pins
COLOR_GPIO_RIGHT = (220, 60, 220)  # Magenta for right pins
COLOR_PIN_NUM = (40, 40, 50)       # Dark for pin numbers
COLOR_GND = (60, 60, 70)           # Dark gray for GND
COLOR_POWER = (200, 50, 50)        # Red for power

# Module colors
MODULE_COLORS = {
    'MOTOR_1': (100, 150, 255),    # Blue
    'MOTOR_2': (255, 140, 80),     # Orange
    'MOTOR_3': (255, 100, 140),    # Pink
    'I2C': (150, 120, 255),        # Purple
    'LED': (255, 200, 100),        # Yellow
    'IR': (100, 255, 150),         # Green
    'BUTTON': (180, 220, 255),     # Light Blue
    'XILANH': (255, 100, 100)      # Red
}

# Font settings
FONT = cv2.FONT_HERSHEY_SIMPLEX
FONT_TITLE = 1.4
FONT_SUBTITLE = 0.75
FONT_NORMAL = 0.55
FONT_SMALL = 0.45
FONT_TINY = 0.35

# ============================================
# GPIO MAPPING DATA
# ============================================

# Complete GPIO pin mapping with all details
GPIO_DATA = [
    # GPIO, Function, Type, Module, Side, Pin#
    (2, 'Xilanh Output 1', 'OUTPUT', 'Main (Pneumatic)', 'RIGHT', 22),
    (4, 'Motor 1 IN1', 'PWM OUT', 'Motor Control (L298N)', 'RIGHT', 24),
    (5, 'WS2812 LED Data', 'OUTPUT', 'LED Strip', 'RIGHT', 34),
    (13, 'Motor 1 IN2', 'PWM OUT', 'Motor Control (L298N)', 'LEFT', 20),
    (14, 'Motor 1 Encoder A', 'INPUT', 'Motor Control (Encoder)', 'LEFT', 17),
    (15, 'Xilanh Output 2', 'OUTPUT', 'Main (Pneumatic)', 'RIGHT', 21),
    (16, 'I2C SDA', 'I2C', 'QMC5883L Compass', 'RIGHT', 25),
    (17, 'I2C SCL', 'I2C', 'QMC5883L Compass', 'RIGHT', 27),
    (18, 'Motor 2 IN1', 'PWM OUT', 'Motor Control (L298N)', 'RIGHT', 35),
    (19, 'Motor 2 IN2', 'PWM OUT', 'Motor Control (L298N)', 'RIGHT', 38),
    (21, 'Motor 2 Encoder A', 'INPUT', 'Motor Control (Encoder)', 'RIGHT', 42),
    (22, 'Motor 2 Encoder B', 'INPUT', 'Motor Control (Encoder)', 'RIGHT', 40),
    (23, 'Motor 3 IN1', 'PWM OUT', 'Motor Control (L298N)', 'RIGHT', 37),
    (25, 'Button Next', 'INPUT', 'A4L Controller', 'LEFT', 14),
    (26, 'Button Sync', 'INPUT', 'A4L Controller', 'LEFT', 15),
    (27, 'Button HDMI', 'INPUT', 'A4L Controller', 'LEFT', 16),
    (32, 'Motor 1 Encoder B (ADC)', 'INPUT+ADC', 'Motor Control (Encoder)', 'LEFT', 12),
    (33, 'Motor 3 IN2 (ADC)', 'PWM+ADC', 'Motor Control (L298N)', 'LEFT', 13),
    (34, 'IR Sensor 1', 'ADC IN', 'IR Proximity (Input)', 'LEFT', 10),
    (35, 'IR Sensor 2', 'ADC IN', 'IR Proximity (Input)', 'LEFT', 11),
    (36, 'Motor 3 Encoder A', 'IN+ADC', 'Motor Control (Encoder)', 'LEFT', 5),
    (39, 'Motor 3 Encoder B', 'IN+ADC', 'Motor Control (Encoder)', 'LEFT', 8),
]

# ============================================
# HELPER FUNCTIONS
# ============================================

def load_esp32_pinout():
    """Load ESP32 pinout image"""
    if not os.path.exists(ESP32_PINOUT_IMAGE):
        print(f"⚠️  Không tìm thấy: {ESP32_PINOUT_IMAGE}")
        return None
    
    img = cv2.imread(ESP32_PINOUT_IMAGE)
    if img is None:
        print(f"❌ Không đọc được: {ESP32_PINOUT_IMAGE}")
        return None
    
    # Keep original size or slight resize for quality
    target_height = 700
    aspect = img.shape[1] / img.shape[0]
    target_width = int(target_height * aspect)
    resized = cv2.resize(img, (target_width, target_height), interpolation=cv2.INTER_LANCZOS4)
    
    print(f"✅ Loaded ESP32 image: {target_width}x{target_height}")
    return resized

def draw_rounded_box(img, x, y, w, h, color, thickness=-1, radius=12):
    """Draw rounded rectangle"""
    if thickness == -1:  # Filled
        cv2.rectangle(img, (x+radius, y), (x+w-radius, y+h), color, -1)
        cv2.rectangle(img, (x, y+radius), (x+w, y+h-radius), color, -1)
        cv2.circle(img, (x+radius, y+radius), radius, color, -1)
        cv2.circle(img, (x+w-radius, y+radius), radius, color, -1)
        cv2.circle(img, (x+radius, y+h-radius), radius, color, -1)
        cv2.circle(img, (x+w-radius, y+h-radius), radius, color, -1)
    else:  # Outline
        cv2.line(img, (x+radius, y), (x+w-radius, y), color, thickness)
        cv2.line(img, (x+radius, y+h), (x+w-radius, y+h), color, thickness)
        cv2.line(img, (x, y+radius), (x, y+h-radius), color, thickness)
        cv2.line(img, (x+w, y+radius), (x+w, y+h-radius), color, thickness)
        cv2.ellipse(img, (x+radius, y+radius), (radius, radius), 180, 0, 90, color, thickness)
        cv2.ellipse(img, (x+w-radius, y+radius), (radius, radius), 270, 0, 90, color, thickness)
        cv2.ellipse(img, (x+radius, y+h-radius), (radius, radius), 90, 0, 90, color, thickness)
        cv2.ellipse(img, (x+w-radius, y+h-radius), (radius, radius), 0, 0, 90, color, thickness)

def draw_motor_icon(img, x, y, size, color):
    """Draw simple motor icon"""
    center_x, center_y = x + size//2, y + size//2
    radius = size // 3
    
    # Motor circle
    cv2.circle(img, (center_x, center_y), radius, color, 3)
    
    # Motor shaft lines
    cv2.line(img, (center_x - radius, center_y), (center_x + radius, center_y), color, 2)
    cv2.line(img, (center_x, center_y - radius), (center_x, center_y + radius), color, 2)
    
    # 4 terminals
    terminal_offset = int(radius * 0.7)
    for dx, dy in [(-1, -1), (1, -1), (-1, 1), (1, 1)]:
        tx = center_x + dx * terminal_offset
        ty = center_y + dy * terminal_offset
        cv2.circle(img, (tx, ty), 4, color, -1)

def draw_led_icon(img, x, y, size, color):
    """Draw LED strip icon"""
    num_leds = 4
    spacing = size // (num_leds + 1)
    for i in range(num_leds):
        led_x = x + spacing * (i + 1)
        led_y = y + size // 2
        cv2.circle(img, (led_x, led_y), 8, color, -1)
        cv2.circle(img, (led_x, led_y), 10, color, 2)

def draw_button_icon(img, x, y, size, color):
    """Draw button icon"""
    button_size = size // 4
    spacing = size // 3
    for i in range(3):
        btn_x = x + size // 2
        btn_y = y + spacing * (i + 1)
        cv2.rectangle(img, (btn_x - button_size, btn_y - 8), 
                     (btn_x + button_size, btn_y + 8), color, -1)
        cv2.rectangle(img, (btn_x - button_size, btn_y - 8), 
                     (btn_x + button_size, btn_y + 8), COLOR_BORDER, 2)

def draw_sensor_icon(img, x, y, size, color):
    """Draw IR sensor icon"""
    center_x, center_y = x + size//2, y + size//2
    # Two sensors
    for offset in [-15, 15]:
        cv2.circle(img, (center_x + offset, center_y), 12, color, -1)
        cv2.circle(img, (center_x + offset, center_y), 14, COLOR_BORDER, 2)
        # IR rays
        for angle in range(0, 180, 30):
            rad = np.radians(angle + 90)
            ex = int(center_x + offset + np.cos(rad) * 20)
            ey = int(center_y + np.sin(rad) * 20)
            cv2.line(img, (center_x + offset, center_y), (ex, ey), color, 1)

def draw_xilanh_icon(img, x, y, size, color):
    """Draw pneumatic cylinder icon"""
    center_x, center_y = x + size//2, y + size//2
    # Cylinder body
    cv2.rectangle(img, (center_x - 25, center_y - 15), 
                 (center_x + 25, center_y + 15), color, -1)
    cv2.rectangle(img, (center_x - 25, center_y - 15), 
                 (center_x + 25, center_y + 15), COLOR_BORDER, 2)
    # Piston rod
    cv2.line(img, (center_x + 25, center_y), (center_x + 45, center_y), color, 5)
    cv2.circle(img, (center_x + 45, center_y), 6, color, -1)

def draw_compass_icon(img, x, y, size, color):
    """Draw compass icon"""
    center_x, center_y = x + size//2, y + size//2
    radius = size // 3
    cv2.circle(img, (center_x, center_y), radius, color, 3)
    # North arrow
    cv2.line(img, (center_x, center_y), (center_x, center_y - radius + 5), (200, 50, 50), 3)
    cv2.circle(img, (center_x, center_y - radius + 5), 5, (200, 50, 50), -1)
    # Cross
    cv2.line(img, (center_x - radius + 10, center_y), (center_x + radius - 10, center_y), color, 1)
    cv2.line(img, (center_x, center_y - radius + 10), (center_x, center_y + radius - 10), color, 1)

def draw_module_box(img, x, y, module_type, title, pins_info, color):
    """Draw detailed module box with icon and pins"""
    width = 300
    header_h = 90
    pin_h = 28
    height = header_h + len(pins_info) * pin_h + 20
    
    # Shadow
    draw_rounded_box(img, x+6, y+6, width, height, (200, 200, 210), -1, 15)
    
    # Main box
    draw_rounded_box(img, x, y, width, height, (255, 255, 255), -1, 15)
    draw_rounded_box(img, x, y, width, height, COLOR_BORDER, 3, 15)
    
    # Header with color
    draw_rounded_box(img, x+8, y+8, width-16, header_h-8, color, -1, 10)
    
    # Module title
    cv2.putText(img, title, (x + 20, y + 35), FONT, FONT_NORMAL, (255, 255, 255), 2)
    
    # Draw icon
    icon_size = 50
    icon_x = x + width - icon_size - 15
    icon_y = y + 15
    
    if 'MOTOR' in module_type:
        draw_motor_icon(img, icon_x, icon_y, icon_size, (255, 255, 255))
    elif module_type == 'LED':
        draw_led_icon(img, icon_x, icon_y, icon_size, (255, 255, 255))
    elif module_type == 'BUTTON':
        draw_button_icon(img, icon_x, icon_y, icon_size, (255, 255, 255))
    elif module_type == 'IR':
        draw_sensor_icon(img, icon_x, icon_y, icon_size, (255, 255, 255))
    elif module_type == 'XILANH':
        draw_xilanh_icon(img, icon_x, icon_y, icon_size, (255, 255, 255))
    elif module_type == 'I2C':
        draw_compass_icon(img, icon_x, icon_y, icon_size, (255, 255, 255))
    
    # Draw pins
    pin_positions = []
    py = y + header_h + 10
    
    for pin_label, gpio in pins_info:
        # Pin circle (connection point)
        pin_y = py + 10
        cv2.circle(img, (x + 15, pin_y), 6, color, -1)
        cv2.circle(img, (x + 15, pin_y), 8, COLOR_BORDER, 2)
        
        # Pin label
        cv2.putText(img, pin_label, (x + 35, pin_y + 5), FONT, FONT_SMALL, COLOR_TEXT, 1)
        
        # GPIO number (right aligned)
        gpio_text = f"GPIO{gpio}"
        text_size = cv2.getTextSize(gpio_text, FONT, FONT_SMALL, 1)[0]
        cv2.putText(img, gpio_text, (x + width - text_size[0] - 20, pin_y + 5), 
                   FONT, FONT_SMALL, (120, 120, 130), 1)
        
        pin_positions.append((gpio, x + 15, pin_y))
        py += pin_h
    
    return pin_positions, height

def draw_connection_line(img, x1, y1, x2, y2, color, gpio_num, side='left', layer=0):
    """Draw orthogonal connection line with smart routing"""
    thickness = 2
    
    # Calculate routing points for orthogonal lines
    if side == 'left':
        # Left side: module -> left -> vertical -> horizontal -> GPIO
        offset = 80 + layer * 25  # Offset for each layer to avoid overlap
        points = [
            (x1, y1),                    # Module pin
            (x1 - offset, y1),           # Horizontal from module
            (x1 - offset, y2),           # Vertical alignment
            (x2, y2)                     # To GPIO label
        ]
    else:
        # Right side: module -> right -> vertical -> horizontal -> GPIO  
        offset = 80 + layer * 25
        points = [
            (x1, y1),                    # Module pin
            (x1 + offset, y1),           # Horizontal from module
            (x1 + offset, y2),           # Vertical alignment
            (x2, y2)                     # To GPIO label
        ]
    
    # Draw orthogonal lines
    for i in range(len(points) - 1):
        cv2.line(img, points[i], points[i+1], color, thickness, cv2.LINE_AA)
    
    # Draw connection dots
    cv2.circle(img, (x1, y1), 5, color, -1)
    cv2.circle(img, (x2, y2), 5, color, -1)
    
    # Label at midpoint
    mid_idx = len(points) // 2
    if mid_idx < len(points):
        lx, ly = points[mid_idx]
        label = f"GPIO{gpio_num}"
        text_size = cv2.getTextSize(label, FONT, FONT_TINY, 1)[0]
        
        # Background box
        pad = 4
        cv2.rectangle(img, (lx - text_size[0]//2 - pad, ly - text_size[1]//2 - pad),
                     (lx + text_size[0]//2 + pad, ly + text_size[1]//2 + pad),
                     BG_COLOR, -1)
        cv2.rectangle(img, (lx - text_size[0]//2 - pad, ly - text_size[1]//2 - pad),
                     (lx + text_size[0]//2 + pad, ly + text_size[1]//2 + pad),
                     color, 1)
        cv2.putText(img, label, (lx - text_size[0]//2, ly + text_size[1]//2),
                   FONT, FONT_TINY, COLOR_TEXT, 1)

def draw_gpio_table(img, x, y):
    """Draw comprehensive GPIO table"""
    # Title
    cv2.putText(img, "GPIO PIN ASSIGNMENTS", (x, y), FONT, FONT_SUBTITLE, COLOR_TEXT, 2)
    
    # Table dimensions
    col_widths = [60, 220, 120, 280]
    row_height = 28
    total_width = sum(col_widths)
    
    # Header
    headers = ["GPIO", "Function", "Type", "Module"]
    header_y = y + 40
    
    # Header background
    draw_rounded_box(img, x, header_y - 22, total_width, row_height, (220, 220, 230), -1, 8)
    
    # Draw headers
    hx = x + 10
    for i, header in enumerate(headers):
        cv2.putText(img, header, (hx, header_y), FONT, FONT_SMALL, COLOR_TEXT, 2)
        hx += col_widths[i]
    
    # Draw table rows
    current_y = header_y + 10
    for gpio, function, pin_type, module, side, pin_num in sorted(GPIO_DATA, key=lambda x: x[0]):
        current_y += row_height
        
        # Alternating row colors
        if gpio % 2 == 0:
            cv2.rectangle(img, (x, current_y - 20), (x + total_width, current_y + 5),
                         (242, 242, 245), -1)
        
        # Draw row data
        row_x = x + 10
        
        # GPIO number (yellow background)
        gpio_text = str(gpio)
        draw_rounded_box(img, row_x - 5, current_y - 15, 40, 20, (255, 220, 100), -1, 5)
        cv2.putText(img, gpio_text, (row_x, current_y), FONT, FONT_SMALL, COLOR_TEXT, 2)
        row_x += col_widths[0]
        
        # Function
        cv2.putText(img, function, (row_x, current_y), FONT, FONT_TINY, COLOR_TEXT, 1)
        row_x += col_widths[1]
        
        # Type
        type_color = (100, 180, 100) if 'INPUT' in pin_type else (180, 100, 100) if 'OUTPUT' in pin_type else (100, 100, 180)
        cv2.putText(img, pin_type, (row_x, current_y), FONT, FONT_TINY, type_color, 1)
        row_x += col_widths[2]
        
        # Module
        cv2.putText(img, module, (row_x, current_y), FONT, FONT_TINY, (100, 100, 110), 1)
    
    # Border
    table_height = current_y - header_y + 30
    draw_rounded_box(img, x, header_y - 22, total_width, table_height, COLOR_BORDER, 2, 8)

# ============================================
# MAIN SCHEMATIC GENERATOR
# ============================================

def create_detailed_schematic():
    """Create complete detailed schematic"""
    print("=" * 70)
    print("ESP32 DETAILED SYSTEM SCHEMATIC GENERATOR")
    print("=" * 70)
    
    # Create canvas
    img = np.ones((HEIGHT, WIDTH, 3), dtype=np.uint8)
    img[:] = BG_COLOR
    
    # Border
    cv2.rectangle(img, (MARGIN, MARGIN), (WIDTH-MARGIN, HEIGHT-MARGIN), COLOR_BORDER, 3)
    
    # ===== TITLE =====
    title = "ESP32 COMPLETE SYSTEM SCHEMATIC"
    title_size = cv2.getTextSize(title, FONT, FONT_TITLE, 3)[0]
    title_x = (WIDTH - title_size[0]) // 2
    cv2.putText(img, title, (title_x, 120), FONT, FONT_TITLE, COLOR_TEXT, 3)
    
    subtitle = "Single Cube Project - Motor Control System with Sensors & Communication"
    subtitle_size = cv2.getTextSize(subtitle, FONT, FONT_SUBTITLE, 2)[0]
    subtitle_x = (WIDTH - subtitle_size[0]) // 2
    cv2.putText(img, subtitle, (subtitle_x, 160), FONT, FONT_SUBTITLE, (120, 120, 130), 2)
    
    # ===== LOAD ESP32 IMAGE =====
    print("\n📷 Loading ESP32 pinout...")
    esp32_img = load_esp32_pinout()
    
    esp32_x = WIDTH // 2 - 385
    esp32_y = 250
    
    if esp32_img is not None:
        h, w = esp32_img.shape[:2]
        
        # Shadow
        cv2.rectangle(img, (esp32_x + 6, esp32_y + 6), (esp32_x + w + 6, esp32_y + h + 6),
                     (200, 200, 210), -1)
        
        # White background
        cv2.rectangle(img, (esp32_x - 10, esp32_y - 10), (esp32_x + w + 10, esp32_y + h + 10),
                     (255, 255, 255), -1)
        
        # Border
        cv2.rectangle(img, (esp32_x - 10, esp32_y - 10), (esp32_x + w + 10, esp32_y + h + 10),
                     COLOR_BORDER, 4)
        
        # Image
        img[esp32_y:esp32_y+h, esp32_x:esp32_x+w] = esp32_img
        
        esp32_width = w
        esp32_height = h
    else:
        esp32_width = 770
        esp32_height = 700
        h, w = esp32_height, esp32_width
    
    # ===== DRAW MODULES =====
    print("\n📦 Drawing modules...")
    
    all_pins = []
    
    # Left side modules
    left_x = 150
    left_y = 280
    
    # Motor 1
    pins, h1 = draw_module_box(img, left_x, left_y, 'MOTOR_1', 'MOTOR 1',
                                [('IN1 (PWM)', 4), ('IN2 (PWM)', 13), 
                                 ('Encoder A', 14), ('Encoder B', 32)],
                                MODULE_COLORS['MOTOR_1'])
    all_pins.extend(pins)
    left_y += h1 + 30
    
    # IR Sensors
    pins, h2 = draw_module_box(img, left_x, left_y, 'IR', 'IR SENSORS',
                                [('ADC1', 34), ('ADC2', 35)],
                                MODULE_COLORS['IR'])
    all_pins.extend(pins)
    left_y += h2 + 30
    
    # Buttons
    pins, h3 = draw_module_box(img, left_x, left_y, 'BUTTON', 'A4L BUTTONS',
                                [('Next', 25), ('Sync', 26), ('HDMI', 27)],
                                MODULE_COLORS['BUTTON'])
    all_pins.extend(pins)
    left_y += h3 + 30
    
    # Motor 3
    pins, h4 = draw_module_box(img, left_x, left_y, 'MOTOR_3', 'MOTOR 3',
                                [('IN1 (PWM)', 23), ('IN2 (PWM)', 33),
                                 ('Encoder A', 36), ('Encoder B', 39)],
                                MODULE_COLORS['MOTOR_3'])
    all_pins.extend(pins)
    
    # Right side modules
    right_x = WIDTH - 470
    right_y = 280
    
    # Motor 2
    pins, h5 = draw_module_box(img, right_x, right_y, 'MOTOR_2', 'MOTOR 2',
                                [('IN1 (PWM)', 18), ('IN2 (PWM)', 19),
                                 ('Encoder A', 21), ('Encoder B', 22)],
                                MODULE_COLORS['MOTOR_2'])
    all_pins.extend(pins)
    right_y += h5 + 30
    
    # LED
    pins, h6 = draw_module_box(img, right_x, right_y, 'LED', 'WS2812 LED',
                                [('Data', 5)],
                                MODULE_COLORS['LED'])
    all_pins.extend(pins)
    right_y += h6 + 30
    
    # I2C Compass
    pins, h7 = draw_module_box(img, right_x, right_y, 'I2C', 'QMC5883L COMPASS',
                                [('SDA', 16), ('SCL', 17)],
                                MODULE_COLORS['I2C'])
    all_pins.extend(pins)
    right_y += h7 + 30
    
    # Xilanh
    pins, h8 = draw_module_box(img, right_x, right_y, 'XILANH', 'XILANH CONTROL',
                                [('Output 1', 2), ('Output 2', 15)],
                                MODULE_COLORS['XILANH'])
    all_pins.extend(pins)
    
    # ===== DRAW CONNECTIONS =====
    print("🔌 Drawing connections...")
    
    # GPIO label positions - đo CHÍNH XÁC từ ảnh esp_pinout.png
    # Nhìn vào ảnh: GPIO labels màu tím nằm ở vị trí cố định
    # Left side: x ở khoảng 115-125px từ mép trái ESP32 image
    # Right side: x ở khoảng 705-715px từ mép trái ESP32 image
    # Y bắt đầu: ~100px, spacing: ~35px
    
    left_gpio_x = 118   # Giữa của GPIO labels màu tím bên trái
    right_gpio_x = 710  # Giữa của GPIO labels màu tím bên phải
    gpio_start_y = 100  # Y của GPIO đầu tiên
    gpio_spacing = 35   # Khoảng cách giữa các GPIO
    
    gpio_positions = {
        # Left side GPIO labels - từ trên xuống theo thứ tự trên ảnh
        36: (esp32_x + left_gpio_x, esp32_y + gpio_start_y + gpio_spacing * 0),   # GPIO36
        39: (esp32_x + left_gpio_x, esp32_y + gpio_start_y + gpio_spacing * 1),   # GPIO39  
        34: (esp32_x + left_gpio_x, esp32_y + gpio_start_y + gpio_spacing * 2),   # GPIO34
        35: (esp32_x + left_gpio_x, esp32_y + gpio_start_y + gpio_spacing * 3),   # GPIO35
        32: (esp32_x + left_gpio_x, esp32_y + gpio_start_y + gpio_spacing * 4),   # GPIO32
        33: (esp32_x + left_gpio_x, esp32_y + gpio_start_y + gpio_spacing * 5),   # GPIO33
        25: (esp32_x + left_gpio_x, esp32_y + gpio_start_y + gpio_spacing * 6),   # GPIO25
        26: (esp32_x + left_gpio_x, esp32_y + gpio_start_y + gpio_spacing * 7),   # GPIO26
        27: (esp32_x + left_gpio_x, esp32_y + gpio_start_y + gpio_spacing * 8),   # GPIO27
        14: (esp32_x + left_gpio_x, esp32_y + gpio_start_y + gpio_spacing * 9),   # GPIO14
        12: (esp32_x + left_gpio_x, esp32_y + gpio_start_y + gpio_spacing * 10),  # GPIO12
        13: (esp32_x + left_gpio_x, esp32_y + gpio_start_y + gpio_spacing * 11),  # GPIO13
        # Right side GPIO labels - từ trên xuống theo thứ tự trên ảnh  
        23: (esp32_x + right_gpio_x, esp32_y + gpio_start_y + gpio_spacing * 0),   # GPIO23
        22: (esp32_x + right_gpio_x, esp32_y + gpio_start_y + gpio_spacing * 1),   # GPIO22
        21: (esp32_x + right_gpio_x, esp32_y + gpio_start_y + gpio_spacing * 2),   # GPIO21
        19: (esp32_x + right_gpio_x, esp32_y + gpio_start_y + gpio_spacing * 3),   # GPIO19
        18: (esp32_x + right_gpio_x, esp32_y + gpio_start_y + gpio_spacing * 4),   # GPIO18
        5: (esp32_x + right_gpio_x, esp32_y + gpio_start_y + gpio_spacing * 5),    # GPIO5
        17: (esp32_x + right_gpio_x, esp32_y + gpio_start_y + gpio_spacing * 6),   # GPIO17
        16: (esp32_x + right_gpio_x, esp32_y + gpio_start_y + gpio_spacing * 7),   # GPIO16
        4: (esp32_x + right_gpio_x, esp32_y + gpio_start_y + gpio_spacing * 8),    # GPIO4
        0: (esp32_x + right_gpio_x, esp32_y + gpio_start_y + gpio_spacing * 9),    # GPIO0
        2: (esp32_x + right_gpio_x, esp32_y + gpio_start_y + gpio_spacing * 10),   # GPIO2
        15: (esp32_x + right_gpio_x, esp32_y + gpio_start_y + gpio_spacing * 11),  # GPIO15
    }
    
    # Group pins by side and sort for layer routing
    left_pins = [(gpio, px, py) for gpio, px, py in all_pins if gpio in [36, 39, 34, 35, 32, 33, 25, 26, 27, 14, 12, 13]]
    right_pins = [(gpio, px, py) for gpio, px, py in all_pins if gpio in [23, 22, 21, 19, 18, 5, 17, 16, 4, 0, 2, 15]]
    
    # Draw left side connections with layering
    left_pins_sorted = sorted(left_pins, key=lambda x: x[2])  # Sort by Y position
    for idx, (gpio, pin_x, pin_y) in enumerate(left_pins_sorted):
        if gpio in gpio_positions:
            esp_x, esp_y = gpio_positions[gpio]
            module_color = MODULE_COLORS.get('MOTOR_1', (150, 150, 160))
            for module_name, color in MODULE_COLORS.items():
                if any(g == gpio for g, _, _ in all_pins):
                    module_color = color
                    break
            draw_connection_line(img, pin_x, pin_y, esp_x, esp_y, module_color, gpio, 'left', idx)
    
    # Draw right side connections with layering  
    right_pins_sorted = sorted(right_pins, key=lambda x: x[2])  # Sort by Y position
    for idx, (gpio, pin_x, pin_y) in enumerate(right_pins_sorted):
        if gpio in gpio_positions:
            esp_x, esp_y = gpio_positions[gpio]
            module_color = MODULE_COLORS.get('MOTOR_2', (150, 150, 160))
            for module_name, color in MODULE_COLORS.items():
                if any(g == gpio for g, _, _ in all_pins):
                    module_color = color
                    break
            draw_connection_line(img, pin_x, pin_y, esp_x, esp_y, module_color, gpio, 'right', idx)
    
    # ===== GPIO TABLE =====
    print("📋 Drawing GPIO table...")
    table_x = 150
    table_y = HEIGHT - 680
    draw_gpio_table(img, table_x, table_y)
    
    # ===== SPECS BOX =====
    specs_x = WIDTH - 550
    specs_y = HEIGHT - 320
    
    draw_rounded_box(img, specs_x, specs_y, 450, 250, (255, 255, 255), -1, 12)
    draw_rounded_box(img, specs_x, specs_y, 450, 250, COLOR_BORDER, 3, 12)
    
    cv2.putText(img, "SPECIFICATIONS", (specs_x + 20, specs_y + 35),
               FONT, FONT_SUBTITLE, COLOR_TEXT, 2)
    
    specs = [
        "Board: ESP32 NodeMCU-32S (ESP-32S Kit)",
        "GPIO Active: 22 / 34 (64.7%)",
        "",
        "Motor Control:",
        "  - PWM: 5kHz, 8-bit resolution (0-255)",
        "  - Encoder: 20 PPR, 2ms debounce",
        "  - Driver: L298N H-Bridge",
        "",
        "Communication:",
        "  - I2C: 100kHz (QMC5883L @ 0x0D)",
        "  - WiFi: UDP to 192.168.0.202:1509",
        "",
        "Sensors & I/O:",
        "  - IR: 2x ADC 12-bit (0-4095)",
        "  - Buttons: 3x Digital Input",
        "  - LED: WS2812 Serial Protocol"
    ]
    
    sy = specs_y + 60
    for line in specs:
        cv2.putText(img, line, (specs_x + 20, sy), FONT, FONT_TINY, COLOR_TEXT, 1)
        sy += 22
    
    # ===== FOOTER =====
    footer = f"Generated: 2025-12-06 | Single Cube ESP32 Project | Total GPIO: 22 Active Pins"
    footer_size = cv2.getTextSize(footer, FONT, FONT_SMALL, 1)[0]
    footer_x = (WIDTH - footer_size[0]) // 2
    cv2.putText(img, footer, (footer_x, HEIGHT - 40), FONT, FONT_SMALL, (120, 120, 130), 1)
    
    return img

# ============================================
# MAIN
# ============================================

def main():
    img = create_detailed_schematic()
    
    # Save
    cv2.imwrite(OUTPUT_IMAGE, img)
    print(f"\n✅ Saved: {OUTPUT_IMAGE}")
    print(f"   Size: {WIDTH}x{HEIGHT} pixels")
    
    # Display
    scale = 0.35
    display_img = cv2.resize(img, (int(WIDTH * scale), int(HEIGHT * scale)))
    cv2.imshow("ESP32 Detailed System Schematic", display_img)
    print("\n⌨️  Press any key to close...")
    cv2.waitKey(0)
    cv2.destroyAllWindows()
    
    print("\n🎉 Complete!")
    print(f"📊 Statistics:")
    print(f"   - 8 Modules rendered")
    print(f"   - 22 GPIO connections mapped")
    print(f"   - Complete GPIO table included")

if __name__ == "__main__":
    main()
