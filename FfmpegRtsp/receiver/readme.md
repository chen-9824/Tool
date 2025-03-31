通过ffmpeg获取rtsp流图像，可选择通过opencv / opengl显示

可获取最新帧数据，获取的最新帧数据格式为构造函数指定

目前最新帧数据会额外复制一次，后续如果需要降低获取最新帧数据的延迟，可以考虑减少一次复制，直接传回frame_bgr

```
                        if (_player_type == player_type::none)
                        {
                            std::lock_guard<std::mutex> lock(frameQueueMutex);
                            av_frame_copy(latest_frame, frame_bgr);
                            frameQueueCond.notify_one();
                        }

```

> ```
> void RTSPStream::get_latest_frame(AVFrame &frame)
> {
>     std::unique_lock<std::mutex> lock(frameQueueMutex);
>     frameQueueCond.wait(lock, [this]()
>                         { return !latest_frame; });
>     av_frame_copy(&frame, latest_frame);
> }
> ```
