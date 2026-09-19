#include "AudioRecorder.h"

#include <algorithm>
#include <cstring>

#include <juce_audio_formats/juce_audio_formats.h>

namespace daw
{
AudioRecorder::AudioRecorder() = default;

AudioRecorder::~AudioRecorder()
{
    stopRecording();
}

juce::String AudioRecorder::startRecording (const juce::File& file, double sampleRateToUse, int numChannelsToUse)
{
    stopRecording();

    if (sampleRateToUse <= 0.0 || numChannelsToUse <= 0 || numChannelsToUse > maxChannels)
        return "Invalid recording configuration";

    outputStream = file.createOutputStream();

    if (outputStream == nullptr)
        return "Could not create file: " + file.getFullPathName();

    juce::WavAudioFormat wavFormat;

    const auto options = juce::AudioFormatWriter::Options{}
                             .withSampleRate (sampleRateToUse)
                             .withNumChannels (numChannelsToUse)
                             .withBitsPerSample (32)
                             .withQualityOptionIndex (0);

    writer = wavFormat.createWriterFor (outputStream, options);

    if (writer == nullptr)
    {
        outputStream.reset();
        return "Could not create WAV writer for: " + file.getFullPathName();
    }

    sampleRate = sampleRateToUse;
    numChannels = numChannelsToUse;

    slotStorage.assign (static_cast<size_t> (queueCapacity),
                        std::vector<float> (static_cast<size_t> (maxBlockFrames * numChannels), 0.0f));
    framesPerSlot.assign (static_cast<size_t> (queueCapacity), 0);

    writeIndex.store (0, std::memory_order_relaxed);
    readIndex.store (0, std::memory_order_relaxed);
    droppedBlocks.store (0, std::memory_order_relaxed);
    framesWritten.store (0, std::memory_order_relaxed);

    recording.store (true, std::memory_order_release);

    writerThread = std::make_unique<WriterThread> (*this);
    writerThread->startThread();

    return {};
}

void AudioRecorder::stopRecording()
{
    if (writerThread != nullptr && writerThread->isThreadRunning())
    {
        recording.store (false, std::memory_order_release);

        while (writeIndex.load (std::memory_order_acquire) != readIndex.load (std::memory_order_acquire))
            juce::Thread::sleep (1);

        writerThread->stopThread (10000);
    }

    writerThread.reset();

    if (writer != nullptr)
    {
        writer->flush();
        writer.reset();
    }

    outputStream.reset();

    recording.store (false, std::memory_order_release);
}

void AudioRecorder::recordBlock (const float* const* channels, int numChannelsIn, int numSamples) noexcept
{
    if (!recording.load (std::memory_order_relaxed))
        return;

    if (numChannelsIn > numChannels)
        numChannelsIn = numChannels;

    const int writePos = writeIndex.load (std::memory_order_relaxed);
    const int nextPos = (writePos + 1) % queueCapacity;

    if (nextPos == readIndex.load (std::memory_order_acquire))
    {
        droppedBlocks.fetch_add (1, std::memory_order_relaxed);
        return;
    }

    if (numSamples > maxBlockFrames)
        numSamples = maxBlockFrames;

    auto& slot = slotStorage[static_cast<size_t> (writePos)];

    for (int channel = 0; channel < numChannelsIn; ++channel)
        if (channels[channel] != nullptr)
            std::memcpy (slot.data() + static_cast<size_t> (channel * maxBlockFrames),
                         channels[channel],
                         static_cast<size_t> (numSamples) * sizeof (float));

    framesPerSlot[static_cast<size_t> (writePos)] = numSamples;

    writeIndex.store (nextPos, std::memory_order_release);
}

void AudioRecorder::WriterThread::run()
{
    while (!threadShouldExit())
    {
        owner.writeAvailableSlots();
        juce::Thread::sleep (1);
    }

    owner.writeAvailableSlots();
}

void AudioRecorder::writeAvailableSlots()
{
    const float* channelPointers[maxChannels] = {};

    while (writer != nullptr)
    {
        const int writePos = writeIndex.load (std::memory_order_acquire);
        const int readPos = readIndex.load (std::memory_order_relaxed);

        if (readPos == writePos)
            return;

        const auto& slot = slotStorage[static_cast<size_t> (readPos)];
        const int frames = framesPerSlot[static_cast<size_t> (readPos)];

        for (int channel = 0; channel < numChannels; ++channel)
            channelPointers[channel] = slot.data() + static_cast<size_t> (channel * maxBlockFrames);

        if (writer->writeFromFloatArrays (channelPointers, numChannels, frames))
            framesWritten.fetch_add (static_cast<std::uint64_t> (frames), std::memory_order_relaxed);

        readIndex.store ((readPos + 1) % queueCapacity, std::memory_order_release);
    }
}
} // namespace daw
