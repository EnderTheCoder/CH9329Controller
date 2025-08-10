# CH9329 Controller Library

A C++ library for controlling CH9329 USB-to-Serial devices, providing comprehensive APIs for keyboard, mouse, and HID operations.

## 📋 Features

- Complete keyboard control (regular and multimedia keys)
- Absolute and relative mouse operations
- Custom HID data transmission
- Device configuration management
- High-level mouse operations (click, drag, hover, etc.)
- Cross-platform support (Linux, Windows, macOS)

## 🛠️ Dependencies

- C++20 or later
- [Boost.Asio](https://www.boost.org/doc/libs/release/doc/html/boost_asio.html)
- CMake 3.14 or later

## 📦 Installation

### Install Dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install libboost-system-dev cmake build-essential
```

**macOS (using Homebrew):**
```bash
brew install boost cmake
```

**Windows:**
- Install [Boost](https://www.boost.org/users/download/)
- Install [CMake](https://cmake.org/download/)

### Build and Install Library

```bash
git clone https://github.com/EnderTheCoder/CH9329Controller
cd CH9329Lib
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
```

## 🚀 Usage

### CMake Integration

#### Method 1: Find Package (after installation)

```cmake
find_package(CH9329Controller REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE CH9329::CH9329Controller)
```

#### Method 2: Add Subdirectory

```cmake
add_subdirectory(path/to/CH9329Lib)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE CH9329Controller)
```

### Basic Example

```cpp
#include <ch9329/CH9329Controller.hpp>
#include <iostream>
#include <thread>
#include <chrono>

using namespace enderhive::device;
using namespace std::chrono_literals;

int main() {
    try {
        // Initialize controller with serial port
        CH9329Controller controller("/dev/ttyUSB0", 9600);
        
        // Get device information
        auto info = controller.get_info();
        if (info) {
            std::cout << "Device connected: " << info->usb_connected << std::endl;
            std::cout << "Version: " << static_cast<int>(info->version_major) 
                      << "." << static_cast<int>(info->version_minor) << std::endl;
        }
        
        // Send keyboard input: "Hello"
        controller.send_kb_general_data(KeyboardCtrlKey::None, {0x0B, 0x08, 0x0C, 0x0C, 0x12}); // H,e,l,l,o
        
        // Move mouse to absolute position and click
        controller.move_to_absolute(2048, 2048); // Center of screen
        std::this_thread::sleep_for(100ms);
        controller.click(MouseButton::Left);
        
        // Drag operation
        controller.drag_absolute(1000, 1000, 3000, 3000, MouseButton::Left);
        
        // Scroll wheel
        controller.scroll_wheel(-3); // Scroll down
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
```

## 📚 API Overview

### Device Management

```cpp
// Constructor
CH9329Controller controller("/dev/ttyUSB0", 9600);

// Get device information
std::optional<DeviceInfo> info = controller.get_info();

// Reset device
controller.reset();

// Restore factory settings
controller.set_default_config();
```

### Keyboard Operations

```cpp
// Regular keyboard input
controller.send_kb_general_data(KeyboardCtrlKey::LeftShift, {0x04}); // 'A'

// Multimedia keys
controller.send_kb_media_data(0x02, 0xE9); // Volume Up
```

### Mouse Operations

```cpp
// Absolute positioning
controller.move_to_absolute(4095, 4095); // Bottom-right corner
controller.click_at_absolute(2048, 2048);

// Relative movement
controller.move_mouse(100, -50);
controller.scroll_wheel(2); // Scroll up

// High-level operations
controller.click(MouseButton::Right, 100);
controller.double_click();
controller.drag_select(100, 100, 500, 500);
controller.right_click_menu();
```

### Configuration Management

```cpp
// USB string descriptors
auto manufacturer = controller.get_usb_string(UsbStringType::Manufacturer);
controller.set_usb_string(UsbStringType::Product, "My Custom Device");

// Parameter configuration
auto config = controller.get_para_config();
if (config) {
    // Modify config...
    controller.set_para_config(*config);
}
```

## 🎯 Coordinate Conversion

```cpp
// Convert screen coordinates to CH9329 absolute coordinates (0-4095)
auto [abs_x, abs_y] = CH9329Controller::convert_screen_to_absolute(
    1920, 1080,  // Screen coordinates
    3840, 2160   // Screen resolution
);
```

## 📖 Enumerations

### Keyboard Control Keys
```cpp
enum class KeyboardCtrlKey {
    LeftCtrl, LeftShift, LeftAlt, LeftWin,
    RightCtrl, RightShift, RightAlt, RightWin
};
```

### Mouse Buttons
```cpp
enum class MouseButton {
    None, Left, Right, Middle
};
```

### USB String Types
```cpp
enum class UsbStringType {
    Manufacturer, Product, SerialNumber
};
```

## 🔧 Building Examples

Enable examples during build:
```bash
cmake .. -DBUILD_EXAMPLES=ON
make
```

Run the demo:
```bash
./examples/demo
```

## 📄 License

This library is released under the MIT License. See [LICENSE](LICENSE) file for details.

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push to the branch
5. Create a Pull Request

## 📞 Support

For issues and feature requests, please [open an issue](../../issues) on GitHub.