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

    // ========= 枚举定义 ==========

    /// @brief 键盘控制键位枚举（位组合）
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

    /// @brief 鼠标按键状态
    enum class MouseButton : uint8_t {
        None = 0x00,
        Left = 0x01,
        Right = 0x02,
        Middle = 0x04,
    };

    /// @brief USB 字符串类型
    enum class UsbStringType : uint8_t {
        Manufacturer = 0x00,
        Product = 0x01,
        SerialNumber = 0x02,
    };

    /// @brief 命令执行状态码
    enum class CommandStatus : uint8_t {
        Success = 0x00,
        Timeout = 0xE1,
        HeadError = 0xE2,
        CmdError = 0xE3,
        ChecksumError = 0xE4,
        ParameterError = 0xE5,
        OperationFailed = 0xE6,
    };

    // ========= 结构体定义 ==========

    /// @brief 设备基本信息
    struct DeviceInfo {
        uint8_t version_major = 0;
        uint8_t version_minor = 0;
        bool usb_connected = false;
        bool num_lock = false;
        bool caps_lock = false;
        bool scroll_lock = false;
        bool pc_sleeping = false;
    };

    /// @brief USB 字符串描述符配置内容
    struct UsbStringDescriptor {
        std::string content;
    };

    /// @brief 设备参数配置数据（长度50字节）
    struct ParaConfig {
        std::array<uint8_t, 50> raw_bytes;
    };

    // ========= 主要控制器类 ==========

    class CH9329Controller {
    public:
        /// @brief 构造函数，打开指定的串口并设置波特率
        explicit CH9329Controller(const std::string &port, unsigned int baud_rate = 9600);

        /// @brief 析构，自动关闭端口
        ~CH9329Controller();

        // 禁止拷贝
        CH9329Controller(const CH9329Controller &) = delete;

        CH9329Controller &operator=(const CH9329Controller &) = delete;

        // === 功能接口 ===

        /// @brief 获取设备信息
        std::optional<DeviceInfo> get_info();

        /// @brief 发送普通键盘数据
        bool send_kb_general_data(KeyboardCtrlKey ctrl, const std::array<uint8_t, 6> &keys = {});

        /// @brief 发送多媒体键盘数据
        bool send_kb_media_data(uint8_t report_id, uint16_t keycode);

        /// @brief 模拟绝对鼠标数据
        bool send_ms_abs_data(MouseButton button, uint16_t x, uint16_t y, int8_t wheel = 0);

        /// @brief 模拟相对鼠标数据
        bool send_ms_rel_data(MouseButton button, int8_t x_delta, int8_t y_delta, int8_t wheel = 0);

        /// @brief 发送自定义 HID 数据（最大64字节）
        bool send_hid_data(const std::vector<uint8_t> &data);

        /// @brief 读取下传的 HID 数据（来自 PC 的输入）
        std::optional<std::vector<uint8_t> > read_hid_data();

        /// @brief 获取当前参数配置
        std::optional<ParaConfig> get_para_config();

        /// @brief 设置参数配置
        bool set_para_config(const ParaConfig &config);

        /// @brief 获取指定类型的 USB 字符串
        std::optional<UsbStringDescriptor> get_usb_string(UsbStringType type);

        /// @brief 设置指定类型的 USB 字符串
        bool set_usb_string(UsbStringType type, const std::string &content);

        /// @brief 恢复出厂配置
        bool set_default_config();

        /// @brief 软件复位设备
        bool reset();

        // ========= 高级鼠标操作方法 ==========

        /// @brief 鼠标点击（按下并释放）
        /// @param button 鼠标按键
        /// @param hold_time_ms 按下保持时间（毫秒）
        bool click(MouseButton button = MouseButton::Left, uint16_t hold_time_ms = 50);

        /// @brief 鼠标双击
        /// @param button 鼠标按键
        /// @param click_interval_ms 两次点击间隔时间（毫秒）
        /// @param hold_time_ms 每次按下保持时间（毫秒）
        bool double_click(MouseButton button = MouseButton::Left,
                          uint16_t click_interval_ms = 150,
                          uint16_t hold_time_ms = 50);

        /// @brief 鼠标按下
        /// @param button 鼠标按键
        bool mouse_down(MouseButton button = MouseButton::Left);

        /// @brief 鼠标释放
        /// @param button 鼠标按键
        bool mouse_up(MouseButton button = MouseButton::Left);

        /// @brief 拖拽操作（从当前位置拖拽）
        /// @param button 拖拽按键
        /// @param x_delta X轴相对移动距离
        /// @param y_delta Y轴相对移动距离
        /// @param hold_time_ms 按下保持时间（毫秒）
        bool drag(MouseButton button, int8_t x_delta, int8_t y_delta, uint16_t hold_time_ms = 100);

        /// @brief 移动鼠标（相对坐标）
        /// @param x_delta X轴相对移动距离
        /// @param y_delta Y轴相对移动距离
        bool move_mouse(int8_t x_delta, int8_t y_delta);

        /// @brief 滚动鼠标滚轮
        /// @param wheel_delta 滚轮滚动量（正数向上，负数向下）
        bool scroll_wheel(int8_t wheel_delta);

        /// @brief 移动到绝对坐标（需要先进行坐标转换）
        /// @param x X轴绝对坐标（0-4095）
        /// @param y Y轴绝对坐标（0-4095）
        bool move_to_absolute(uint16_t x, uint16_t y);

        /// @brief 在绝对坐标点击
        /// @param x X轴绝对坐标（0-4095）
        /// @param y Y轴绝对坐标（0-4095）
        /// @param button 鼠标按键
        /// @param hold_time_ms 按下保持时间（毫秒）
        bool click_at_absolute(uint16_t x, uint16_t y,
                               MouseButton button = MouseButton::Left,
                               uint16_t hold_time_ms = 50);

        /// @brief 在绝对坐标拖拽
        /// @param start_x 起始点X轴坐标（0-4095）
        /// @param start_y 起始点Y轴坐标（0-4095）
        /// @param end_x 结束点X轴坐标（0-4095）
        /// @param end_y 结束点Y轴坐标（0-4095）
        /// @param button 拖拽按键
        bool drag_absolute(uint16_t start_x, uint16_t start_y,
                           uint16_t end_x, uint16_t end_y,
                           MouseButton button = MouseButton::Left);

        /// @brief 鼠标悬停在当前位置
        /// @param duration_ms 悬停时间（毫秒）
        bool hover(uint16_t duration_ms = 1000);

        /// @brief 鼠标右键菜单（右键点击后等待）
        /// @param wait_time_ms 等待时间（毫秒）
        bool right_click_menu(uint16_t wait_time_ms = 500);

        /// @brief 鼠标拖拽选择区域
        /// @param x1 起始点X坐标
        /// @param y1 起始点Y坐标
        /// @param x2 结束点X坐标
        /// @param y2 结束点Y坐标
        bool drag_select(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

        /// @brief 坐标转换：将屏幕坐标转换为CH9329绝对坐标
        /// @param screen_x 屏幕X坐标
        /// @param screen_y 屏幕Y坐标
        /// @param screen_width 屏幕宽度
        /// @param screen_height 屏幕高度
        /// @return 转换后的绝对坐标对
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

        // 辅助函数：组合鼠标按键
        static uint8_t pack_mouse_button(MouseButton b) { return static_cast<uint8_t>(b); }

        // 辅助函数：组合键盘控制键
        static uint8_t pack_keyboard_ctrl_key(KeyboardCtrlKey k) { return static_cast<uint8_t>(k); }
    };
}
