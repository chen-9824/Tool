#include <iostream>
#include "HLK_LD2451.h"
#include "pch.h"

using namespace std;

string dev_name = "/dev/ttyCH341USB0";

int main(int, char **)
{
    spdlog::set_level(spdlog::level::debug);

    shared_ptr<HLK_LD2451> radar = make_shared<HLK_LD2451>(dev_name, B115200);

    if (radar->init() != 0)
    {
        spdlog::error("{} 无法打开", dev_name);
        return -1;
    }

    radar->start_read_thread();

    std::string input;
    while (true)
    {
        std::vector<HLK_LD2451::target_info> data = radar->get_target_data();
        if (!data.empty())
        {
            for (HLK_LD2451::target_info info : data)
            {
                radar->print_target_info(info);
            }
            spdlog::debug("**********************************");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        /*std::cout << "请输入内容（输入 'exit' 退出）: ";
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
            // radar->sendCommand(cmd);
        }*/
    }

    radar->deinit();
}
