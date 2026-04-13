#include "audio_processor.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <mutex>
#include <stdexcept>
#include <vector>

#define MEOW_FFT_IMPLEMENTATION
#include "meow_fft.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

namespace
{
constexpr int fft_size = 1024;
constexpr int max_buffered_samples = fft_size * 4;
constexpr float pi = 3.14159265358979323846f;
} // namespace

class c_audio_processor::impl {
public:
    impl()
        : bands(6, 0.0f)
    {
        if (ma_context_init(nullptr, 0, nullptr, &context) != MA_SUCCESS) {
            throw std::runtime_error("failed to initialize miniaudio context");
        }
        context_initialized = true;

        ma_device_config config = ma_device_config_init(ma_device_type_loopback);
        config.capture.format = ma_format_f32;
        config.capture.channels = 1;
        config.sampleRate = 48000;
        config.dataCallback = data_callback;
        config.pUserData = this;

        if (ma_device_init(&context, &config, &device) != MA_SUCCESS) {
            throw std::runtime_error("failed to initialize loopback device");
        }
        device_initialized = true;

        fft_workset_bytes = meow_fft_generate_workset_real(fft_size, nullptr);
        fft_workset = static_cast<Meow_FFT_Workset_Real*>(std::malloc(fft_workset_bytes));
        if (fft_workset == nullptr) {
            throw std::runtime_error("failed to allocate meow_fft workset");
        }

        meow_fft_generate_workset_real(fft_size, fft_workset);

        audio_buffer.reserve(max_buffered_samples);
        windowed_samples.resize(fft_size);
        fft_out.resize((fft_size + 1) / 2);

        if (ma_device_start(&device) != MA_SUCCESS) {
            throw std::runtime_error("failed to start loopback device");
        }
    }

    ~impl()
    {
        if (device_initialized) {
            ma_device_uninit(&device);
        }

        if (context_initialized) {
            ma_context_uninit(&context);
        }

        if (fft_workset != nullptr) {
            std::free(fft_workset);
            fft_workset = nullptr;
        }
    }

    bool update()
    {
        std::lock_guard<std::mutex> lock(audio_mutex);

        if (audio_buffer.size() < fft_size) {
            return false;
        }

        std::copy(
            audio_buffer.end() - fft_size,
            audio_buffer.end(),
            windowed_samples.begin()
        );

        for (int i = 0; i < fft_size; ++i) {
            const float window =
                0.5f * (1.0f - std::cos((2.0f * pi * static_cast<float>(i)) / static_cast<float>(fft_size - 1)));
            windowed_samples[i] *= window;
        }

        process_fft(windowed_samples.data(), windowed_samples.size());
        return true;
    }

    const std::vector<float>& get_bands() const
    {
        return bands;
    }

private:
    static void data_callback(ma_device* device, void* output, const void* input, ma_uint32 frame_count)
    {
        (void)output;

        if (device == nullptr || device->pUserData == nullptr || input == nullptr) {
            return;
        }

        auto* self = static_cast<impl*>(device->pUserData);
        self->handle_audio_input(static_cast<const float*>(input), frame_count);
    }

    void handle_audio_input(const float* input, ma_uint32 frame_count)
    {
        std::lock_guard<std::mutex> lock(audio_mutex);

        audio_buffer.insert(audio_buffer.end(), input, input + frame_count);

        if (static_cast<int>(audio_buffer.size()) > max_buffered_samples) {
            const auto erase_count = audio_buffer.size() - max_buffered_samples;
            audio_buffer.erase(audio_buffer.begin(), audio_buffer.begin() + erase_count);
        }
    }

    void process_fft(const float* samples, size_t count)
    {
        if (samples == nullptr || count != fft_size || fft_workset == nullptr) {
            return;
        }

        meow_fft_real(fft_workset, samples, fft_out.data());

        std::vector<float> magnitudes(count / 2, 0.0f);
        magnitudes[0] = std::fabs(fft_out[0].r);

        for (size_t i = 1; i < count / 2; ++i) {
            const float real = fft_out[i].r;
            const float imag = fft_out[i].j;
            magnitudes[i] = std::sqrt(real * real + imag * imag);
        }

        static constexpr std::array<std::array<int, 2>, 6> bands_map{{
            {{  1,   4}},
            {{  5,  15}},
            {{ 16,  40}},
            {{ 41, 100}},
            {{101, 200}},
            {{201, 400}}
        }};

        for (size_t band_index = 0; band_index < bands.size(); ++band_index) {
            const int from = bands_map[band_index][0];
            const int to = bands_map[band_index][1];

            float sum = 0.0f;
            int used_bins = 0;

            for (int bin = from; bin <= to && bin < static_cast<int>(magnitudes.size()); ++bin) {
                sum += magnitudes[bin];
                ++used_bins;
            }

            bands[band_index] = (used_bins > 0) ? (sum / static_cast<float>(used_bins)) : 0.0f;
        }
    }

    ma_context context{};
    ma_device device{};
    bool context_initialized = false;
    bool device_initialized = false;
    std::vector<float> audio_buffer;
    std::vector<float> windowed_samples;
    std::mutex audio_mutex;
    Meow_FFT_Workset_Real* fft_workset = nullptr;
    std::size_t fft_workset_bytes = 0;
    std::vector<Meow_FFT_Complex> fft_out;
    std::vector<float> bands;
};

c_audio_processor::c_audio_processor()
    : p_impl(std::make_unique<impl>())
{
}

c_audio_processor::~c_audio_processor() = default;

bool c_audio_processor::update()
{
    return p_impl->update();
}

const std::vector<float>& c_audio_processor::get_bands() const
{
    return p_impl->get_bands();
}
