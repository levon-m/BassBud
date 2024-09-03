#pragma once

#include <JuceHeader.h>
#include "YinPitchDetector.h"

class DefaultAudioProcessor  : public juce::AudioProcessor
{
public:
    DefaultAudioProcessor();
    ~DefaultAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    float getCurrentPitch() const { return currentPitch; }
    int getCurrentString() const { return currentString; }
    int getCurrentFret() const { return currentFret; }
    juce::String getCurrentNote() const { return currentNote; }

private:
    std::unique_ptr<YinPitchDetector> pitchDetector;
    float currentPitch;
    int currentString;
    int currentFret;
    juce::String currentNote;

    static const int largerBufferSize = 4096;
    std::vector<float> ringBuffer;
    int ringBufferPos;
    int samplesPerBlock;

    float smoothedPitch;
    int stableFrameCount;
    static const int requiredStableFrames = 3;

    void updateCurrentNote(float pitch);
    juce::String frequencyToNoteName(float frequency);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DefaultAudioProcessor)
};