/**
 * @file OMNI_USAGE_EXAMPLES.cpp
 * @brief Ví dụ sử dụng hệ thống điều khiển robot 3 bánh omni
 * @date 2025-12-06
 * 
 * Tập hợp các ví dụ điều khiển robot omni holonomic
 */

#include <Arduino.h>
#include "omni.h"

// ============================================
// VÍ DỤ 1: Setup và khởi động cơ bản
// ============================================
void example1_basic_setup() {
    Serial.println("\n=== Example 1: Basic Setup ===");
    
    // Khởi tạo hệ thống omni
    setupOmni();
    
    // Kích hoạt robot
    setOmniEnabled(true);
    
    // Reset odometry về gốc
    resetOmniOdometry();
}

// ============================================
// VÍ DỤ 2: Di chuyển thẳng
// ============================================
void example2_move_forward() {
    Serial.println("\n=== Example 2: Move Forward ===");
    
    // Di chuyển thẳng với tốc độ 50 cm/s
    omniForward(50.0);
    
    // Chờ 2 giây
    delay(2000);
    
    // Dừng
    omniStop();
    
    // Di chuyển lùi với tốc độ 30 cm/s
    omniForward(-30.0);
    delay(2000);
    omniStop();
}

// ============================================
// VÍ DỤ 3: Di chuyển ngang (Strafe)
// ============================================
void example3_strafe() {
    Serial.println("\n=== Example 3: Strafe Movement ===");
    
    // Di chuyển sang phải 40 cm/s
    omniStrafe(40.0);
    delay(2000);
    omniStop();
    
    // Di chuyển sang trái 40 cm/s
    omniStrafe(-40.0);
    delay(2000);
    omniStop();
}

// ============================================
// VÍ DỤ 4: Quay tại chỗ
// ============================================
void example4_rotate() {
    Serial.println("\n=== Example 4: Rotate in Place ===");
    
    // Quay ngược chiều kim đồng hồ (CCW) 90 deg/s
    omniRotate(90.0);
    delay(2000);
    omniStop();
    
    // Quay cùng chiều kim đồng hồ (CW) 90 deg/s
    omniRotate(-90.0);
    delay(2000);
    omniStop();
}

// ============================================
// VÍ DỤ 5: Di chuyển theo hướng
// ============================================
void example5_move_direction() {
    Serial.println("\n=== Example 5: Move in Direction ===");
    
    // Di chuyển theo góc 45° với tốc độ 50 cm/s
    omniMoveDirection(45.0, 50.0);
    delay(2000);
    
    // Di chuyển theo góc 135° với tốc độ 50 cm/s
    omniMoveDirection(135.0, 50.0);
    delay(2000);
    
    // Di chuyển theo góc 270° (trái) với tốc độ 50 cm/s
    omniMoveDirection(270.0, 50.0);
    delay(2000);
    
    omniStop();
}

// ============================================
// VÍ DỤ 6: Di chuyển tổng hợp (vx, vy, omega)
// ============================================
void example6_combined_motion() {
    Serial.println("\n=== Example 6: Combined Motion ===");
    
    // Di chuyển chéo và quay đồng thời
    // vx = 0.3 m/s (forward), vy = 0.2 m/s (right), omega = 0.5 rad/s (CCW)
    omniMove(0.3, 0.2, 0.5);
    delay(3000);
    
    omniStop();
    delay(1000);
    
    // Di chuyển chéo ngược hướng
    omniMove(-0.3, -0.2, -0.5);
    delay(3000);
    
    omniStop();
}

// ============================================
// VÍ DỤ 7: Vẽ hình vuông
// ============================================
void example7_square_pattern() {
    Serial.println("\n=== Example 7: Square Pattern ===");
    
    float side_length = 100.0; // cm
    float speed = 30.0; // cm/s
    float move_time = (side_length / speed) * 1000; // ms
    
    for (int i = 0; i < 4; i++) {
        // Di chuyển thẳng
        omniForward(speed);
        delay(move_time);
        omniStop();
        delay(500);
        
        // Quay 90 độ
        omniRotate(90.0);
        delay(1000); // ~90 độ với 90 deg/s
        omniStop();
        delay(500);
    }
}

// ============================================
// VÍ DỤ 8: Vẽ hình tròn
// ============================================
void example8_circle_pattern() {
    Serial.println("\n=== Example 8: Circle Pattern ===");
    
    // Di chuyển thành vòng tròn: vx + omega
    float forward_speed = 0.3; // m/s
    float angular_speed = 0.5; // rad/s (~28.6 deg/s)
    
    omniMove(forward_speed, 0, angular_speed);
    delay(5000); // 5 giây
    
    omniStop();
}

// ============================================
// VÍ DỤ 9: Di chuyển đến vị trí mục tiêu
// ============================================
void example9_move_to_target() {
    Serial.println("\n=== Example 9: Move to Target Position ===");
    
    float target_x = 50.0; // cm
    float target_y = 30.0; // cm
    float speed = 40.0; // cm/s
    
    // Loop cho đến khi đến đích
    while (!omniMoveTo(target_x, target_y, speed)) {
        updateOmni();
        delay(50);
    }
    
    Serial.println("Target reached!");
}

// ============================================
// VÍ DỤ 10: Quay đến góc mục tiêu
// ============================================
void example10_rotate_to_angle() {
    Serial.println("\n=== Example 10: Rotate to Target Angle ===");
    
    float target_angle = 90.0; // degrees
    float angular_speed = 60.0; // deg/s
    
    // Loop cho đến khi đến góc mục tiêu
    while (!omniRotateTo(target_angle, angular_speed)) {
        updateOmni();
        delay(50);
    }
    
    Serial.println("Target angle reached!");
}

