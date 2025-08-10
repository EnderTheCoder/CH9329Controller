#include <ch9329/CH9329Controller.hpp>
#include <iostream>
#include <thread>
#include <numeric>
#include <ranges>
namespace ender {
    constexpr uint8_t FRAME_HEAD_1 = 0x57;
    constexpr uint8_t FRAME_HEAD_2 = 0xAB;
    constexpr uint8_t DEVICE_ADDR = 0x00;

    CH9329Controller::CH9329Controller(const std::string &port, unsigned int baud_rate)
        : io_(), port_(io_, port) {
        port_.set_option(asio::serial_port::baud_rate(baud_rate));
        port_.set_option(asio::serial_port::character_size(8));
        port_.set_option(asio::serial_port::stop_bits(asio::serial_port::stop_bits::one));
        port_.set_option(asio::serial_port::parity(asio::serial_port::parity::none));
        port_.set_option(asio::serial_port::flow_control(asio::serial_port::flow_control::none));
    }

    CH9329Controller::~CH9329Controller() {
        if (port_.is_open()) {
            port_.close();
        }
    }

    std::vector<uint8_t> CH9329Controller::make_frame(uint8_t addr, uint8_t cmd, const std::vector<uint8_t> &data) {
        std::vector<uint8_t> frame = {FRAME_HEAD_1, FRAME_HEAD_2, addr, cmd, static_cast<uint8_t>(data.size())};
        frame.insert(frame.end(), data.begin(), data.end());

        uint8_t sum = std::accumulate(frame.begin(), frame.end(), 0);
        frame.push_back(sum);

        return frame;
    }

    std::optional<std::vector<uint8_t> > CH9329Controller::read_response() {
        boost::system::error_code ec;
        std::vector<uint8_t> buffer(128);
        size_t len = port_.read_some(asio::buffer(buffer), ec);
        if (ec || len < 6) return std::nullopt;

        buffer.resize(len);
        return buffer;
    }

    std::optional<std::vector<uint8_t> > CH9329Controller::validate_response(
        const std::vector<uint8_t> &resp, uint8_t expected_cmd) {
        if (resp.size() < 6) return std::nullopt;
        if (resp[0] != FRAME_HEAD_1 || resp[1] != FRAME_HEAD_2) return std::nullopt;
        if (resp[2] != DEVICE_ADDR) return std::nullopt;

        uint8_t rcv_cmd = resp[3];
        if ((rcv_cmd & 0x3F) != expected_cmd) return std::nullopt;

        uint8_t len = resp[4];
        if (resp.size() != 6 + len) return std::nullopt;

        uint8_t sum = std::accumulate(resp.begin(), resp.end() - 1, 0);
        if (resp.back() != sum) return std::nullopt;

        return std::vector<uint8_t>(resp.begin() + 5, resp.end() - 1);
    }

    std::optional<std::vector<uint8_t> > CH9329Controller::send_command(uint8_t cmd, const std::vector<uint8_t> &data) {
        auto frame = make_frame(DEVICE_ADDR, cmd, data);
        boost::system::error_code ec;
        asio::write(port_, asio::buffer(frame), ec);
        if (ec) return std::nullopt;

        std::this_thread::sleep_for(10ms);

        auto resp = read_response();
        if (!resp) return std::nullopt;

        auto payload = validate_response(*resp, cmd);
        if (!payload || payload->empty()) return std::nullopt;

        return payload;
    }

    // --- 接口实现 ---

    std::optional<DeviceInfo> CH9329Controller::get_info() {
        const auto response = send_command(0x01);
        if (!response || response->size() < 8) return std::nullopt;
        DeviceInfo info{};

        const uint8_t version = (*response)[0];
        info.version_major = (version >> 4) & 0x0F - 2; // 高4位为版本号
        info.version_minor = version & 0x0F; // 低4位为子版本号

        info.usb_connected = (*response)[1] == 0x01;

        const uint8_t led_status = (*response)[2];
        info.num_lock = (led_status & 0x01) != 0; // 位0：NUM LOCK
        info.caps_lock = (led_status & 0x02) != 0; // 位1：CAPS LOCK
        info.scroll_lock = (led_status & 0x04) != 0; // 位2：SCROLL LOCK

        info.pc_sleeping = (*response)[3] == 0x03;

        return info;
    }

    bool CH9329Controller::send_ms_rel_data(MouseButton button, int8_t x_delta, int8_t y_delta, int8_t wheel) {
        std::vector<uint8_t> data = {
            0x01, // Report ID for relative mouse
            pack_mouse_button(button), // Mouse button state
            static_cast<uint8_t>(x_delta), // X movement delta
            static_cast<uint8_t>(y_delta), // Y movement delta
            static_cast<uint8_t>(wheel) // Wheel movement
        };
        const auto r = send_command(0x05, data);
        return r.has_value() &&
               !r->empty() &&
               r->at(0) == static_cast<uint8_t>(CommandStatus::Success);
    }

