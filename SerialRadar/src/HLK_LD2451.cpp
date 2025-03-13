#include "HLK_LD2451.h"
#include "../pch.h"

#define READ_LOG_FIEL 0 // 是否将读数据线程打印写入到日志文件，1写入，0打印到控制台

using namespace std;

static std::string get_date();

const int frame_min_size = 17; // 至少包含一个目标数据
const int frame_target_size = 7;
const std::vector<uint8_t> frame_header{0xf4, 0xf3, 0xf2, 0xf1};
const std::vector<uint8_t> frame_end{0xf8, 0xf7, 0xf6, 0xf5};

HLK_LD2451::HLK_LD2451(const std::string &port, int baudrate) : RadarSerial(port, baudrate)
{
    _reading.store(false);
}

HLK_LD2451::~HLK_LD2451()
{
    deinit();
}

int HLK_LD2451::init()
{
    _radar = make_shared<RadarSerial>("/dev/ttyCH341USB0", B115200);

    if (!_radar->openSerial())
    {
        spdlog::error("/dev/ttyCH341USB0 无法打开");
        return -1;
    }

    spdlog::info("雷达串口已打开");
    _available = true;

    _radar->set_frame_flag(frame_min_size, frame_header, frame_end);
    return 0;
}

void HLK_LD2451::deinit()
{

    _reading.store(false);
    if (_read_t.joinable())
    {
        _read_t.join();
    }

    _radar->stopReading();
    _radar->closeSerial();
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

void HLK_LD2451::read_thread(int id, HLK_LD2451 *radar)
{
    if (!radar || !radar->_available)
    {
        return;
    }
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
        radar->_radar->startReading();

        while (radar->_reading.load())
        {
            std::vector<std::vector<uint8_t>> allData = radar->_radar->getAllData();

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
                        for (uint8_t byte : data)
                        {
                            /*if (isprint(byte))
                            {
                                std::cout << (char)byte; // 直接打印可读字符
                            }
                            else
                            {
                                std::cout << "[0x" << std::hex << (int)byte << "]"; // 不可读字符以十六进制显示
                            }*/
                            std::cout << "[0x" << std::hex << (int)byte << "]";
                        }
                        std::cout << std::endl;

                        radar->parse_hlk_radar_data(radar->_radar->parseFrame(data));
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