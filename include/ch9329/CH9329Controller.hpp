#pragma once

#include <boost/asio.hpp>
#include <string>
#include <vector>
#include <array>
#include <chrono>
#include <optional>

namespace ender {
    namespace asio = boost::asio;
    using namespace std::chrono_literals;

    /*
     * ========= Enum Definitions ==========
     */

    /*
     * @brief Keyboard control key enumeration (bitwise combination)
     */
    enum class KeyboardCtrlKey : uint8_t {
        LeftCtrl = 0x01,
        LeftShift = 0x02,
        LeftAlt = 0x04,
        LeftWin = 0x08,
        RightCtrl = 0x10,
        RightShift = 0x20,
        RightAlt = 0x40,
        RightWin = 0x80,
    };

    /*
     * @brief Mouse button status
     */
    enum class MouseButton : uint8_t {
        None = 0x00,
        Left = 0x01,
        Right = 0x02,
        Middle = 0x04,
    };

    /*
     * @brief USB string descriptor type
     */
    enum class UsbStringType : uint8_t {
        Manufacturer = 0x00,
        Product = 0x01,
        SerialNumber = 0x02,
    };

    /*
     * @brief Command execution status codes
     */
    enum class CommandStatus : uint8_t {
        Success = 0x00,
        Timeout = 0xE1,
        HeadError = 0xE2,
        CmdError = 0xE3,
        ChecksumError = 0xE4,
        ParameterError = 0xE5,
        OperationFailed = 0xE6,
    };

    /*
     * ========= Structure Definitions ==========
     */

    /*
     * @brief Device basic information
     */
    struct DeviceInfo {
        uint8_t version_major = 0;
        uint8_t version_minor = 0;
        bool usb_connected = false;
        bool num_lock = false;
        bool caps_lock = false;
        bool scroll_lock = false;
        bool pc_sleeping = false;
    };

    /*
     * @brief USB string descriptor configuration content
     */
    struct UsbStringDescriptor {
        std::string content;
    };

    /*
     * @brief Device parameter configuration data (50 bytes)
     */
    struct ParaConfig {
        std::array<uint8_t, 50> raw_bytes;
    };

    /*
     * ========= Main Controller Class ==========
     */

    class CH9329Controller {
    public:
        /*
         * @brief Constructor, opens the specified serial port with given baud rate
         */
        explicit CH9329Controller(const std::string &port, unsigned int baud_rate = 9600);

        /*
         * @brief Destructor, automatically closes the port
         */
        ~CH9329Controller();

        // Disable copy
        CH9329Controller(const CH9329Controller &) = delete;
        CH9329Controller &operator=(const CH9329Controller &) = delete;

        // === Functional Interface ===

        /*
         * @brief Get device information
         */
        std::optional<DeviceInfo> get_info();

        /*
         * @brief Send general keyboard data
         */
        bool send_kb_general_data(KeyboardCtrlKey ctrl, const std::array<uint8_t, 6> &keys = {});

        /*
         * @brief Send multimedia keyboard data
         */
        bool send_kb_media_data(uint8_t report_id, uint16_t keycode);

        /*
         * @brief Send absolute mouse data
         */
        bool send_ms_abs_data(MouseButton button, uint16_t x, uint16_t y, int8_t wheel = 0);

        /*
         * @brief Send relative mouse data
         */
        bool send_ms_rel_data(MouseButton button, int8_t x_delta, int8_t y_delta, int8_t wheel = 0);

        /*
         * @brief Send custom HID data (up to 64 bytes)
         */
        bool send_hid_data(const std::vector<uint8_t> &data);

        /*
         * @brief Read HID data from PC (upstream input)
         */
        std::optional<std::vector<uint8_t> > read_hid_data();

        /*
         * @brief Get current parameter configuration
         */
        std::optional<ParaConfig> get_para_config();

        /*
         * @brief Set parameter configuration
         */
        bool set_para_config(const ParaConfig &config);

        /*
         * @brief Get USB string descriptor of the specified type
         */
        std::optional<UsbStringDescriptor> get_usb_string(UsbStringType type);

        /*
         * @brief Set USB string descriptor of the specified type
         */
        bool set_usb_string(UsbStringType type, const std::string &content);

        /*
         * @brief Reset configuration to factory default
         */
        bool set_default_config();

        /*
         * @brief Software reset the device
         */
        bool reset();

        /*
         * ========= Advanced Mouse Operation Methods ==========
         */

        /*
         * @brief Mouse click (press and release)
         * @param button Mouse button to click
         * @param hold_time_ms Time to hold the button (in milliseconds)
         */
        bool click(MouseButton button = MouseButton::Left, uint16_t hold_time_ms = 50);

