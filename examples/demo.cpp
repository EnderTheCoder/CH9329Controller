#include <ch9329/CH9329Controller.hpp>
#include <iostream>

int main() {
    ender::CH9329Controller ctrl("/dev/ttyUSB0", 9600);
    if (const auto info = ctrl.get_info()) {
        std::cout << "Connected: " << info->usb_connected << std::endl;
    } else {
        std::cerr << "Failed to get device info." << std::endl;
    }
    return 0;
}
