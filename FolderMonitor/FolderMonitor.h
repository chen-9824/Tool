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
class FolderMonitor {
 public:
 /*
 监控指定文件夹大小，超过指定大小即进行删除操作，删除早于指定时间跨度的所有文件
(例如若超过指定大小，指定文件夹中最早文件创建时间为3-10，指定时间跨度为3，则删除3-13之前的所有文件)
*/
  // maxSize:超过该值即进行删除操作, 单位为字节
  // daysToDelete: 删除时删除最老的 daysToDelete 天的文件
  FolderMonitor(const std::string& folderPath, uintmax_t maxSize,
                int daysToDelete);
  ~FolderMonitor();

  void start();  // 启动监控线程
  void stop();   // 停止监控线程

 private:
  std::string folderPath;
  uintmax_t maxSize;
  int daysToDelete;
  bool running;
  std::thread monitorThread;
  std::mutex mtx;
  std::condition_variable cv;

  void monitor();             // 监控任务
  uintmax_t getFolderSize();  // 获取文件夹大小
  void deleteOldFiles();      // 删除最老的 N 天的文件
  void sleepUntilMidnight();  // 休眠至午夜
};

#endif