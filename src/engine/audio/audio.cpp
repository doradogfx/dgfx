#include "audio/audio.h"

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include <chrono>
#include <cstdio>
#include <unordered_map>

static double seconds() {
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

struct Audio::Impl {
    struct Sound {
        ma_sound preload;     // never played: it keeps the decoded file in miniaudio's cache
        ma_sound_group group; // the plays of this file go through it, for the volume
        double lastPlayed = -1.0e9;
        bool failed = false;
    };

    ma_engine engine;
    bool ready = false;
    std::unordered_map<std::string, Sound> sounds; // map nodes do not move, so the miniaudio objects stay valid
};

Audio::Audio() : impl(std::make_unique<Impl>()) {
    impl->ready = ma_engine_init(nullptr, &impl->engine) == MA_SUCCESS;

    if (!impl->ready)
        std::fprintf(stderr, "No audio device: the game runs without sound\n");
}

Audio::~Audio() {
    if (!impl->ready)
        return;

    for (auto& [file, sound] : impl->sounds) {
        if (!sound.failed) {
            ma_sound_uninit(&sound.preload);
            ma_sound_group_uninit(&sound.group);
        }
    }

    ma_engine_uninit(&impl->engine);
}

void Audio::play(const std::string& file, float volume, float minGap) {
    if (!impl->ready)
        return;

    auto [it, added] = impl->sounds.try_emplace(file);
    Impl::Sound& sound = it->second;
    const std::string path = SOUND_DIR + file;

    // The first play loads the file. A file that does not load stays silent, with one message.
    if (added) {
        if (ma_sound_group_init(&impl->engine, 0, nullptr, &sound.group) != MA_SUCCESS) {
            sound.failed = true;
        } else if (ma_sound_init_from_file(&impl->engine, path.c_str(), MA_SOUND_FLAG_DECODE, nullptr, nullptr, &sound.preload) != MA_SUCCESS) {
            ma_sound_group_uninit(&sound.group);
            sound.failed = true;
        }

        if (sound.failed) {
            std::fprintf(stderr, "Could not load sound %s\n", path.c_str());
        }
    }

    if (sound.failed)
        return;

    const double now = seconds();

    if (now - sound.lastPlayed < minGap)
        return;

    sound.lastPlayed = now;
    ma_sound_group_set_volume(&sound.group, volume);
    ma_engine_play_sound(&impl->engine, path.c_str(), &sound.group); // miniaudio frees the copy when it ends
}

void Audio::setVolume(float volume) {
    if (impl->ready)
        ma_engine_set_volume(&impl->engine, volume);
}