    bool CH9329Controller::send_kb_general_data(KeyboardCtrlKey ctrl, const std::array<uint8_t, 6> &keys) {
        std::vector<uint8_t> data(8, 0);
        data[0] = pack_keyboard_ctrl_key(ctrl);
        data[1] = 0x00;
        std::ranges::copy(keys, data.begin() + 2);
        const auto r = send_command(0x02, data);
        return r.has_value() && !r->empty() && r->at(0) == static_cast<uint8_t>(CommandStatus::Success);
    }

    bool CH9329Controller::send_ms_abs_data(MouseButton button, uint16_t x, uint16_t y, int8_t wheel) {
        std::vector<uint8_t> data(7);
        data[0] = 0x02;
        data[1] = pack_mouse_button(button);
        data[2] = static_cast<uint8_t>(x & 0xFF);
        data[3] = static_cast<uint8_t>((x >> 8) & 0xFF);
        data[4] = static_cast<uint8_t>(y & 0xFF);
        data[5] = static_cast<uint8_t>((y >> 8) & 0xFF);
        data[6] = static_cast<uint8_t>(wheel);
        const auto r = send_command(0x04, data);
        return r.has_value() && !r->empty() && r->at(0) == static_cast<uint8_t>(CommandStatus::Success);
    }

    std::optional<std::vector<uint8_t> > CH9329Controller::read_hid_data() {
        return read_response(); // 该数据为设备主动上传，无需命令
    }

    std::optional<UsbStringDescriptor> CH9329Controller::get_usb_string(UsbStringType type) {
        auto resp = send_command(0x0A, {static_cast<uint8_t>(type)});
        if (!resp) return std::nullopt;

        UsbStringDescriptor desc;
        desc.content = std::string(resp->begin(), resp->end());
        return desc;
    }

    bool CH9329Controller::set_usb_string(UsbStringType type, const std::string &str) {
        std::vector<uint8_t> data = {
            static_cast<uint8_t>(type),
            static_cast<uint8_t>(str.size())
        };
        data.insert(data.end(), str.begin(), str.end());
        const auto r = send_command(0x0B, data);
        return r.has_value() && !r->empty() && r->at(0) == static_cast<uint8_t>(CommandStatus::Success);
    }


    bool CH9329Controller::send_kb_media_data(uint8_t report_id, uint16_t keycode) {
        std::vector<uint8_t> data = {
            report_id, static_cast<uint8_t>(keycode & 0xFF), static_cast<uint8_t>((keycode >> 8) & 0xFF)
        };
        const auto r = send_command(0x03, data);
        return r.has_value() && !r->empty() && r->at(0) == 0x00;
    }

    bool CH9329Controller::send_hid_data(const std::vector<uint8_t> &data) {
        if (data.size() > 64) return false;
        const auto r = send_command(0x06, data);
        return r.has_value() && !r->empty() && r->at(0) == 0x00;
    }

    std::optional<ParaConfig> CH9329Controller::get_para_config() {
        auto resp = send_command(0x08);
        if (!resp || resp->size() != 50) return std::nullopt;

        ParaConfig config{};
        std::ranges::copy(*resp, config.raw_bytes.begin());
        return config;
    }

    bool CH9329Controller::set_para_config(const ParaConfig &config) {
        const auto r = send_command(0x09, std::vector<uint8_t>(config.raw_bytes.begin(), config.raw_bytes.end()));
        return r.has_value() && !r->empty() && r->at(0) == 0x00;
    }

    bool CH9329Controller::set_default_config() {
        const auto r = send_command(0x0C);
        return r.has_value() && !r->empty() && r->at(0) == 0x00;
    }

    bool CH9329Controller::reset() {
        const auto r = send_command(0x0F);
        return r.has_value() && !r->empty() && r->at(0) == 0x00;
    }

