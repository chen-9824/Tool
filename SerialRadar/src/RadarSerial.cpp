#include "RadarSerial.h"
#include "../pch.h"

#define MAX_QUEUE_SIZE 1000 // 限制缓冲队列最大大小

RadarSerial::RadarSerial(const std::string &port, int baudrate)
    : port(port), baudrate(baudrate), fd(-1), running(false), pausing(false) {}

RadarSerial::~RadarSerial()
{
    stopReading();
    closeSerial();
}

void RadarSerial::set_frame_flag(uint8_t frame_start_flag, uint8_t frame_end_flag)
{
    _complete_frame_enable = true;
    _frame_start_flag = frame_start_flag; // 帧起始字符
    _frame_end_flag = frame_end_flag;     // 帧结束字符
}

void RadarSerial::set_frame_flag(int frame_min_size,
                                 std::vector<uint8_t> frame_header,
                                 std::vector<uint8_t> frame_tail)
{
    _frame_min_size = frame_min_size;
    _frame_header = frame_header;
    _frame_tail = frame_tail;
}

bool RadarSerial::openSerial()
{
    std::lock_guard<std::mutex> lock(ioMutex);
    fd = open(port.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1)
    {
        std::cerr << "无法打开串口: " << strerror(errno) << std::endl;
        return false;
    }
    struct termios options;
    tcgetattr(fd, &options);
    cfsetispeed(&options, baudrate);
    cfsetospeed(&options, baudrate);
    options.c_cflag &= ~PARENB;                         // 不启用奇偶校验
    options.c_cflag &= ~CSTOPB;                         // 使用 1 个停止位
    options.c_cflag &= ~CSIZE;                          // 清空数据位设置
    options.c_cflag |= CS8;                             // 设置数据位为 8 位
    options.c_cflag |= (CLOCAL | CREAD);                // 忽略 modem 控制线 启用接收功能
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // 闭回显，使用非规范模式（避免 \n 换行等待）
    options.c_cc[VMIN] = 0;                             // 至少读取 0 个字节才返回（非阻塞模式）
    options.c_cc[VTIME] = 10;                           // 读取超时时间，单位为 100ms（即 1 秒超时）。
    options.c_iflag &= ~(IXON | IXOFF | IXANY);         // 关闭软件流控制
    tcsetattr(fd, TCSANOW, &options);

    _available = true;
    return true;
}

void RadarSerial::closeSerial()
{
    _available = false;
    std::lock_guard<std::mutex> lock(ioMutex);
    if (fd != -1)
    {
        close(fd);
        fd = -1;
    }
}

bool RadarSerial::is_available()
{
    return _available;
}

void RadarSerial::startReading()
{
    // 避免多次启动
    if (running.load())
        return;
    running.store(true);
    readThread = std::thread(&RadarSerial::readLoop, this);
}

void RadarSerial::stopReading()
{
    running.store(false);
    if (readThread.joinable())
    {
        readThread.join();
    }
}

void RadarSerial::pauseReading() { pausing.store(true); }

void RadarSerial::resumeReading() { pausing.store(false); }

void RadarSerial::readLoop()
{
    if (fd == -1)
    {
        spdlog::error("Serial port not open");
        return;
    }

    std::vector<uint8_t> buffer(256);
    std::vector<uint8_t> frameBuffer; // 存放拼接后的完整帧

    while (running.load())
    {
        while (pausing)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            if (!running.load())
                break;
        }

        std::lock_guard<std::mutex> lock(ioMutex);
        int bytesRead = read(fd, buffer.data(), buffer.size());
        if (bytesRead > 0)
        {
            buffer.resize(bytesRead);
            {
                std::lock_guard<std::mutex> dataLock(dataMutex);
                if (_complete_frame_enable)
                {
                    // spdlog::debug("RadarSerial 收到数据:");
                    for (uint8_t byte : buffer)
                    {
                        // std::cout << "[0x" << std::hex << (int)byte << "]";
                        if (byte == _frame_start_flag)
                        {
                            frameBuffer.clear(); // 遇到起始字符，清空缓冲区
                        }
                        frameBuffer.push_back(byte);
                        if (byte == _frame_end_flag)
                        {
                            // 收到结束字符，表示帧完整，加入队列
                            if (dataQueue.size() >= MAX_QUEUE_SIZE)
                            {
                                spdlog::warn("RadarSerial 收到数据过多, 丢弃队首数据");
                                dataQueue.pop_front(); // 丢弃旧数据
                            }
                            dataQueue.push_back(frameBuffer);
                            frameBuffer.clear(); // 清空，准备接收下一帧
                        }
                    }
                    // std::cout << std::endl;
                }
                else
                {

                    if (dataQueue.size() >= MAX_QUEUE_SIZE)
                    {
                        spdlog::warn("RadarSerial 收到数据过多, 丢弃队首数据");
                        dataQueue.pop_front(); // 丢弃旧数据
                    }
                    dataQueue.push_back(buffer);
                    // frameBuffer.clear();
                }
            }
        }
        buffer.resize(256);
        // std::this_thread::sleep_for(std::chrono::milliseconds(100));
        //  usleep(100 000);
    }
}

