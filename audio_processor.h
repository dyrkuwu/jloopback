#pragma once
#include <cstddef>
#include <memory>
#include <vector>

class c_audio_processor {
public:
    c_audio_processor();
    ~c_audio_processor();

    c_audio_processor(const c_audio_processor&) = delete;
    c_audio_processor& operator=(const c_audio_processor&) = delete;
    c_audio_processor(c_audio_processor&&) = delete;
    c_audio_processor& operator=(c_audio_processor&&) = delete;

    bool update();
    const std::vector<float>& get_bands() const;

private:
    class impl;
    std::unique_ptr<impl> p_impl;
};
