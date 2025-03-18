#include <iostream>
#include "FolderMonitor.h"
#include <thread>
#include <chrono>
#include "spdlog/spdlog.h"

int main(int, char **)
{
   spdlog::set_level(spdlog::level::debug);

   FolderMonitor _monitor("/home/ido/chenFan/Tool/FolderMonitor/test/1", 10 * 1024 * 1024, 3);
   std::vector<std::string> _other_floder{
       "/home/ido/chenFan/Tool/FolderMonitor/test/2",
       "/home/ido/chenFan/Tool/FolderMonitor/test/3"};
   _monitor.set_multiple_directories("/home/ido/chenFan/Tool/FolderMonitor/test", "/home/ido/chenFan/Tool/FolderMonitor/test/1", _other_floder);
   _monitor.start();
   std::cout << "Hello, from FolderMonitor!\n";
   while (1)
   {
      std::this_thread::sleep_for(std::chrono::seconds(1));
   }
}
