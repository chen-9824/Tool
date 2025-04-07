#include "MyWSClient.h"

#include "spdlog/spdlog.h"

using namespace std;

MyWSClient::MyWSClient(const std::string &url, int port, const std::string &path, MessageCallback onMessage)
    : url(url), port(port), path(path), onMessageCallback(onMessage), context(nullptr), wsi(nullptr), running(false) {}

MyWSClient::~MyWSClient() { stop(); }

void MyWSClient::start()
{
  running = true;
  wsThread = std::thread(&MyWSClient::run, this);
}

void MyWSClient::stop()
{

  if (running == false)
    return;

  running = false;

  if (wsThread.joinable())
  {
    wsThread.join();
  }

  if (context)
  {
    lws_context_destroy(context);
    context = nullptr;
  }
}

void MyWSClient::sendMessage(const string &message)
{

  if (!wsi)
  {
    std::cout << __func__ << " error send, ws server not connected" << std::endl;
    return;
  }

  std::lock_guard<std::mutex> lock(sendMutex);
  sendQueue.push_back(message);
  if (wsi)
  {
    int res = lws_callback_on_writable(wsi);
    std::cout << "MyWSClient " << __func__ << " send :" << message << ", res:" << res << std::endl;
  }
}
void MyWSClient::run()
{
  struct lws_context_creation_info ctx_info = {};
  struct lws_client_connect_info conn_info = {};

  ctx_info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
  ctx_info.port = CONTEXT_PORT_NO_LISTEN;
  ctx_info.protocols = protocols;
  context = lws_create_context(&ctx_info);

  if (!context)
  {
    std::cerr << "[WebSocket] Failed to create context" << std::endl;
    return;
  }

  conn_info.context = context;
  conn_info.address = url.c_str();
  conn_info.port = port;
  conn_info.path = path.c_str();
  conn_info.host = url.c_str();
  conn_info.origin = url.c_str();
  conn_info.protocol = "ws";
  conn_info.ssl_connection = LCCSCF_USE_SSL |
                             LCCSCF_ALLOW_SELFSIGNED |
                             LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK |
                             LCCSCF_ALLOW_EXPIRED;
  conn_info.userdata = this;

  long last_time = std::chrono::duration_cast<std::chrono::seconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                       .count();

  while (running)
  {
    if (!wsi)
    {
      std::this_thread::sleep_for(std::chrono::seconds(3));
      std::cout << "[WebSocket] Attempting to connect..." << std::endl;
      wsi = lws_client_connect_via_info(&conn_info);
    }

    // 判断wsi是否可用
    if ((std::chrono::duration_cast<std::chrono::seconds>(
             std::chrono::system_clock::now().time_since_epoch())
             .count() -
         last_time) > detect_dup)
    {
      if (lws_callback_on_writable(wsi) <= 0)
      {

        std::cerr
            << "[WebSocket] Connection failed, retrying in 3s..." << std::endl;
        wsi = nullptr;
        // continue;
      }
      last_time = std::chrono::duration_cast<std::chrono::seconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count();
    }

    if (lws_service(context, 0) < 0)
    {
      std::cerr
          << "[WebSocket] lws_service failed" << std::endl;
    }
  }

  // lws_context_destroy(context);
}

void MyWSClient::reconnect()
{
  stop();
  std::cerr
      << "[WebSocket] reconnect, retrying in 3s..." << std::endl;
  run();
}

int MyWSClient::callback(lws *wsi, lws_callback_reasons reason, void *user, void *in, size_t len)
{
  MyWSClient *client = (MyWSClient *)lws_wsi_user(wsi);

  switch (reason)
  {
  case LWS_CALLBACK_CLIENT_ESTABLISHED:
  {
    std::cout << "[WebSocket] Connected to server" << std::endl;
    lws_callback_on_writable(wsi); // 请求写入
    break;
  }

  case LWS_CALLBACK_CLIENT_RECEIVE:
  {
    if (client->onMessageCallback)
    {
      client->onMessageCallback(std::string((char *)in, len));
    }
    break;
  }

  case LWS_CALLBACK_CLIENT_WRITEABLE:
  {
    std::lock_guard<std::mutex> lock(client->sendMutex);
    if (!client->sendQueue.empty())
    {
      std::string message = client->sendQueue.front();
      client->sendQueue.erase(client->sendQueue.begin());

      // std::cout << __func__ << " send:" << message << std::endl;

      // 确保 LWS_PRE 字节已预留
      unsigned char buf[LWS_PRE + 1024] = {0};
      int msgLen = message.size();
      memcpy(buf + LWS_PRE, message.c_str(), msgLen);

      int sent = lws_write(wsi, buf + LWS_PRE, msgLen, LWS_WRITE_TEXT);
      if (sent < 0)
      {
        std::cerr << __func__ << "lws_write failed!" << std::endl;
      }

      // 如果还有数据，继续请求写入
      if (!client->sendQueue.empty())
      {
        lws_callback_on_writable(wsi);
      }
    }
    break;
  }

  case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
  {
    std::cerr << "[WebSocket] Connection error: " << (in ? (char *)in : "unknown") << std::endl;
    client->wsi = nullptr;
    // client->reconnect();
    break;
  }

  case LWS_CALLBACK_CLOSED:
  {
    std::cout << "[WebSocket] Connection closed" << std::endl;
    client->wsi = nullptr;
    // client->reconnect();
    break;
  }

  default:
    break;
  }
  return 0;
}

// 定义协议数组
struct lws_protocols MyWSClient::protocols[] = {
    {"ws", MyWSClient::callback, 0, 4096},
    {nullptr, nullptr, 0, 0}};
