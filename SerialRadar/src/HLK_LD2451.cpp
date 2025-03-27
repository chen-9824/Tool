#include "HLK_LD2451.h"
#include "../pch.h"

#define MAX_QUEUE_SIZE 1000 // 限制缓冲队列最大大小

#define READ_LOG_FIEL 0 // 是否将读数据线程打印写入到日志文件，1写入，0打印到控制台

using namespace std;

static std::string get_date();

const int frame_min_size = 17; // 至少包含一个目标数据
const int frame_target_size = 7;
const std::vector<uint8_t> frame_header{0xf4, 0xf3, 0xf2, 0xf1};
const std::vector<uint8_t> frame_end{0xf8, 0xf7, 0xf6, 0xf5};

int frame_ack_min_size = 14;
const std::vector<uint8_t> cmd_header{0xfd, 0xfc, 0xfb, 0xfa};
const std::vector<uint8_t> cmd_end{0x04, 0x03, 0x02, 0x01};

const std::vector<uint8_t> cmd_enable_cfg_mode{0xff, 0x00};
const std::vector<uint8_t> cmd_disable_cfg_mode{0xfe, 0x00};

HLK_LD2451::HLK_LD2451(const std::string &port, int baudrate) : RadarSerial(port, baudrate)
{
    _reading.store(false);
    _pausing.store(false);
}

HLK_LD2451::~HLK_LD2451()
{
    deinit();
}

int HLK_LD2451::init()
{
    setReconnect(1000, true);
    set_frame_flag(frame_min_size, frame_header, frame_end);
    return 0;
}

void HLK_LD2451::deinit()
{

    _reading.store(false);
    if (_read_t.joinable())
    {
        _read_t.join();
    }

    stopReading();
    closeSerial();
}

void HLK_LD2451::print_target_info(const target_info &data)
{
    spdlog::debug("角度: {}", data.angle);
    spdlog::debug("距离: {} 米", data.distance);
    spdlog::debug("速度方向: {} (1:靠近 0:远离)", data.direction);
    spdlog::debug("速度值: {} km/h", data.speed);
    spdlog::debug("信噪比: {}", data.SNR);
}

void HLK_LD2451::start_read_thread()
{
    _reading.store(true);
    _read_t = thread(read_thread, 1, this);
}

void HLK_LD2451::stop_read_thread()
{
    _reading.store(false);
    if (_read_t.joinable())
    {
        _read_t.join();
    }
    spdlog::info("读线程已停止");
    _read_t = std::thread();
}

void HLK_LD2451::pause_read_thread()
{
    spdlog::info("暂停读线程");
    _pausing.store(true);
}

void HLK_LD2451::resume_read_thread()
{
    spdlog::info("恢复读线程");
    _pausing.store(false);
}

std::vector<HLK_LD2451::target_info> HLK_LD2451::get_target_data()
{
    std::lock_guard<std::mutex> dataLock(dataMutex);
    std::vector<target_info> allData;
    while (!dataQueue.empty())
    {
        allData.push_back(dataQueue.front());
        dataQueue.pop_front();
    }
    return allData;
}

