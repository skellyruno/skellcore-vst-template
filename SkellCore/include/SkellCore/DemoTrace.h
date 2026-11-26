#pragma once
// Opt-in offline capture contract. Never call from a realtime audio callback.
// The host runs parameter setters on its permitted thread and captures meters
// after processing. No changes to published parameter IDs or plugin state.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <locale>
#include <stdexcept>
#include <string>
#include <array>

namespace skellcore::demo {
struct BlockLevels {
    double rms = 0, peak = 0;
};
inline BlockLevels measure(const float* audio, int n) {
    if (!audio || n <= 0) throw std::invalid_argument("Empty measurement");
    double sum = 0, peak = 0;
    for (int i = 0; i < n; ++i) {
        const double x = audio[i];
        if (!std::isfinite(x)) throw std::runtime_error("Nonfinite audio");
        sum += x*x; peak = std::max(peak, std::abs(x));
    }
    return {std::sqrt(sum/n),peak};
}
// Portable v1 trace: sample indexes refer to the original dry/processed clock.
// A meter callback must supply actual measured values, never a guessed curve.
// The ten-band schema is a LiveMultibandDucker adapter, not a generic FFT.
class Trace {
public:
    Trace(const std::string& events, const std::string& meters, std::int64_t frames)
        : events_(events), meters_(meters), total_(frames) {
        if (!events_ || !meters_ || total_ <= 0) throw std::runtime_error("Cannot create trace");
        events_.imbue(std::locale::classic()); meters_.imbue(std::locale::classic());
        events_ << "sample,parameter,normalized\n" << std::setprecision(9);
        meters_ << "sample,frames,input_rms,output_rms,input_peak,output_peak";
        for(int b=0;b<10;++b) meters_ << ",cut" << b;
        meters_ << '\n' << std::setprecision(9);
    }
    // This logs the accepted parameter command. A JUCE host adapter should
    // obtain the actual parameter readback before calling it.
    void parameter(std::int64_t sample, const std::string& id, double normalized) {
        if (sample != next_ || sample >= total_ || id.empty() || id.size()>64 ||
            id.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.-") != std::string::npos ||
            !std::isfinite(normalized) || normalized<0 || normalized>1)
            throw std::invalid_argument("Invalid parameter event");
        events_ << sample << ',' << id << ',' << normalized << '\n';
        if(!events_) throw std::runtime_error("Parameter trace write failed");
    }
    void block(std::int64_t sample, int n, const float* dry, const float* wet,
               const std::array<float,10>& cutDb) {
        if (sample != next_ || n <= 0 || n > total_-next_) throw std::invalid_argument("Noncontiguous trace");
        const auto a=measure(dry,n), b=measure(wet,n);
        for(auto cut:cutDb) if(!std::isfinite(cut)||cut<0||cut>40) throw std::invalid_argument("Invalid band meter");
        meters_ << sample << ',' << n << ',' << a.rms << ',' << b.rms << ',' << a.peak << ',' << b.peak;
        for(auto cut:cutDb) meters_ << ',' << cut;
        meters_ << '\n';
        if(!meters_) throw std::runtime_error("Meter trace write failed");
        next_+=n;
    }
    void finish() {
        if(next_ != total_) throw std::runtime_error("Incomplete trace");
        events_.flush(); meters_.flush();
        if(!events_ || !meters_) throw std::runtime_error("Trace flush failed");
    }
private:
    std::ofstream events_, meters_;
    std::int64_t total_, next_=0;
};
}
