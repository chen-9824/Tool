#include <iostream>

#include "AudioPlayer.h"

int main(int, char **)
{
    std::cout << "Hello, from AudioPlayer!\n";

    AudioPlayer test_player;
    test_player.load("phone_call.wav");
    test_player.playSync();
}
