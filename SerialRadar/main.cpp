#include <iostream>
#include "RadarSerial.h"
int main(int, char **)
{
    std::cout << "Hello, from RadarSerial!\n";

    spdlog::set_level(spdlog::level::debug);

    RadarSerial radar("/dev/ttyACM0", B9600);
    if (!radar.openSerial())
        return -1;

    radar.set_frame_flag('N', '\n');
    radar.startReading();

    std::cout << "读取串口数据并打印: " << std::endl;
    while (true)
    {
        std::vector<std::vector<uint8_t>> allData = radar.getAllData();

        if (!allData.empty())
        {
            spdlog::debug("main 收到数据:");
            for (const auto &data : allData)
            {

                if (!data.empty())
                {
                    for (uint8_t byte : data)
                    {
                        if (isprint(byte))
                        {
                            std::cout << (char)byte; // 直接打印可读字符
                        }
                        else if (byte == 0x0D)
                        { // 过滤回车
                            continue;
                        }
                        else if (byte == 0x0A)
                        {
                            std::cout << std::endl;
                        }
                        else
                        {
                            std::cout << "[0x" << std::hex << (int)byte << "]"; // 不可读字符以十六进制显示
                        }
                    }
                }
            }
            std::cout << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    radar.stopReading();
    radar.closeSerial();
}
