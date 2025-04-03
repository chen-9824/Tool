# rknn简单封装

当前运行平台为rk3566 aarch64

## main.cpp 读取一张图片并推理

main.cpp

## rtsp_rknn.cpp 读取rtsp流并推理

目前读取rtsp流解码及转换均使用ffmpeg相关，未能使用rk相关的硬件功能，所以解码及格式转换能力均不强，后续需增加。

相关的模型路径配置在config.ini中，也可配置指定识别对象，若连续多帧识别到则通过tcp客户端发送消息

注意，优先使用config.ini中的配置项，若不存在才使用默认配置项。
