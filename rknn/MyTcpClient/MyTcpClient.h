//
// Created by cfan on 2025/1/3.
//

#ifndef LVGL_APP_SRC_CUSTOMIZE_MYTCPCLIENT_MYTCPCLIENT_H_
#define LVGL_APP_SRC_CUSTOMIZE_MYTCPCLIENT_MYTCPCLIENT_H_

#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

class MyTcpClient {
 public:
  MyTcpClient(const std::string& server_ip, int server_port);
  virtual ~MyTcpClient();
  void connectToServer();

  void stop();

  bool receiveMessage(std::string& message);
  bool sendMessage(const std::string& message);

 private:
  /*
   * 自动重连
   */
  void connectLoop();
  void receiverLoop();
  void senderLoop();
  void heartbeatLoop();

 private:
  std::string _server_ip;
  int _server_port;
  int _sock_fd = -1;

  std::atomic<bool> _connect_flag; //是否连接到服务器标志 为真已连接到 为假未连接到
  bool _stop_flag; //是否停止运行标志 为真停止 为假正在运行

  int reconnect_dup = 10; //s

  std::thread *_connect_thread = nullptr;
  std::thread *_receiver_thread = nullptr;
  std::thread *_sender_thread = nullptr;
  std::thread *_heartbeat_thread = nullptr;

  std::queue<std::string> _send_queue;
  std::queue<std::string> _receive_queue;

  std::mutex _send_queue_mutex;
  std::mutex _receive_queue_mutex;
  std::condition_variable _send_queue_cv;
};

#endif //LVGL_APP_SRC_CUSTOMIZE_MYTCPCLIENT_MYTCPCLIENT_H_