// ============================================
// VÍ DỤ 11: Field-centric control
// ============================================
void example11_field_centric() {
    Serial.println("\n=== Example 11: Field-Centric Control ===");
    
    // Luôn di chuyển về hướng Bắc (field), bất kể robot quay hướng nào
    float vx_field = 0.5; // m/s hướng Bắc
    float vy_field = 0.0;
    float omega = 0.3; // rad/s - robot quay trong khi di chuyển
    
    omniFieldCentricMove(vx_field, vy_field, omega);
    delay(5000);
    
    omniStop();
}

// ============================================
// VÍ DỤ 12: Zigzag pattern
// ============================================
void example12_zigzag_pattern() {
    Serial.println("\n=== Example 12: Zigzag Pattern ===");
    
    for (int i = 0; i < 5; i++) {
        // Di chuyển chéo phải
        omniMove(0.3, 0.2, 0);
        delay(1000);
        
        // Di chuyển chéo trái
        omniMove(0.3, -0.2, 0);
        delay(1000);
    }
    
    omniStop();
}

// ============================================
// VÍ DỤ 13: Đọc và in trạng thái robot
// ============================================
void example13_monitor_state() {
    Serial.println("\n=== Example 13: Monitor Robot State ===");
    
    // Di chuyển với vận tốc nhất định
    omniMove(0.4, 0.3, 0.5);
    
    // In trạng thái mỗi giây
    for (int i = 0; i < 5; i++) {
        updateOmni();
        delay(1000);
        
        omniPrintDebug();
        
        OmniRobotState* state = getOmniState();
        Serial.printf("Odometry: (%.2f, %.2f) cm, heading: %.1f deg\n",
                     state->pos_x * 100, state->pos_y * 100, 
                     omniRadToDeg(state->heading));
    }
    
    omniStop();
}

// ============================================
// VÍ DỤ 14: Điều khiển với joystick (giả lập)
// ============================================
void example14_joystick_control() {
    Serial.println("\n=== Example 14: Joystick Control (Simulated) ===");
    
    // Giả lập input từ joystick
    float joy_x = 0.7;  // -1.0 to 1.0
    float joy_y = 0.5;  // -1.0 to 1.0
    float joy_rot = 0.3; // -1.0 to 1.0
    
    // Scale to actual velocities
    float max_linear = 0.5; // m/s
    float max_angular = 2.0; // rad/s
    
    float vx = joy_x * max_linear;
    float vy = joy_y * max_linear;
    float omega = joy_rot * max_angular;
    
    omniMove(vx, vy, omega);
    delay(3000);
    
    omniStop();
}

// ============================================
// VÍ DỤ 15: Emergency brake test
// ============================================
void example15_emergency_brake() {
    Serial.println("\n=== Example 15: Emergency Brake Test ===");
    
    // Tăng tốc đến tốc độ cao
    omniMove(0.8, 0, 0);
    delay(2000);
    
    Serial.println("EMERGENCY BRAKE!");
    omniBrake();
    
    delay(1000);
}

// ============================================
// VÍ DỤ 16: Autonomous navigation pattern
// ============================================
void example16_autonomous_navigation() {
    Serial.println("\n=== Example 16: Autonomous Navigation ===");
    
    // Waypoint list (cm)
    float waypoints[][2] = {
        {50, 0},
        {50, 50},
        {0, 50},
        {0, 0}
    };
    
    int num_waypoints = 4;
    float speed = 30.0; // cm/s
    
    for (int i = 0; i < num_waypoints; i++) {
        Serial.printf("Going to waypoint %d: (%.0f, %.0f)\n", 
                     i, waypoints[i][0], waypoints[i][1]);
        
        // Di chuyển đến waypoint
        while (!omniMoveTo(waypoints[i][0], waypoints[i][1], speed)) {
            updateOmni();
            delay(50);
        }
        
        Serial.println("Waypoint reached!");
        delay(1000);
    }
}

// ============================================
// MAIN DEMO FUNCTION
// ============================================
void demo_omni_robot() {
    Serial.println("\n");
    Serial.println("========================================");
    Serial.println("    OMNI ROBOT CONTROL EXAMPLES");
    Serial.println("========================================");
    
    // Chạy tất cả ví dụ (comment out những cái không cần)
    
    example1_basic_setup();
    delay(1000);
    
    // example2_move_forward();
    // example3_strafe();
    // example4_rotate();
    // example5_move_direction();
    // example6_combined_motion();
    // example7_square_pattern();
    // example8_circle_pattern();
    // example9_move_to_target();
    // example10_rotate_to_angle();
    // example11_field_centric();
    // example12_zigzag_pattern();
    // example13_monitor_state();
    // example14_joystick_control();
    // example15_emergency_brake();
    // example16_autonomous_navigation();
    
    Serial.println("\nDemo completed!");
}

// ============================================
// LOOP EXAMPLE - Tích hợp vào main loop
// ============================================
void omni_control_loop_example() {
    // Gọi trong loop() của main.cpp
    
    static unsigned long last_update = 0;
    unsigned long current_time = millis();
    
    if (current_time - last_update >= 50) { // 20 Hz update
        updateOmni();
        last_update = current_time;
    }
    
    // Có thể thêm các command control ở đây
    // Ví dụ: đọc từ serial, joystick, v.v.
}
