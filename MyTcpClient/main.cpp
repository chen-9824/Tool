#include <iostream>
#include <zconf.h>
#include <csignal>
#include <iconv.h>
#include "MyTcpClient.h"

using namespace std;

static int quit = false;

extern "C"
{
    static void sigterm_handler(int sig);
}

static void sigterm_handler(int sig)
{
    cout << "get signal " << sig << endl;
    quit = true;
}

std::string convert_utf8_to_gbk(const std::string &input)
{
    iconv_t cd = iconv_open("GBK", "UTF-8");
    if (cd == (iconv_t)-1)
    {
        std::cerr << "iconv_open failed" << std::endl;
        return "";
    }

    size_t in_len = input.size();
    size_t out_len = in_len * 2; // GBK 编码字符可能比 UTF-8 更长
    char *in_buf = const_cast<char *>(input.c_str());
    char *out_buf = new char[out_len];
    char *out_ptr = out_buf;

    size_t result = iconv(cd, &in_buf, &in_len, &out_ptr, &out_len);
    if (result == (size_t)-1)
    {
        std::cerr << "iconv failed" << std::endl;
        iconv_close(cd);
        delete[] out_buf;
        return "";
    }

    std::string output(out_buf, out_ptr - out_buf);
    iconv_close(cd);
    delete[] out_buf;
    return output;
}

std::string convert_gbk_to_utf8(const std::string &input)
{
    // 打开转换描述符
    iconv_t cd = iconv_open("UTF-8", "GBK");
    if (cd == (iconv_t)-1)
    {
        std::cerr << "iconv_open failed" << std::endl;
        return "";
    }

    size_t in_len = input.size();
    size_t out_len = in_len * 2; // UTF-8 编码字符可能比 GBK 更长
    char *in_buf = const_cast<char *>(input.c_str());
    char *out_buf = new char[out_len];
    char *out_ptr = out_buf;

    // 转换字符
    size_t result = iconv(cd, &in_buf, &in_len, &out_ptr, &out_len);
    if (result == (size_t)-1)
    {
        std::cerr << "iconv failed" << std::endl;
        iconv_close(cd);
        delete[] out_buf;
        return "";
    }

    // 将转换后的数据存入 std::string
    std::string output(out_buf, out_ptr - out_buf);
    iconv_close(cd);
    delete[] out_buf;
    return output;
}

int main()
{
    signal(SIGINT, sigterm_handler);

    std::cout << "Hello, World!" << std::endl;
    MyTcpClient *_client = new MyTcpClient("192.168.51.87", 12345);
    _client->connectToServer();

    std::string msg;

    while (!quit)
    {
        sleep(1);
        if (_client->receiveMessage(msg))
        {
            std::string data = convert_gbk_to_utf8(msg);
            std::cout << "receive: " << msg << std::endl;
            std::cout << "data: " << data << std::endl;
            _client->sendMessage(msg);
        }
    }

    delete _client;

    return 0;
}