        /*
         * @brief Mouse double click
         * @param button Mouse button to click
         * @param click_interval_ms Interval between clicks (in milliseconds)
         * @param hold_time_ms Time to hold the button (in milliseconds)
         */
        bool double_click(MouseButton button = MouseButton::Left,
                          uint16_t click_interval_ms = 150,
                          uint16_t hold_time_ms = 50);

        /*
         * @brief Mouse down
         * @param button Mouse button to press
         */
        bool mouse_down(MouseButton button = MouseButton::Left);

        /*
         * @brief Mouse up
         * @param button Mouse button to release
         */
        bool mouse_up(MouseButton button = MouseButton::Left);

        /*
         * @brief Drag (drag from current position)
         * @param button Mouse button to drag
         * @param x_delta X-axis relative movement
         * @param y_delta Y-axis relative movement
         * @param hold_time_ms Hold time for drag (in milliseconds)
         */
        bool drag(MouseButton button, int8_t x_delta, int8_t y_delta, uint16_t hold_time_ms = 100);

        /*
         * @brief Move mouse relatively
         * @param x_delta X-axis relative movement
         * @param y_delta Y-axis relative movement
         */
        bool move_mouse(int8_t x_delta, int8_t y_delta);

        /*
         * @brief Scroll mouse wheel
         * @param wheel_delta Scroll amount (positive = up, negative = down)
         */
        bool scroll_wheel(int8_t wheel_delta);

        /*
         * @brief Move mouse to absolute coordinates (requires prior coordinate mapping)
         * @param x Absolute X-coordinate (0-4095)
         * @param y Absolute Y-coordinate (0-4095)
         */
        bool move_to_absolute(uint16_t x, uint16_t y);

        /*
         * @brief Click at absolute coordinates
         * @param x Absolute X-coordinate (0-4095)
         * @param y Absolute Y-coordinate (0-4095)
         * @param button Mouse button to use
         * @param hold_time_ms Hold duration (in milliseconds)
         */
        bool click_at_absolute(uint16_t x, uint16_t y,
                               MouseButton button = MouseButton::Left,
                               uint16_t hold_time_ms = 50);

        /*
         * @brief Drag from one absolute point to another
         * @param start_x Starting X-coordinate (0-4095)
         * @param start_y Starting Y-coordinate (0-4095)
         * @param end_x Ending X-coordinate (0-4095)
         * @param end_y Ending Y-coordinate (0-4095)
         * @param button Mouse button to use
         */
        bool drag_absolute(uint16_t start_x, uint16_t start_y,
                           uint16_t end_x, uint16_t end_y,
                           MouseButton button = MouseButton::Left);

        /*
         * @brief Hover mouse at current position
         * @param duration_ms Hover time (in milliseconds)
         */
        bool hover(uint16_t duration_ms = 1000);

        /*
         * @brief Trigger right-click context menu (right-click + wait)
         * @param wait_time_ms Wait time after click (milliseconds)
         */
        bool right_click_menu(uint16_t wait_time_ms = 500);

        /*
         * @brief Drag select an area using absolute coordinates
         * @param x1 Start point X coordinate
         * @param y1 Start point Y coordinate
         * @param x2 End point X coordinate
         * @param y2 End point Y coordinate
         */
        bool drag_select(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

        /*
         * @brief Coordinate conversion from screen space to CH9329 absolute space
         * @param screen_x Screen X coordinate
         * @param screen_y Screen Y coordinate
         * @param screen_width Screen width
         * @param screen_height Screen height
         * @return Converted (X, Y) coordinate pair
         */
        static std::pair<uint16_t, uint16_t> convert_screen_to_absolute(uint16_t screen_x, uint16_t screen_y,
                                                                        uint16_t screen_width, uint16_t screen_height);

    private:
        asio::io_context io_;
        asio::serial_port port_;
        const std::chrono::milliseconds timeout_ = 500ms;

        std::optional<std::vector<uint8_t> > send_command(uint8_t cmd, const std::vector<uint8_t> &data = {});

        static std::vector<uint8_t> make_frame(uint8_t addr, uint8_t cmd, const std::vector<uint8_t> &data);

        static std::optional<std::vector<uint8_t> > validate_response(const std::vector<uint8_t> &resp,
                                                                      uint8_t expected_cmd);

        std::optional<std::vector<uint8_t> > read_response();

        // Helper function: pack mouse button value
        static uint8_t pack_mouse_button(MouseButton b) { return static_cast<uint8_t>(b); }

        // Helper function: pack keyboard control key value
        static uint8_t pack_keyboard_ctrl_key(KeyboardCtrlKey k) { return static_cast<uint8_t>(k); }
    };
}
