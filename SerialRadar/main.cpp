#include <iostream>
#include "./src/HLK_LD2451.h"
#include "pch.h"

using namespace std;

string dev_name = "/dev/ttyCH341USB0";
string test_name = "/dev/ttyUSB6";

int main(int, char **)
{

    spdlog::set_level(spdlog::level::debug);
    // spdlog::set_pattern("[%Y-%m-%d %H:%M:%S] [%^%l%$] [%s:%# %!()] %v");

    shared_ptr<HLK_LD2451>
        radar = make_shared<HLK_LD2451>(test_name, B115200);

    if (radar->init() != 0)
    {
        spdlog::error("{} 无法打开", dev_name);
        return -1;
    }

    radar->start_read_thread();

    std::string input;
    while (true)
    {
#if 0
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
#endif

#if 1
        std::cout << "请输入内容（输入 'exit' 退出）: ";
        std::getline(std::cin, input);
        if (input == "exit")
        {
            std::cout << "程序退出。" << std::endl;
            break;
        }
        std::cout << "你输入的是：" << input << std::endl;

        if (input == "1")
        {
            radar->enable_cfg_mode();
        }
        else if (input == "2")
        {
            radar->disable_cfg_mode();
        }
        else if (input == "3")
        {
            radar->read_target_cfg();
        }
#endif
    }

    radar->deinit();
}
