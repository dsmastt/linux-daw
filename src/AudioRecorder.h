#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

namespace juce
{
class AudioFormatWriter;
class OutputStream;
} // namespace juce

namespace daw
{
class AudioRecorder final
{
public:
    AudioRecorder();
    ~AudioRecorder();

    juce::String startRecording (const juce::File& file, double sampleRate, int numChannels);
    void stopRecording();

    bool isRecording() const noexcept { return recording.load(); }
    void recordBlock (const float* const* channels, int numChannels, int numSamples) noexcept;

    std::uint64_t getFramesWritten() const noexcept { return framesWritten.load(); }
    int getDroppedBlocks() const noexcept { return droppedBlocks.load(); }

private:
    static constexpr int queueCapacity = 256;
    static constexpr int maxBlockFrames = 2048;
    static constexpr int maxChannels = 2;

    class WriterThread final : public juce::Thread
    {
    public:
        explicit WriterThread (AudioRecorder& ownerToUse)
            : juce::Thread ("AudioRecorderWriter"), owner (ownerToUse)
        {
        }

        void run() override;

    private:
        AudioRecorder& owner;
    };

    void writeAvailableSlots();

    std::atomic<bool> recording { false };
    std::atomic<int> writeIndex { 0 };
    std::atomic<int> readIndex { 0 };
    std::atomic<int> droppedBlocks { 0 };
    std::atomic<std::uint64_t> framesWritten { 0 };

    std::vector<std::vector<float>> slotStorage;
    std::vector<int> framesPerSlot;

    std::unique_ptr<WriterThread> writerThread;
    std::unique_ptr<juce::OutputStream> outputStream;
    std::unique_ptr<juce::AudioFormatWriter> writer;

    double sampleRate = 0.0;
    int numChannels = 0;
};
} // namespace daw