std::vector<uint8_t> HLK_LD2451::send_cmd(const std::vector<uint8_t> &cmd_key, const std::vector<uint8_t> &cmd_val)
{
    std::vector<uint8_t> res;
    if (!is_available())
    {
        return res;
    }

    uint16_t send_key = cmd_key[0] | (cmd_key[1] << 8);

    if (cmd_key[0] == cmd_enable_cfg_mode[0] && cmd_key[1] == cmd_enable_cfg_mode[1])
    {
        // 使能配置模式前要停止读线程,, 保证下发命令能获取到回复
        pause_read_thread();
    }

    std::vector<uint8_t> cmd;
    cmd.reserve(cmd_header.size() + 2 + cmd_key.size() + cmd_val.size() + cmd_end.size());
    cmd.insert(cmd.end(), cmd_header.begin(), cmd_header.end());

    int size = cmd_key.size() + cmd_val.size();
    uint8_t high, low;
    high = 0x00;
    if (size > 255)
    {
        high = (size >> 8) & 0xFF;
    }
    low = size & 0xFF;

    cmd.push_back(low);
    cmd.push_back(high);

    cmd.insert(cmd.end(), cmd_key.begin(), cmd_key.end());
    cmd.insert(cmd.end(), cmd_val.begin(), cmd_val.end());

    cmd.insert(cmd.end(), cmd_end.begin(), cmd_end.end());

    spdlog::debug("下发命令:");
    printf_uint8(cmd);

    if (sendCommand(cmd))
    {
        if (cmd_key[0] == cmd_disable_cfg_mode[0] && cmd_key[1] == cmd_disable_cfg_mode[1])
        {
            // 退出配置模式需要重启读线程,不需要知道返回结果
            resume_read_thread();
            return res;
        }

        while (true)
        {
            spdlog::debug("获取下发命令回复...");
            std::vector<uint8_t> allData = getOlestData();
            if (!allData.empty())
            {

                spdlog::debug("allData:");
                printf_uint8(allData);
                std::vector<std::vector<uint8_t>> allResponse = cutFrame(allData, cmd_header, cmd_end, frame_ack_min_size);

                for (std::vector<uint8_t> response : allResponse)
                {
                    if (!response.empty())
                    {
                        spdlog::debug("下发命令回复:");
                        printf_uint8(response);
                    }

                    int key_pos = 0;
                    uint16_t response_key = response[key_pos] | (response[key_pos + 1] << 8);

                    // spdlog::debug("send_key: 0x{:04x}, response_key: 0x{:04x}", send_key, response_key);

                    if (response_key == (send_key | 0x0100))
                    {
                        if (response[key_pos + 2] == 0x00)
                        {
                            res = std::vector<uint8_t>(response.begin() + (key_pos + 2), (response.end()));
                            spdlog::debug("0x{:04x} 命令执行成功, 返回值如下:", send_key);
                            printf_uint8(res);
                            return res;
                        }
                        else if (response[key_pos + 2] == 0x01)
                        {
                            spdlog::error("{} 命令执行失败");
                            return res;
                        }
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }

    return res;
}

void HLK_LD2451::enable_cfg_mode()
{

    std::vector<uint8_t> val{0x01, 0x00};
    std::vector<uint8_t> res = send_cmd(cmd_enable_cfg_mode, val);
}

void HLK_LD2451::disable_cfg_mode()
{
    std::vector<uint8_t> val;
    std::vector<uint8_t> res = send_cmd(cmd_disable_cfg_mode, val);
}

void HLK_LD2451::read_target_cfg()
{
    std::vector<uint8_t> key{0x12, 0x00};
    std::vector<uint8_t> val;
    std::vector<uint8_t> res = send_cmd(key, val);
    if (!res.empty())
    {
        if (res[0] == 0x00)
        {
            spdlog::debug("最远探测距离: {} m", static_cast<int>(res[2]));
            spdlog::debug("运动方向设置: {} ", static_cast<int>(res[3]));
            spdlog::debug("最小运动速度设置: {} km/h", static_cast<int>(res[4]));
            spdlog::debug("无目标延迟时间设置: {} s", static_cast<int>(res[5]));
        }
        else if (res[0] == 0x01)
        {
            spdlog::error("read_target_cfg failed");
        }
    }
}

void HLK_LD2451::read_thread(int id, HLK_LD2451 *radar)
{

    if (!radar)
        return;
#if READ_LOG_FIEL
    auto logger = spdlog::basic_logger_mt("thread_" + std::to_string(id), "/home/firefly/chenFan/Tool/SerialRadar/log/thread_" + std::to_string(id) + "_" + get_date() + ".log");
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [thread %t] [%l] %v");
#endif
    try
    {
#if READ_LOG_FIEL
        logger->info("线程 {}: 运行中...读取串口数据并打印: ", id);
#else
        spdlog::info("线程 {}: 运行中...读取串口数据并打印: ", id);
#endif

        while (radar->_reading.load())
        {

            if (!radar->is_available())
            {
                spdlog::error("radar is not available, wait for reconncet");
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                continue;
            }
            else
            {
                /*if (!radar->is_reading())
                {
                    radar->startReading();
                }*/
            }

            while (radar->_pausing)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                if (!radar->_reading.load())
                    break;
            }

            std::vector<std::vector<uint8_t>> allData = radar->getAllData();

            if (!allData.empty())
            {
#if READ_LOG_FIEL
                logger->debug("读线程收到数据:");
#else
                spdlog::debug("读线程收到数据:");
#endif
                for (std::vector<uint8_t> &data : allData)
                {
                    if (!data.empty())
                    {
                        radar->printf_uint8(data);
                        radar->parse_hlk_radar_data(radar->parseFrame(data));
                    }
                }

#if READ_LOG_FIEL
                logger->debug("====================================");

#else
                spdlog::debug("====================================");
#endif
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    catch (const std::exception &e)
    {
        spdlog::error("获取数据时发生异常: {}", e.what());
    }
}

void HLK_LD2451::parse_hlk_radar_data(const std::vector<std::vector<uint8_t>> &data)
{
    if (data.empty())
        return;
    for (const std::vector<uint8_t> &buffer : data)
    {

        if (buffer.size() >= frame_target_size)
        {

            int index = 0;
            int target_num = static_cast<int>(buffer[index++]);
            spdlog::debug("目标数量: {}", target_num);
            int alarm_info = static_cast<int>(buffer[index++]);
            spdlog::debug("报警信息: {}", alarm_info);

            for (int i = 0; i < target_num; ++i)
            {
                std::lock_guard<std::mutex> lock(dataMutex);
                target_info info;
                info.angle = static_cast<int>(buffer[index++] - 0x80);
                info.distance = static_cast<int>(buffer[index++]);
                info.direction = static_cast<int>(buffer[index++]);
                info.speed = static_cast<int>(buffer[index++]);
                info.SNR = static_cast<int>(buffer[index++]);

                spdlog::debug("第{}个目标:", i + 1);
                print_target_info(info);

                if (dataQueue.size() >= MAX_QUEUE_SIZE)
                {
                    spdlog::warn("RadarSerial 收到数据过多, 丢弃队首数据");
                    dataQueue.pop_front(); // 丢弃旧数据
                }
                dataQueue.push_back(info);
            }
            spdlog::debug("**********************************");
        }
        else
        {
            spdlog::error("Incomplete frame!!!");
        }
    }
}

std::string get_date()
{
    std::time_t t = std::time(nullptr);
    std::tm tm;
    localtime_r(&t, &tm); // Linux/macOS
    // localtime_s(&tm, &t); // Windows
    std::ostringstream oss;
    oss << (tm.tm_year + 1900) << "-" << (tm.tm_mon + 1) << "-" << tm.tm_mday;
    return oss.str();
}