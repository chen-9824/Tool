#ifndef FOLDERMONITOR_H
#define FOLDERMONITOR_H

#pragma once
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <thread>
class FolderMonitor
{
public:
  /*
  监控指定文件夹大小，超过指定大小即进行删除操作，删除早于指定时间跨度的所有文件
 (例如若超过指定大小，指定文件夹中最早文件创建时间为3-10，指定时间跨度为3，则删除3-13之前的所有文件)
 */
  // maxSize:超过该值即进行删除操作, 单位为字节
  // daysToDelete: 删除时删除最老的 daysToDelete 天的文件
  FolderMonitor(const std::string &folderPath, uintmax_t maxSize,
                int daysToDelete);
  ~FolderMonitor();

  void start(); // 启动监控线程
  void stop();  // 停止监控线程

  // 监控多个目录, 启动监控线程前使用, 当调用该函数后构造函数指定的检测路径folderPath将失去作用
  // rootPath:根目录，用于检测该目录大小是否超过限制
  // detectPath：检测文件目录，在该目录中对文件进行修改时间排序，用于删除最老文件
  // other_floder：联动删除的其他文件夹，被删除文件名与 detectPath 检测到的文件名一样, 可置空
  void set_multiple_directories(const std::string &rootPath, const std::string &detectPath, const std::vector<std::string> &other_floder);

private:
  std::string folderPath;
  uintmax_t maxSize;
  int daysToDelete;
  bool running;
  std::thread monitorThread;
  std::mutex mtx;
  std::condition_variable cv;

  bool _multi_floder_enable = false;
  std::string _rootPath;
  std::string _detectPath;
  std::vector<std::string> _other_floder;

  void monitor();            // 监控任务
  uintmax_t getFolderSize(); // 获取文件夹大小
  void deleteOldFiles();     // 删除最老的 N 天的文件
  void sleepUntilMidnight(); // 休眠至午夜
};

#endif