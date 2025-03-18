#include "FolderMonitor.h"

#include <vector>

#include "spdlog/spdlog.h"

namespace fs = std::filesystem;
FolderMonitor::FolderMonitor(const std::string &folderPath, uintmax_t maxSize,
                             int daysToDelete)
    : folderPath(folderPath),
      maxSize(maxSize),
      daysToDelete(daysToDelete),
      running(false) {}

FolderMonitor::~FolderMonitor() { stop(); }

void FolderMonitor::start()
{
  running = true;
  monitorThread = std::thread(&FolderMonitor::monitor, this);
}

void FolderMonitor::stop()
{
  {
    std::lock_guard<std::mutex> lock(mtx);
    running = false;
  }
  cv.notify_all();
  if (monitorThread.joinable())
  {
    monitorThread.join();
  }
}

void FolderMonitor::set_multiple_directories(const std::string &rootPath, const std::string &detectPath, const std::vector<std::string> &other_floder)
{
  _multi_floder_enable = true;
  _rootPath = rootPath;
  _detectPath = detectPath;
  _other_floder = other_floder;
}

void FolderMonitor::monitor()
{
  while (running)
  {
    uintmax_t t_folderSize = getFolderSize();
    spdlog::info("待检测目录: {}, 当前目录大小: {} MB, 限制大小:{} MB", folderPath,
                 t_folderSize / (1024 * 1024), maxSize / (1024 * 1024));
    spdlog::info("删除最老 {} 天的文件...", daysToDelete);

    if (t_folderSize > maxSize)
    {
      spdlog::warn("目录超出阈值，开始删除最老的 {} 天的文件...", daysToDelete);
      deleteOldFiles();
      continue;
    }

    sleepUntilMidnight(); // 休眠到凌晨 00:00

    if (!running)
      break;

    t_folderSize = getFolderSize();

    spdlog::info("{} ,当前目录大小: {} MB, 限制大小:{} MB", folderPath,
                 t_folderSize / (1024 * 1024), maxSize / (1024 * 1024));

    if (t_folderSize > maxSize)
    {
      spdlog::warn("目录超出阈值，开始删除最老的 {} 天的文件...", daysToDelete);
      deleteOldFiles();
    }
  }
}

// 获取文件夹大小
uintmax_t FolderMonitor::getFolderSize()
{
  uintmax_t totalSize = 0;
  std::string path = folderPath;
  if (_multi_floder_enable)
  {
    path = _rootPath;
  }
  for (const auto &entry : fs::recursive_directory_iterator(path))
  {
    if (fs::is_regular_file(entry.path()))
    {
      totalSize += fs::file_size(entry.path());
    }
  }
  return totalSize;
}

// 删除最老的 N 天的文件
void FolderMonitor::deleteOldFiles()
{
  std::vector<std::pair<fs::path, std::time_t>> files;

  try
  {
    std::string detectPath = folderPath;
    if (_multi_floder_enable)
    {
      detectPath = _detectPath;
    }

    for (const auto &entry : fs::directory_iterator(detectPath))
    {
      if (fs::is_regular_file(entry.path()))
      {
        auto ftime = fs::last_write_time(entry.path());
        // 转换为 system_clock::time_point
        auto sctp =
            std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                ftime - std::filesystem::file_time_type::clock::now() +
                std::chrono::system_clock::now());
        // 转换为 time_t
        std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);

        files.emplace_back(entry.path(), cftime);
      }
    }

    // **按修改时间排序**
    std::sort(files.begin(), files.end(),
              [](const auto &a, const auto &b)
              { return a.second < b.second; });

    char buffer[100];
#if 0
    spdlog::debug("检测目录: {}, 所有文件: ", detectPath);
    for (const auto &[path, modTime] : files)
    {
      std::tm *localTime = std::localtime(&modTime);
      std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localTime);
      spdlog::debug("{} , {} ", path.c_str(), buffer);
    }
#endif
    std::time_t del_time = files[0].second;
    std::tm last_file_time = *(std::localtime(&del_time));
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &last_file_time);
    spdlog::info("当前最老文件创建时间为: {} ", buffer);
    last_file_time.tm_mday += daysToDelete;
    last_file_time.tm_hour = 0;
    last_file_time.tm_min = 0;
    last_file_time.tm_sec = 0;
    char del_buffer[100];
    std::strftime(del_buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &last_file_time);
    spdlog::info(" {} 以前文件均删除 ", del_buffer);

    del_time = std::mktime(&last_file_time);

    // **删除最老的 N 天文件**
    for (const auto &[path, modTime] : files)
    {
      if (modTime < del_time)
      {
        std::tm *localTime = std::localtime(&modTime);
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localTime);
        spdlog::warn("删除 {}, {}", path.c_str(), buffer);
        fs::remove(path);
        if (_multi_floder_enable)
        {
          for (auto other_path : _other_floder)
          {
            fs::path p1 = other_path / path.filename();
            fs::remove(p1);
            spdlog::warn("删除 {}, {}", p1.c_str(), buffer);
          }
        }
      }
      else
      {
        break;
      }
    }
    if (files.empty())
    {
      spdlog::warn("当前该文件夹下文件均被删除\n\n\n");
    }
    else
    {
      spdlog::warn("{} 以前文件均删除, 当前最老记录为:{}, {}\n\n\n", del_buffer, files[0].first.c_str(), buffer);
    }
  }
  catch (const std::exception &e)
  {
    spdlog::error("删除文件时出错: {}", e.what());
  }

  // 避免过快
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
}

// 休眠至午夜
void FolderMonitor::sleepUntilMidnight()
{
  auto now = std::chrono::system_clock::now();
  auto nowTime = std::chrono::system_clock::to_time_t(now);

  std::tm *nowTm = std::localtime(&nowTime);
  nowTm->tm_hour = 0;
  nowTm->tm_min = 0;
  nowTm->tm_sec = 0;

  auto midnight = std::chrono::system_clock::from_time_t(std::mktime(nowTm)) +
                  std::chrono::hours(24);
  // 转换为 time_t
  std::time_t midnightTime = std::chrono::system_clock::to_time_t(midnight);

  // 转换为本地时间
  std::tm *midnightTm = std::localtime(&midnightTime);

  // 格式化成年月日时分秒
  char buffer[100];
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", midnightTm);

  spdlog::info("下次检测时间: {}", buffer);

  std::this_thread::sleep_until(midnight);
}