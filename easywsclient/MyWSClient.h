#ifndef MYWSCLIENT_H
#define MYWSCLIENT_H

#pragma once

#include <iostream>
#include <thread>

#include <libwebsockets.h>
#include <atomic>
#include <functional>
#include <mutex>
#include <condition_variable>

/*
异步启动WebSocket
自动重连
*/
class MyWSClient
{
public:
  using MessageCallback = std::function<void(const std::string &)>;

  MyWSClient(const std::string &url, int port, const std::string &path, MessageCallback onMessage = nullptr);

  ~MyWSClient();

  void start();

  void stop();

  void sendMessage(const std::string &message);

private:
  std::string url;
  std::string path;
  int port;
  int detect_dup = 3; // s
  MessageCallback onMessageCallback;
  struct lws_context *context;
  struct lws *wsi;
  std::thread wsThread;
  std::atomic<bool> running;
  std::mutex sendMutex;
  std::vector<std::string> sendQueue;


  void run();
  void reconnect();
  static int callback(struct lws *wsi, enum lws_callback_reasons reason,
                      void *user, void *in, size_t len);
  static struct lws_protocols protocols[];
};

#endif