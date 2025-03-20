//
// Created by cfan on 2025/2/12.
//

#include "AudioPlayer.h"
#include "spdlog/spdlog.h"
AudioPlayer::AudioPlayer() : audio_pos(0), audio_len(0), dev(0), loop(false), async_mode(false), is_playing(false) {
    SDL_memset(&spec, 0, sizeof(SDL_AudioSpec));
}
AudioPlayer::~AudioPlayer() {
    stop();
    if (wav_buffer) {
        SDL_FreeWAV(wav_buffer);
        //wav_buffer = nullptr;
    }
    // 目前未进行格式转换，所以两个指针实际指向同一块空间，避免重复释放
    if (audio_data && audio_data != wav_buffer) {
        free(audio_data);
        audio_data = nullptr;
    }
}
bool AudioPlayer::load(const std::string &path) {
    SDL_AudioSpec file_spec;
    if (SDL_LoadWAV(path.c_str(), &file_spec, &wav_buffer, &wav_length) == NULL) {
        std::cerr << "Error loading WAV file: " << SDL_GetError() << std::endl;
        return false;
    }

    /*SDL_AudioCVT cvt;
    SDL_BuildAudioCVT(&cvt, file_spec.format, file_spec.channels, file_spec.freq,
                      AUDIO_S16SYS, 2, 44100);
    cvt.buf = (Uint8*)malloc(wav_length * cvt.len_mult);
    cvt.len = wav_length;
    memcpy(cvt.buf, wav_buffer, wav_length);
    SDL_ConvertAudio(&cvt);

    audio_data = cvt.buf;
    audio_len = cvt.len_cvt;
    audio_pos = audio_data;

    spec = file_spec;
    spec.callback = audio_callback;
    spec.userdata = this;*/

    // **不进行格式转换，直接使用原始格式**
    spec = file_spec;
    spec.callback = audio_callback;
    spec.userdata = this;

    // 设置音频数据指针
    audio_data = wav_buffer;
    audio_len = wav_length;
    audio_pos = audio_data;


    return true;
}
void AudioPlayer::playAsync(bool loop_mode) {
    if (is_playing) return;

    loop = loop_mode;
    //async_mode = true;
    is_playing = true;

    // 使用一个原子标志来确保线程安全
    if (play_thread.joinable()) {
        play_thread.join();  // 确保上一个线程已完全退出
    }

    play_thread = std::thread([this]() {
      spdlog::debug("playAsync thread start...");
      dev = SDL_OpenAudioDevice(NULL, 0, &spec, NULL, 0);
      spdlog::debug("playAsync thread start...");
      if (dev == 0) {
          std::cerr << "SDL_OpenAudioDevice failed: " << SDL_GetError() << std::endl;
          is_playing = false;
          //async_mode = false;
          return;
      }
      SDL_PauseAudioDevice(dev, 0);

      while (is_playing) {
          SDL_Delay(100);
      }

      // 播放结束后关闭音频设备
      SDL_PauseAudioDevice(dev, 1);
      SDL_CloseAudioDevice(dev);
      dev = 0;
      reset();
      spdlog::debug("playAsync thread exit...");
    });
}
void AudioPlayer::playSync(bool loop_mode) {
    if (is_playing) return;

    loop = loop_mode;
    //async_mode = false;
    is_playing = true;

    dev = SDL_OpenAudioDevice(NULL, 0, &spec, NULL, 0);
    if (dev == 0) {
        std::cerr << "SDL_OpenAudioDevice failed: " << SDL_GetError() << std::endl;
        return;
    }
    SDL_PauseAudioDevice(dev, 0);

    while (is_playing) {
        SDL_Delay(100);
    }

    // 播放结束后关闭音频设备
    SDL_CloseAudioDevice(dev);
    dev = 0;
    reset();
}
void AudioPlayer::stop() {
    is_playing = false;
    if (dev) {
        SDL_PauseAudioDevice(dev, 1);
        SDL_CloseAudioDevice(dev);
        dev = 0;
    }
    if (play_thread.joinable()) play_thread.join();
    reset();
}
void AudioPlayer::audio_callback(void *userdata, Uint8 *stream, int len) {
    AudioPlayer* self = static_cast<AudioPlayer*>(userdata);

    //if (!self->is_playing) return;  // 确保播放未停止，防止 SDL 访问无效数据

    SDL_memset(stream, 0, len);

    if (self->audio_len == 0){
        self->is_playing = false;
        return;
    }

    len = (len < self->audio_len) ? len : self->audio_len;
    //SDL_MixAudioFormat(stream, self->audio_pos, AUDIO_S16SYS, len, SDL_MIX_MAXVOLUME);
    SDL_memcpy(stream, self->audio_pos, len);
    self->audio_pos += len;
    self->audio_len -= len;

    if (self->audio_len <= 0 && self->loop) {
        self->reset();
    }
}
void AudioPlayer::reset() {
    audio_pos = audio_data;
    audio_len = wav_length;
}