    bool CH9329Controller::click(MouseButton button, uint16_t hold_time_ms) {
        if (!mouse_down(button)) return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(hold_time_ms));
        return mouse_up(button);
    }

    bool CH9329Controller::double_click(MouseButton button, uint16_t click_interval_ms, uint16_t hold_time_ms) {
        if (!click(button, hold_time_ms)) return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(click_interval_ms));
        return click(button, hold_time_ms);
    }

    bool CH9329Controller::mouse_down(MouseButton button) {
        std::vector<uint8_t> data = {0x01, pack_mouse_button(button), 0x00, 0x00, 0x00};
        auto r = send_command(0x05, data);
        return r.has_value() && !r->empty() && r->at(0) == static_cast<uint8_t>(CommandStatus::Success);
    }

    bool CH9329Controller::mouse_up(MouseButton button) {
        std::vector<uint8_t> data = {0x01, 0x00, 0x00, 0x00, 0x00}; // Release all buttons
        auto r = send_command(0x05, data);
        return r.has_value() && !r->empty() && r->at(0) == static_cast<uint8_t>(CommandStatus::Success);
    }

    bool CH9329Controller::drag(MouseButton button, int8_t x_delta, int8_t y_delta, uint16_t hold_time_ms) {
        if (!mouse_down(button)) return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(hold_time_ms));
        if (!move_mouse(x_delta, y_delta)) return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(hold_time_ms));
        return mouse_up(button);
    }

    bool CH9329Controller::move_mouse(int8_t x_delta, int8_t y_delta) {
        std::vector<uint8_t> data = {
            0x01, 0x00,
            static_cast<uint8_t>(x_delta),
            static_cast<uint8_t>(y_delta),
            0x00
        };
        auto r = send_command(0x05, data);
        return r.has_value() && !r->empty() && r->at(0) == static_cast<uint8_t>(CommandStatus::Success);
    }

    bool CH9329Controller::scroll_wheel(int8_t wheel_delta) {
        std::vector<uint8_t> data = {0x01, 0x00, 0x00, 0x00, static_cast<uint8_t>(wheel_delta)};
        auto r = send_command(0x05, data);
        return r.has_value() && !r->empty() && r->at(0) == static_cast<uint8_t>(CommandStatus::Success);
    }

    bool CH9329Controller::move_to_absolute(uint16_t x, uint16_t y) {
        // 确保坐标在有效范围内
        x = std::min(x, static_cast<uint16_t>(4095));
        y = std::min(y, static_cast<uint16_t>(4095));

        std::vector<uint8_t> data(7);
        data[0] = 0x02; // Absolute mouse report ID
        data[1] = 0x00; // No buttons pressed
        data[2] = static_cast<uint8_t>(x & 0xFF);
        data[3] = static_cast<uint8_t>((x >> 8) & 0xFF);
        data[4] = static_cast<uint8_t>(y & 0xFF);
        data[5] = static_cast<uint8_t>((y >> 8) & 0xFF);
        data[6] = 0x00; // No wheel movement

        auto r = send_command(0x04, data);
        return r.has_value() && !r->empty() && r->at(0) == static_cast<uint8_t>(CommandStatus::Success);
    }

    bool CH9329Controller::click_at_absolute(uint16_t x, uint16_t y, MouseButton button, uint16_t hold_time_ms) {
        if (!move_to_absolute(x, y)) return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 短暂延迟确保移动完成
        return click(button, hold_time_ms);
    }

    bool CH9329Controller::drag_absolute(uint16_t start_x, uint16_t start_y,
                                         uint16_t end_x, uint16_t end_y,
                                         MouseButton button) {
        if (!move_to_absolute(start_x, start_y)) return false;
        if (!mouse_down(button)) return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        if (!move_to_absolute(end_x, end_y)) return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        return mouse_up(button);
    }

    bool CH9329Controller::hover(uint16_t duration_ms) {
        // 发送空移动命令保持鼠标位置
        std::vector<uint8_t> data = {0x01, 0x00, 0x00, 0x00, 0x00};
        auto r = send_command(0x05, data);
        if (!(r.has_value() && !r->empty() && r->at(0) == static_cast<uint8_t>(CommandStatus::Success))) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));
        return true;
    }

    bool CH9329Controller::right_click_menu(uint16_t wait_time_ms) {
        if (!click(MouseButton::Right)) return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(wait_time_ms));
        return true;
    }

    bool CH9329Controller::drag_select(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
        return drag_absolute(x1, y1, x2, y2, MouseButton::Left);
    }

    std::pair<uint16_t, uint16_t> CH9329Controller::convert_screen_to_absolute(uint16_t screen_x, uint16_t screen_y,
                                                                               uint16_t screen_width,
                                                                               uint16_t screen_height) {
        uint16_t abs_x = static_cast<uint16_t>((static_cast<uint32_t>(screen_x) * 4095) / screen_width);
        uint16_t abs_y = static_cast<uint16_t>((static_cast<uint32_t>(screen_y) * 4095) / screen_height);

        // 确保不超过最大值
        abs_x = std::min(abs_x, static_cast<uint16_t>(4095));
        abs_y = std::min(abs_y, static_cast<uint16_t>(4095));

        return std::make_pair(abs_x, abs_y);
    }
} // namespace enderhive::device
