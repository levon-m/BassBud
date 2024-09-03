#include "PluginProcessor.h"
#include "PluginEditor.h"

DefaultAudioProcessor::DefaultAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)  // Configuring stereo input
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)  // Configuring stereo output
                     #endif
                       )
#endif
    , currentPitch(0.0f), currentString(-1), currentFret(-1), currentNote("---"),
      smoothedPitch(0.0f), stableFrameCount(0)  // Initialize pitch detection and note-related variables
{
}

DefaultAudioProcessor::~DefaultAudioProcessor()
{
}

const juce::String DefaultAudioProcessor::getName() const
{
    return JucePlugin_Name;  // Defined in the project settings
}

bool DefaultAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool DefaultAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool DefaultAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

/**
 * Returns the tail length in seconds, which is 0 for this processor.
 * This typically indicates how long the effect continues to process after the input stops.
 */
double DefaultAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;  // No tail processing
}

/**
 * Returns the number of programs (presets) available in the processor.
 * For this processor, there is only one program.
 */
int DefaultAudioProcessor::getNumPrograms()
{
    return 1;  // Only one program/preset
}

int DefaultAudioProcessor::getCurrentProgram()
{
    return 0;  // Only one program, so always return 0
}

void DefaultAudioProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);  // No implementation needed
}

const juce::String DefaultAudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);  // No program names to return
    return {};
}

void DefaultAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);  // No implementation needed
}

/**
 * Prepares the processor to play audio by initializing the pitch detector.
 * This is called when playback starts or the audio configuration changes.
 */
void DefaultAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Use a larger buffer size for better low-frequency detection
    int pitchDetectorBufferSize = std::max(1024, samplesPerBlock);
    pitchDetector = std::make_unique<YinPitchDetector>(static_cast<float>(sampleRate), pitchDetectorBufferSize);
    smoothedPitch = 0.0f;  // Reset smoothed pitch
    stableFrameCount = 0;  // Reset stable frame count
}

void DefaultAudioProcessor::releaseResources()
{
    // No resources to release in this implementation
}

// Channel Configurations
#ifndef JucePlugin_PreferredChannelConfigurations
bool DefaultAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;  // MIDI effects don't require audio buses
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;  // Only mono and stereo output are supported

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;  // Input and output layouts must match
   #endif

    return true;  // Layout is supported
  #endif
}
#endif

/**
 * Processes the audio block by detecting pitch and updating the current note.
 * This is the main audio processing function where the plugin's DSP code runs.
 */
void DefaultAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;  // Prevents denormals from affecting performance
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any output channels that are not being used by the input
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // If there is at least one input channel, process the pitch detection
    if (totalNumInputChannels > 0)
    {
        auto* channelData = buffer.getReadPointer(0);  // Get the data from the first input channel
        float detectedPitch = pitchDetector->detectPitch(channelData);  // Detect the pitch from the input signal

        // Check if the detected pitch is within the valid range for a bass guitar
        if (detectedPitch >= 40.0f && detectedPitch <= 400.0f)
        {
            // Smooth the pitch detection to avoid jumps and update if stable
            if (std::abs(detectedPitch - smoothedPitch) < 3.0f || smoothedPitch == 0.0f)
            {
                smoothedPitch = 0.7f * smoothedPitch + 0.3f * detectedPitch;  // Apply a smoothing filter
                stableFrameCount++;
                if (stableFrameCount >= requiredStableFrames)
                {
                    currentPitch = smoothedPitch;  // Update the current pitch if it has been stable
                    updateCurrentNote(currentPitch);  // Update the current note based on the pitch
                }
            }
            else
            {
                stableFrameCount = 0;  // Reset the stability counter if the pitch is unstable
                smoothedPitch = detectedPitch;  // Update the smoothed pitch immediately
            }
        }
        else
        {
            stableFrameCount = 0;  // Reset the stability counter if the pitch is out of range
            if (smoothedPitch > 0.0f)
            {
                smoothedPitch *= 0.9f;  // Apply a slow decay to the smoothed pitch
                if (smoothedPitch < 30.0f)
                {
                    smoothedPitch = 0.0f;  // Reset the pitch if it decays too low
                    currentPitch = 0.0f;  // Clear the current pitch
                    currentString = -1;  // Reset string index
                    currentFret = -1;  // Reset fret index
                    currentNote = "---";  // Reset note display
                }
            }
        }
    }

    // Pass the audio through unchanged
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        juce::ignoreUnused(channelData);  // Unused in this implementation, but necessary to avoid compiler warnings
    }
}

