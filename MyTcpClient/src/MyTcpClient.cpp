//
// Created by cfan on 2025/1/3.
//

#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <chrono>
#include <future>
#include <signal.h>
#include "MyTcpClient.h"
#include "spdlog/spdlog.h"

#define HEART_BEAT 0

MyTcpClient::MyTcpClient(const std::string &server_ip, int server_port)
    : _server_ip(server_ip), _server_port(server_port), _connect_flag(false), _stop_flag(false) {}

MyTcpClient::~MyTcpClient()
{
    _stop_flag = true;
    _connect_flag = false;
    if (_connect_thread != nullptr && _connect_thread->joinable())
        _connect_thread->join();

    stop();

    std::cout << __func__ << " 成功释放资源" << std::endl;
}
void MyTcpClient::connectToServer()
{
    _connect_flag = false;
    _connect_thread = new std::thread(&MyTcpClient::connectLoop, this);
}
void MyTcpClient::receiverLoop()
{
    char buffer[4096];
    // 设置接收超时时间
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    if (setsockopt(_sock_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1)
    {
        std::cerr << "Setting socket timeout failed" << std::endl;
    }
    while (_connect_flag)
    {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(_sock_fd, buffer, sizeof(buffer) - 1, 0);

        if (bytes_received > 0)
        {
            buffer[bytes_received] = '\0'; // 确保数据以'\0'结尾
            std::lock_guard<std::mutex> lock(_receive_queue_mutex);
            _receive_queue.push(std::string(buffer, bytes_received));

#if 0
            std::cout << "raw data:";
            for (int i = 0; i < bytes_received; ++i)
            {
                printf("0x%02X ", (unsigned char)buffer[i]);
            }
            std::cout << std::endl;
#endif
            std::cout << __func__ << " " << _server_ip << ":" << _server_port << " receive: " << buffer << std::endl;
        }
        else if (bytes_received == 0)
        {
            std::cerr << __func__ << " " << _server_ip << ":" << _server_port
                      << " Connection closed or error during receiving. errno:" << errno << std::endl;
            _connect_flag = false;
            break;
        }
        else if ((bytes_received < 0))
        {
            if (errno == EAGAIN)
            {
                continue;
            }
            std::cerr << __func__ << " " << _server_ip << ":" << _server_port
                      << " Connection closed or error during receiving. errno:" << errno << std::endl;
            _connect_flag = false;
            break;
        }
    }

    std::cout << __func__ << " exit..." << std::endl;
}
void MyTcpClient::connectLoop()
{
    //std::cout << __func__ << " " << _server_ip << ":" << _server_port << " connect start!" << std::endl;
    spdlog::info("MyTcpClient connectLoop start, {0}:{1}", _server_ip, _server_port);

    while (!_connect_flag)
    {

        if (_stop_flag)
        {
            break;
        }
        //_sock_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
        _sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (_sock_fd < 0)
        {
            //std::cerr << __func__ << " Socket creation failed!" << std::endl;
            spdlog::error("MyTcpClient Socket creation failed!");
            return;
        }

        sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(_server_port);
        if (inet_pton(AF_INET, _server_ip.c_str(), &server_addr.sin_addr) <= 0)
        {
            //std::cerr << __func__ << " Invalid server IP address! " << _server_ip << std::endl;
            spdlog::error("Invalid server IP address! address:{}", _server_ip);
            return;
        }

        int res = connect(_sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (res == 0)
        {
            _connect_flag = true;
            _receiver_thread = new std::thread(&MyTcpClient::receiverLoop, this);
            _sender_thread = new std::thread(&MyTcpClient::senderLoop, this);

#if HEART_BEAT
            _heartbeat_thread = new std::thread(&MyTcpClient::heartbeatLoop, this);
#endif

            //std::cout << __func__ << " " << _server_ip << ":" << _server_port << " connect successes!" << std::endl;
            spdlog::info("MyTcpClient connectLoop connect successes!, {0}:{1}", _server_ip, _server_port);
            // break;
        }
        else
        {
            /*std::cerr << __func__ << " " << _server_ip << ":" << _server_port << " Connection failed, retrying ..."
                      << std::endl;*/
            spdlog::error("MyTcpClient {0}:{1} connectLoop connect failed! retrying ...", _server_ip, _server_port);
            close(_sock_fd);
            std::this_thread::sleep_for(std::chrono::seconds(reconnect_dup));
            continue;
        }
        //}

        while (_connect_flag && !_stop_flag)
        {
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }

        if (_stop_flag)
        {
            break;
        }

        stop();
        //std::cerr << "Attempting to reconnect..." << std::endl;
        spdlog::info("MyTcpClient {0}:{1} connectLoop Attempting to reconnect...", _server_ip, _server_port);
    }
    //std::cout << __func__ << " exit..." << std::endl;
    spdlog::info("MyTcpClient {0}:{1} connectLoop exit...", _server_ip, _server_port);
}
void MyTcpClient::senderLoop()
{
    while (_connect_flag)
    {
        std::unique_lock<std::mutex> lock(_send_queue_mutex);
        _send_queue_cv.wait(lock, [this]()
                            { return (!_send_queue.empty() || !_connect_flag); });

        if (!_connect_flag)
            break;

        while (!_send_queue.empty())
        {
            const std::string &message = _send_queue.front();
            if (send(_sock_fd, message.c_str(), message.size(), 0) < 0)
            {
                std::cerr << __func__ << " " << _server_ip << ":" << _server_port << " Failed to send message."
                          << std::endl;
                _connect_flag = false;
                // reconnect();
                break;
            }
            std::cout << __func__ << " " << _server_ip << ":" << _server_port << " send: " << message << std::endl;
            _send_queue.pop();
        }
    }
    std::cout << __func__ << " exit..." << std::endl;
}
void MyTcpClient::heartbeatLoop()
{
    while (_connect_flag)
    {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        if (_connect_flag)
        {
            if (send(_sock_fd, "HEARTBEAT", 9, 0) < 0)
            {
                std::cerr << __func__ << " " << _server_ip << ":" << _server_port
                          << " Heartbeat failed, reconnecting..."
                          << std::endl;
                _connect_flag = false;
            }
        }
    }
    std::cout << __func__ << " exit..." << std::endl;
}
void MyTcpClient::stop()
{

    if (_sock_fd >= 0)
        close(_sock_fd);

    _connect_flag = false;

    if (_receiver_thread != nullptr && _receiver_thread->joinable())
    {
        // pthread_cancel(_receiver_thread->native_handle());
        _receiver_thread->join();
        _receiver_thread = nullptr;
    }

    if (_sender_thread != nullptr && _sender_thread->joinable())
    {
        _send_queue_cv.notify_one();
        _sender_thread->join();
        _sender_thread = nullptr;
    }

    if (_heartbeat_thread != nullptr && _heartbeat_thread->joinable())
    {
        _heartbeat_thread->join();
        _heartbeat_thread = nullptr;
    }

    std::cout << __func__ << " thread exit!" << std::endl;
}
bool MyTcpClient::receiveMessage(std::string &message)
{
    std::lock_guard<std::mutex> lock(_receive_queue_mutex);
    if (_receive_queue.empty())
        return false;
    message = _receive_queue.front();
    _receive_queue.pop();
    return true;
}
bool MyTcpClient::sendMessage(const std::string &message)
{
    if (!_connect_flag)
    {
        std::cout << __func__ << " " << _server_ip << ":" << _server_port << " disconnect!" << std::endl;
        return false;
    }
    std::lock_guard<std::mutex> lock(_send_queue_mutex);
    _send_queue.push(message);
    _send_queue_cv.notify_one();
    return true;
}
