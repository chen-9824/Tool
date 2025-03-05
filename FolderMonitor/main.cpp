#include <iostream>
#include "FolderMonitor.h"
#include <thread>
#include <chrono>
#include "spdlog/spdlog.h"

int main(int, char **)
{
   spdlog::set_level(spdlog::level::debug);

   FolderMonitor _monitor("/home/ido/chenFan/FolderMonitor/test", 5 * 1024 * 1024, 3);
   _monitor.start();
   std::cout << "Hello, from FolderMonitor!\n";
   while (1)
   {
      std::this_thread::sleep_for(std::chrono::seconds(1));
   }
}