/**
 * Updates the current note, string, and fret based on the detected pitch.
 */
void DefaultAudioProcessor::updateCurrentNote(float pitch)
{
    const float openStringFrequencies[] = {41.20f, 55.00f, 73.42f, 98.00f};  // Frequencies of the open strings (EADG) on a bass guitar
    const int numStrings = 4;  // Number of strings on the bass guitar

    float minDifference = std::numeric_limits<float>::max();  // Initialize minimum difference to a large value
    int closestString = -1;  // Initialize the closest string index

    // Find the string with the closest pitch to the detected pitch
    for (int i = 0; i < numStrings; ++i)
    {
        float difference = std::abs(std::log2(pitch / openStringFrequencies[i]));
        if (difference < minDifference)
        {
            minDifference = difference;
            closestString = i;  // Update the closest string index
        }
    }

    currentString = closestString;  // Update the current string

    // Calculate the fret number and note name based on the closest string
    if (currentString != -1)
    {
        float semitones = 12 * std::log2(pitch / openStringFrequencies[currentString]);  // Calculate the number of semitones from the open string
        currentFret = std::round(semitones);  // Round to the nearest fret
        if (currentFret < 0) currentFret = 0;  // Ensure the fret number is non-negative
        currentNote = frequencyToNoteName(pitch);  // Convert the pitch to a note name
    }
    else
    {
        currentFret = -1;  // Reset the fret number if no string is found
        currentNote = "---";  // Reset the note name
    }
}

/**
 * Converts a frequency to a musical note name.
 */
juce::String DefaultAudioProcessor::frequencyToNoteName(float frequency)
{
    static const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };  // Note names in an octave

    float referenceFrequency = 440.0f;  // Frequency of A4 (the reference note)
    int semitonesFromA4 = std::round(12.0f * std::log2(frequency / referenceFrequency));  // Calculate the number of semitones from A4

    int noteIndex = (semitonesFromA4 + 9) % 12;  // Calculate the index of the note within the octave (0 = C, 11 = B)
    if (noteIndex < 0) noteIndex += 12;  // Ensure the note index is non-negative

    int octave = 4 + (semitonesFromA4 + 9) / 12;  // Calculate the octave number

    return juce::String(noteNames[noteIndex]) + juce::String(octave);  // Return the note name with the octave
}

/**
 * Indicates whether the processor has an editor component.
 * This processor does have an editor, so this method returns true.
 */
bool DefaultAudioProcessor::hasEditor() const
{
    return true;  // This processor has an editor
}

/**
 * Creates and returns a new editor component for the processor.
 */
juce::AudioProcessorEditor* DefaultAudioProcessor::createEditor()
{
    return new DefaultAudioProcessorEditor(*this);  // Create and return the editor
}

/**
 * Saves the current state of the processor to the given memory block.
 * This is used to save plugin settings in the DAW project.
 */
void DefaultAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::ignoreUnused(destData);  // No state information to save in this implementation
}

/**
 * Restores the processor state from the given data.
 * This is used to restore plugin settings when a DAW project is loaded.
 */
void DefaultAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    juce::ignoreUnused(data, sizeInBytes);  // No state information to restore in this implementation
}

/**
 * Factory function that creates and returns a new instance of the plugin.
 * This is the entry point for the plugin when it is loaded by the host.
 */
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DefaultAudioProcessor();  // Create and return the processor
}
