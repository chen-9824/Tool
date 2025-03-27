#include <iostream>

#include "RTSPStream.h"
int main()
{
    std::string rtsp_url = "rtsp://192.168.51.166:5554/user=admin&password=&channel=1&stream=0.sdp?";
    // std::string rtsp_url = "rtsp://192.168.147.128:8554/test";
    RTSPStream stream(rtsp_url, 640, 640, AV_PIX_FMT_BGR24);
    stream.start();

    std::cout << "Press Enter to stop..." << std::endl;
    std::cin.get();

    stream.stop();
    return 0;
}