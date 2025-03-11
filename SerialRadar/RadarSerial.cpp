#include "RadarSerial.h"
#include "pch.h"

#define MAX_QUEUE_SIZE 1000 // 限制缓冲队列最大大小

RadarSerial::RadarSerial(const std::string &port, int baudrate)
    : port(port), baudrate(baudrate), fd(-1), running(false) {}

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
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    options.c_cflag |= (CLOCAL | CREAD);
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_cc[VMIN] = 1;
    options.c_cc[VTIME] = 10;
    tcsetattr(fd, TCSANOW, &options);
    return true;
}

void RadarSerial::closeSerial()
{
    std::lock_guard<std::mutex> lock(ioMutex);
    if (fd != -1)
    {
        close(fd);
        fd = -1;
    }
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

void RadarSerial::readLoop()
{
    std::vector<uint8_t> buffer(256);
    std::vector<uint8_t> frameBuffer; // 存放拼接后的完整帧

    while (running.load())
    {
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
            // parseData(buffer);
        }
        buffer.resize(256);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        // usleep(100 000);
    }
}

bool RadarSerial::sendCommand(const std::vector<uint8_t> &cmd)
{
    std::lock_guard<std::mutex> lock(ioMutex);
    int bytesWritten = write(fd, cmd.data(), cmd.size());
    if (bytesWritten < 0)
    {
        std::cerr << "发送失败: " << strerror(errno) << std::endl;
        return false;
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

void RadarSerial::parseData(const std::vector<uint8_t> &data)
{
    if (data.size() < 5)
        return;
    if (data[0] != 0x55 || data[1] != 0xAA)
        return;
    int dataLen = data[2] << 8 | data[3];
    if (data.size() < dataLen + 5)
        return;
    uint8_t checksum = 0;
    for (size_t i = 0; i < dataLen + 4; i++)
        checksum += data[i];
    if (checksum != data[dataLen + 4])
    {
        std::cerr << "校验失败！" << std::endl;
        return;
    }
    std::cout << "解析到数据: ";
    for (size_t i = 4; i < dataLen + 4; i++)
    {
        std::cout << std::hex << (int)data[i] << " ";
    }
    std::cout << std::endl;
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