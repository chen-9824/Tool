#include <iostream>
#include "RadarSerial.h"
#include "pch.h"

using namespace std;

void read_thread(int id, shared_ptr<RadarSerial> radar);
std::string get_date();

int main(int, char **)
{
    std::cout << "Hello, from RadarSerial!\n";

    spdlog::set_level(spdlog::level::debug);

    shared_ptr<RadarSerial> radar = make_shared<RadarSerial>("/dev/ttyACM0", B9600);

    if (!radar->openSerial())
        return -1;
    spdlog::info("雷达串口已打开");

    // radar->set_frame_flag('N', '\n');

    thread t(read_thread, 1, radar);

    std::string input;
    while (true)
    {
        std::cout << "请输入内容（输入 'exit' 退出）: ";
        std::getline(std::cin, input);
        if (input == "exit")
        {
            std::cout << "程序退出。" << std::endl;
            break;
        }
        std::cout << "你输入的是：" << input << std::endl;

        vector<uint8_t> cmd;
        std::cout << "ASCII 码：";
        for (char ch : input)
        {
            uint8_t d = static_cast<uint8_t>(ch);
            std::cout << d << " ";
            cmd.push_back(d);
        }
        std::cout << std::endl;

        if (cmd[0] == 'S')
        {
            cmd.push_back(static_cast<uint8_t>('\r'));
            cmd.push_back(static_cast<uint8_t>('\n'));
            radar->sendCommand(cmd);
        }
    }

    radar->stopReading();
    radar->closeSerial();
}

void read_thread(int id, shared_ptr<RadarSerial> radar)
{
    auto logger = spdlog::basic_logger_mt("thread_" + std::to_string(id), "/home/firefly/chenFan/Tool/SerialRadar/log/thread_" + std::to_string(id) + "_" + get_date() + ".log");
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [thread %t] [%l] %v");
    try
    {
        radar->startReading();
        logger->info("线程 {}: 运行中...读取串口数据并打印: ", id);
        while (true)
        {
            std::vector<std::vector<uint8_t>> allData = radar->getAllData();

            if (!allData.empty())
            {
                // logger->debug("main 收到数据:");
                //  spdlog::debug("main 收到数据:");
                for (const auto &data : allData)
                {
#if 0
                    if (!data.empty())
                    {
                        /*if (data[0] == 'N')
                        {
                            continue;
                        }*/
                        string s(data.begin(), (data.end() - 2));
                        if (s.find('O') != string::npos || s.find('B') != string::npos)
                        {
                            // logger->debug("{}", s);
                            spdlog::debug("{}", s);
                        }
                        // logger->debug("{}", s);
                    }
#else
                    if (!data.empty())
                    {
                        std::cout << "收到数据: ";
                        for (uint8_t byte : data)
                        {
                            if (isprint(byte))
                            {
                                std::cout << (char)byte; // 直接打印可读字符
                            }
                            else
                            {
                                std::cout << "[0x" << std::hex << (int)byte << "]"; // 不可读字符以十六进制显示
                            }
                        }
                        std::cout << std::endl;
                    }
#endif
                }

                // logger->debug("====================================");
                //  spdlog::debug("====================================");
                //    std::cout << std::endl;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "获取数据时发生异常: " << e.what() << std::endl;
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