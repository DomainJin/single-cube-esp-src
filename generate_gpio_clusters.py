"""
Generate ESP32 GPIO Cluster Pinout Diagram for PCB Layout
Shows physical grouping of GPIO pins by function
"""

from PIL import Image, ImageDraw, ImageFont
import os

# Image settings
WIDTH = 2400
HEIGHT = 3000
BG_COLOR = (255, 255, 255)

# Colors for different blocks
COLORS = {
    'MOTOR': (255, 100, 100),      # Red
    'ENCODER': (100, 150, 255),    # Blue
    'XILANH': (255, 150, 100),     # Orange
    'A4L': (255, 200, 100),        # Yellow
    'IR': (150, 255, 150),         # Green
    'I2C': (200, 150, 255),        # Purple
    'LED': (255, 150, 200),        # Pink
    'MISC': (200, 200, 200)        # Gray
}

def draw_esp32_chip(img, x, y):
    """Draw ESP32 NodeMCU-32S chip representation"""
    draw = ImageDraw.Draw(img)
    
    # Chip body
    chip_width = 400
    chip_height = 1400
    chip_x = x - chip_width // 2
    chip_y = y - chip_height // 2
    
    draw.rectangle([chip_x, chip_y, chip_x + chip_width, chip_y + chip_height], 
                   fill=(50, 50, 50), outline=(0, 0, 0), width=3)
    
    # Label
    try:
        font_large = ImageFont.truetype("arial.ttf", 36)
        font_medium = ImageFont.truetype("arial.ttf", 24)
    except:
        font_large = ImageFont.load_default()
        font_medium = ImageFont.load_default()
    
    draw.text((x, chip_y + 50), "ESP32", fill=(255, 255, 255), 
              font=font_large, anchor="mm")
    draw.text((x, chip_y + 100), "NodeMCU-32S", fill=(200, 200, 200), 
              font=font_medium, anchor="mm")
    
    return chip_x, chip_y, chip_width, chip_height