bool RadarSerial::sendCommand(const std::vector<uint8_t> &cmd)
{
    if (fd == -1)
    {
        spdlog::error("Serial port not open");
        return -1;
    }

    {
        std::lock_guard<std::mutex> lock(ioMutex);
        int bytesWritten = write(fd, cmd.data(), cmd.size());
        if (bytesWritten < 0)
        {
            spdlog::error("RadarSerial sendCommand 失败: {}", strerror(errno));
            return false;
        }
        spdlog::debug("RadarSerial sendCommand 成功: {}", std::string(cmd.begin(), cmd.end()));
    }

    return true;
}

std::vector<uint8_t> RadarSerial::getLatestData()
{
    std::lock_guard<std::mutex> dataLock(dataMutex);
    if (!dataQueue.empty())
    {
        std::vector<uint8_t> data = dataQueue.front();
        dataQueue.pop_front();
        return data;
    }
    return {};
}

std::vector<std::vector<uint8_t>> RadarSerial::cutFrame(std::vector<uint8_t> &buffer, const std::vector<uint8_t> &frame_header, const std::vector<uint8_t> &frame_tail, int complete_frame_size)
{
    std::vector<std::vector<uint8_t>> allParseFrame;
    if (buffer.size() < complete_frame_size)
    {
        spdlog::trace("frame smaller than complete_frame_size");
        return allParseFrame;
    }

    auto it = buffer.begin();

    while (true)
    {
        // 1. 查找帧头
        it = std::search(it, buffer.end(), frame_header.begin(), frame_header.end());

        if (it == buffer.end())
        {
            break; // 没找到帧头，结束解析
        }

        // 2. 计算帧头位置
        size_t header_pos = std::distance(buffer.begin(), it);
        spdlog::trace("Found frame header at position: {}", header_pos);

        // 3. 检查是否有足够的数据读取帧内数据长度
        if (std::distance(it, buffer.end()) < frame_header.size() + 2)
        {
            spdlog::error("Incomplete frame, waiting for more data...");
            break;
        }

        // 4. 读取帧内数据长度（小端格式，长度字段占 2 字节）
        size_t length_pos = header_pos + frame_header.size();
        uint16_t data_length = buffer[length_pos] | (buffer[length_pos + 1] << 8);

        spdlog::trace("Frame data length: {}", data_length);

        // 5. 计算完整帧长度
        size_t frame_size = frame_header.size() + 2 + data_length + frame_tail.size();

        // 6. 检查数据是否足够完整帧
        if (std::distance(it, buffer.end()) < frame_size)
        {
            spdlog::error("Incomplete frame, waiting for more data...");
            break;
        }

        // 7. 检查帧尾
        size_t tail_pos = header_pos + frame_size - frame_tail.size();
        if (!std::equal(buffer.begin() + tail_pos, buffer.begin() + tail_pos + frame_tail.size(), frame_tail.begin()))
        {
            spdlog::error("Frame tail mismatch, discarding frame.");
            it++; // 继续查找下一个帧头
            continue;
        }

        // 8. 提取帧数据
        std::vector<uint8_t> frame_data(buffer.begin() + length_pos + 2, buffer.begin() + tail_pos);
        allParseFrame.push_back(frame_data);
#if 0
        std::cout
            << "Extracted frame data: ";
        for (uint8_t byte : frame_data)
        {
            std::cout << std::hex << static_cast<int>(byte) << " ";
        }
        std::cout << std::dec << std::endl;
#endif
        // 9. 移除已解析的数据
        buffer.erase(buffer.begin(), buffer.begin() + frame_size);

        // 10. 继续查找下一帧
        it = buffer.begin();
    }

    return allParseFrame;
}

std::vector<std::vector<uint8_t>> RadarSerial::parseFrame(std::vector<uint8_t> &buffer)
{
    return cutFrame(buffer, _frame_header, _frame_tail, _frame_min_size);
}

void RadarSerial::printf_uint8(const std::vector<uint8_t> &data)
{
    if (!data.empty())
    {
        std::string hex_str;
        for (uint8_t byte : data)
        {
            // std::cout << "[0x" << std::hex << (int)byte << "]";
            hex_str += fmt::format("[0x{:02X}] ", byte);
        }
        // std::cout << std::endl;
        spdlog::debug("{}", hex_str);
    }
}

std::vector<std::vector<uint8_t>> RadarSerial::getAllData()
{
    std::lock_guard<std::mutex> dataLock(dataMutex);
    std::vector<std::vector<uint8_t>> allData;
    while (!dataQueue.empty())
    {
        allData.push_back(dataQueue.front());
        dataQueue.pop_front();
    }
    return allData;
}