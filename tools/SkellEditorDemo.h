#pragma once
// Renderer-only adapter: actual JUCE slider attachment, actual processor, actual snapshots.
// Run on the message thread on a fresh instance. Not realtime or a DAW automation server.
#include <SkellCore/HeadlessUIRenderer.h>
#include <SkellCore/DemoTrace.h>
#include <array>
#include <algorithm>
#include <set>
#include <stdexcept>
#include <string>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <locale>
#include <regex>
#include <sstream>
#include <vector>

namespace skellcore::editor_demo {
inline constexpr int rate = 48000, fps = 24, frameSamples = rate / fps, frameCount = fps * 6;
inline void write16(std::ofstream& f, std::uint16_t v) { f.put(char(v)); f.put(char(v >> 8)); }
inline void write32(std::ofstream& f, std::uint32_t v) { write16(f, v & 65535); write16(f, v >> 16); }
inline void wav(const std::filesystem::path& path, const std::vector<float>& values) {
    std::ofstream f(path, std::ios::binary);
    f.write("RIFF",4); write32(f, static_cast<std::uint32_t>(36 + 2 * values.size())); f.write("WAVEfmt ",8);
    write32(f,16); write16(f,1); write16(f,1); write32(f,rate); write32(f,rate*2); write16(f,2); write16(f,16);
    f.write("data",4); write32(f,static_cast<std::uint32_t>(2 * values.size()));
    for (auto value : values) {
        if (!std::isfinite(value) || std::abs(value) > 1.f) throw std::runtime_error("Clipped or nonfinite demo audio");
        write16(f,static_cast<std::uint16_t>(std::lround(value * 32767.f)));
    }
    if (!f) throw std::runtime_error("Demo WAV write failed");
}
inline int run(SkellAudioProcessor& processor, SkellAudioProcessorEditor& editor,
               const juce::File& directory, const std::string& revision) {
    try {
        if (!juce::MessageManager::getInstance()->isThisTheMessageThread()) throw std::runtime_error("Requires JUCE message thread");
        if (!std::regex_match(revision, std::regex("[a-f0-9]{40}"))) throw std::invalid_argument("Exact source revision required");
        // Do not write into an existing capture or user project directory.
        if (directory.exists() || directory.createDirectory().failed()) throw std::runtime_error("Use a new capture directory");
        const auto root = std::filesystem::u8path(directory.getFullPathName().toStdString());
        juce::Slider* knob = nullptr;
        for (int i = 0; i < editor.getNumChildComponents(); ++i) {
            if (auto* slider = dynamic_cast<juce::Slider*>(editor.getChildComponent(i))) {
                if (knob) throw std::runtime_error("Ambiguous demo slider");
                knob = slider;
            }
        }
        auto* parameter = processor.parameters.getParameter("gain");
        auto* raw = processor.parameters.getRawParameterValue("gain");
        if (!knob || !parameter || !raw) throw std::runtime_error("Gain attachment not found");
        editor.setStudioSkin(skellcore::ui::studioSkinFromIndex(1), false);
        editor.setSize(480,330);
        processor.setRateAndBufferSizeDetails(rate,500);
        processor.prepareToPlay(rate,500);
        juce::AudioBuffer<float> block(2,500);
        juce::MidiBuffer midi;
        std::vector<float> dry(frameCount * frameSamples), wet(dry.size());
        std::ofstream trace(root / "editor.csv");
        trace.imbue(std::locale::classic()); trace << std::setprecision(9);
        trace << "frame,sample,parameter,normalized,actual,input_rms,output_rms,input_peak,output_peak\n";
        std::set<std::uint64_t> distinct;
        constexpr double pi=3.14159265358979323846;
        for (int frame=0; frame<frameCount; ++frame) {
            const double t=double(frame)/fps;
            // Continuous, easing-based changes; every image is rendered from the attached widget.
            const auto ease=[](double x){x=std::clamp(x,0.0,1.0);return x*x*(3.0-2.0*x);};
            const double gain=t<3.0 ? .25 + 1.35*ease((t-.5)/2.0) : 1.6-1.05*ease((t-3.2)/2.0);
            knob->setValue(gain,juce::sendNotificationSync);
            const float accepted=raw->load();
            const float normalized=parameter->getValue();
            if (!std::isfinite(accepted)||std::abs(accepted-gain)>.002||std::abs(normalized-parameter->convertTo0to1(accepted))>.0001f)
                throw std::runtime_error("Slider did not update the actual attached processor parameter");
            const int start=frame*frameSamples;
            for(int offset=0;offset<frameSamples;offset+=500) {
                for(int i=0;i<500;++i) {
                    const int sample=start+offset+i;
                    const double sec=double(sample)/rate;
                    const float x=static_cast<float>(.18*std::sin(2*pi*220*sec)+.1*std::sin(2*pi*440*sec));
                    dry[static_cast<std::size_t>(sample)]=x;
                    for(int ch=0;ch<2;++ch) block.setSample(ch,i,x);
                }
                midi.clear(); processor.processBlock(block,midi);
                for(int i=0;i<500;++i) wet[static_cast<std::size_t>(start+offset+i)]=block.getSample(0,i);
            }
            const auto a=skellcore::demo::measure(dry.data()+start,frameSamples), b=skellcore::demo::measure(wet.data()+start,frameSamples);
            const auto shot=skellcore::ui::headless::capture(editor,480,330);
            std::ostringstream name; name<<"frame_"<<std::setw(6)<<std::setfill('0')<<frame<<".png";
            if (shot.distinctColours<12||!skellcore::ui::headless::writePng(shot.image,directory.getChildFile(juce::String(name.str()))))
                throw std::runtime_error("Actual editor capture failed");
            distinct.insert(shot.fingerprint);
            trace<<frame<<','<<start<<",gain,"<<normalized<<','<<accepted<<','<<a.rms<<','<<b.rms<<','<<a.peak<<','<<b.peak<<'\n';
        }
        processor.releaseResources();
        trace.flush();
        if(!trace||distinct.size()<12) throw std::runtime_error("Capture did not contain real control movement");
        wav(root/"dry.wav",dry); wav(root/"processed.wav",wet);
        std::ofstream meta(root/"capture.json");
        meta<<"{\n\"schema\":\"skell.demo.editor.v1\",\n\"origin\":\"offline_juce_editor\",\n\"product\":\"Skell Quickstart\",\n\"revision\":\""<<revision<<"\",\n\"fps\":"<<fps<<",\n\"frame_count\":"<<frameCount<<",\n\"sample_rate\":"<<rate<<",\n\"samples\":"<<dry.size()<<",\n\"width\":480,\n\"height\":330,\n\"parameter\":\"gain\",\n\"stimulus\":\"synthetic two-tone test\"\n}\n";
        if(!meta) throw std::runtime_error("Manifest write failed");
        std::cout<<"Captured "<<frameCount<<" actual editor frames, "<<distinct.size()<<" distinct control states and "<<dry.size()<<" synchronized samples\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
} // namespace skellcore::editor_demo