def draw_pin_block(img, x, y, title, pins, color, side='left'):
    """Draw a functional pin block"""
    draw = ImageDraw.Draw(img)
    
    try:
        font_title = ImageFont.truetype("arial.ttf", 28)
        font_pin = ImageFont.truetype("arial.ttf", 20)
    except:
        font_title = ImageFont.load_default()
        font_pin = ImageFont.load_default()
    
    # Block dimensions
    block_width = 350
    pin_height = 50
    block_height = len(pins) * pin_height + 80
    
    # Draw block background
    draw.rectangle([x, y, x + block_width, y + block_height], 
                   fill=color, outline=(0, 0, 0), width=3)
    
    # Title
    draw.text((x + block_width // 2, y + 30), title, 
              fill=(0, 0, 0), font=font_title, anchor="mm")
    
    # Draw pins
    pin_y = y + 70
    for gpio, label in pins:
        # Pin circle
        pin_x = x + 20 if side == 'left' else x + block_width - 20
        draw.ellipse([pin_x - 15, pin_y - 15, pin_x + 15, pin_y + 15], 
                     fill=(255, 255, 0), outline=(0, 0, 0), width=2)
        
        # GPIO number
        draw.text((pin_x, pin_y), str(gpio), fill=(0, 0, 0), 
                  font=font_pin, anchor="mm")
        
        # Label
        label_x = x + 50 if side == 'left' else x + block_width - 50
        anchor_val = "lm" if side == 'left' else "rm"
        draw.text((label_x, pin_y), label, fill=(0, 0, 0), 
                  font=font_pin, anchor=anchor_val)
        
        pin_y += pin_height
    
    return block_height

def draw_connection_line(img, x1, y1, x2, y2, color=(100, 100, 100)):
    """Draw connection line between blocks and chip"""
    draw = ImageDraw.Draw(img)
    draw.line([x1, y1, x2, y2], fill=color, width=3)

def main():
    # Create image
    img = Image.new('RGB', (WIDTH, HEIGHT), BG_COLOR)
    draw = ImageDraw.Draw(img)
    
    # Title
    try:
        font_title = ImageFont.truetype("arial.ttf", 48)
        font_subtitle = ImageFont.truetype("arial.ttf", 32)
    except:
        font_title = ImageFont.load_default()
        font_subtitle = ImageFont.load_default()
    
    draw.text((WIDTH // 2, 80), "ESP32 GPIO PINOUT - CỤM VẬT LÝ", 
              fill=(0, 0, 0), font=font_title, anchor="mm")
    draw.text((WIDTH // 2, 140), "Physical Clusters for PCB Layout", 
              fill=(100, 100, 100), font=font_subtitle, anchor="mm")
    
    # Draw ESP32 chip in center
    chip_center_x = WIDTH // 2
    chip_center_y = HEIGHT // 2
    chip_x, chip_y, chip_w, chip_h = draw_esp32_chip(img, chip_center_x, chip_center_y)
    
    # LEFT SIDE BLOCKS
    left_x = 150
    left_y = 400
    
    # BLOCK 1: Encoders (LEFT)
    encoder_pins = [
        (12, "M1 ENC_A"),
        (13, "M1 ENC_B"),
        (14, "M2 ENC_A"),
        (25, "M2 ENC_B"),
        (26, "M3 ENC_A"),
        (27, "M3 ENC_B")
    ]
    h1 = draw_pin_block(img, left_x, left_y, "🔵 ENCODERS", encoder_pins, COLORS['ENCODER'], 'left')
    
    # BLOCK 2: A4L Relays (LEFT)
    left_y += h1 + 50
    a4l_pins = [
        (32, "Next Button"),
        (33, "Sync Mode"),
    ]
    h2 = draw_pin_block(img, left_x, left_y, "🟠 A4L RELAY", a4l_pins, COLORS['A4L'], 'left')
    
    # BLOCK 3: IR Sensors (LEFT)
    left_y += h2 + 50
    ir_pins = [
        (34, "IR Sensor 1"),
        (35, "IR Sensor 2"),
        (36, "IR Sensor 3")
    ]
    h3 = draw_pin_block(img, left_x, left_y, "🟢 IR SENSORS", ir_pins, COLORS['IR'], 'left')
    
    # RIGHT SIDE BLOCKS
    right_x = WIDTH - 150 - 350
    right_y = 400
    
    # BLOCK 4: Motors (RIGHT)
    motor_pins = [
        (2, "M1 EN (PWM)"),
        (4, "M1 DIR"),
        (5, "M2 EN (PWM)"),
        (16, "M2 DIR"),
        (17, "M3 EN (PWM)"),
        (18, "M3 DIR")
    ]
    h4 = draw_pin_block(img, right_x, right_y, "🔴 MOTORS", motor_pins, COLORS['MOTOR'], 'right')
    
    # BLOCK 5: Xilanh (RIGHT)
    right_y += h4 + 50
    xilanh_pins = [
        (19, "Xilanh EN (PWM)"),
        (21, "Xilanh DIR")
    ]
    h5 = draw_pin_block(img, right_x, right_y, "🟣 XILANH", xilanh_pins, COLORS['XILANH'], 'right')
    
    # BLOCK 6: I2C (RIGHT)
    right_y += h5 + 50
    i2c_pins = [
        (15, "SDA"),
        (22, "SCL")
    ]
    h6 = draw_pin_block(img, right_x, right_y, "🟡 I2C", i2c_pins, COLORS['I2C'], 'right')
    
    # BLOCK 7: LED (RIGHT)
    right_y += h6 + 50
    led_pins = [
        (0, "WS2812 Data")
    ]
    h7 = draw_pin_block(img, right_x, right_y, "💡 LED", led_pins, COLORS['LED'], 'right')
    
    # BLOCK 8: A4L HDMI (RIGHT)
    right_y += h7 + 50
    hdmi_pins = [
        (23, "HDMI Mode")
    ]
    h8 = draw_pin_block(img, right_x, right_y, "🟠 A4L", hdmi_pins, COLORS['A4L'], 'right')
    
    # Add legend
    legend_y = HEIGHT - 400
    draw.text((WIDTH // 2, legend_y - 50), "⚠️ STRAPPING PINS: GPIO 0,2,5,12,15 (Cẩn thận khi boot)", 
              fill=(200, 0, 0), font=font_subtitle, anchor="mm")
    draw.text((WIDTH // 2, legend_y - 10), "📌 INPUT-ONLY: GPIO 34,35,36 (Chỉ dùng cho ADC/IR sensors)", 
              fill=(0, 100, 200), font=font_subtitle, anchor="mm")
    
    # Component count summary
    summary_y = HEIGHT - 300
    try:
        font_small = ImageFont.truetype("arial.ttf", 24)
    except:
        font_small = ImageFont.load_default()
    
    draw.text((WIDTH // 2, summary_y), "🔧 LINH KIỆN CẦN THIẾT:", 
              fill=(0, 0, 0), font=font_title, anchor="mm")
    
    components = [
        "4× L298N motor driver (3 motor + 1 xilanh)",
        "1× 74HC04 hex inverter (DIR → IN2 inversion)",
        "6× 4.7kΩ resistor (encoder pull-up)",
        "6× 1kΩ + 6× 47nF (encoder RC filter)",
        "6× MM3Z3V3 zener diode (level shifter 5V→3.3V)",
        "2× 4.7kΩ resistor (I2C pull-up)",
        "1× 470Ω resistor + 1000µF cap (LED strip)"
    ]
    
    comp_y = summary_y + 60
    for comp in components:
        draw.text((WIDTH // 2, comp_y), comp, fill=(50, 50, 50), font=font_small, anchor="mm")
        comp_y += 40
    
    # Save image
    output_path = "ESP32_GPIO_CLUSTERS.png"
    img.save(output_path, quality=95)
    print(f"✅ Generated: {output_path}")
    print(f"   Size: {WIDTH}x{HEIGHT}px")
    print(f"   File: {os.path.getsize(output_path) / 1024:.1f} KB")

if __name__ == "__main__":
    main()
