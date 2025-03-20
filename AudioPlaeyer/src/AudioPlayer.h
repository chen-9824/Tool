//
// Created by cfan on 2025/2/12.
//

#ifndef LVGL_APP_SRC_CUSTOMIZE_AUDIOPLAYER_H_
#define LVGL_APP_SRC_CUSTOMIZE_AUDIOPLAYER_H_
#include <SDL.h>
#include <string>
#include <thread>
#include <atomic>
#include <iostream>
class AudioPlayer {
 public:
  AudioPlayer();
  virtual ~AudioPlayer();

  bool load(const std::string& path);
  void playAsync(bool loop_mode = false);
  void playSync(bool loop_mode = false);
  void stop();
  void setLoop(bool enable) { loop = enable; }

 private:
  static void audio_callback(void* userdata, Uint8* stream, int len);
  void reset();

  SDL_AudioSpec spec;
  Uint8* wav_buffer = nullptr;
  Uint32 wav_length = 0;

  Uint8* audio_data = nullptr;
  Uint8* audio_pos = nullptr;
  Uint32 audio_len = 0;

  SDL_AudioDeviceID dev;
  std::thread play_thread;

  std::atomic<bool> loop;
  std::atomic<bool> async_mode;
  std::atomic<bool> is_playing;
};

#endif //LVGL_APP_SRC_CUSTOMIZE_AUDIOPLAYER_H_
