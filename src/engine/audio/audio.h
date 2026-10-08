#pragma once

#include <memory>
#include <string>

// Sound playback with miniaudio. Files are relative to sounds/. With no audio device, every call does nothing
// and the game runs silent.
class Audio {
public:
    Audio();
    ~Audio();

    // Plays a file once. The volume applies to every play of that file. A play of the same file less than
    // minGap seconds after the last one is skipped, so many hits in one frame do not make many copies.
    void play(const std::string& file, float volume = 1.0f, float minGap = 0.0f);

    // The volume of all sounds, from 0 (silent) to 1.
    void setVolume(float volume);

    Audio(const Audio&) = delete;
    Audio& operator=(const Audio&) = delete;

private:
    struct Impl; // keeps miniaudio.h (and windows.h) out of this header
    std::unique_ptr<Impl> impl;
};
