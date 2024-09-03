#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class DefaultAudioProcessorEditor  : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    DefaultAudioProcessorEditor (DefaultAudioProcessor&);
    ~DefaultAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    DefaultAudioProcessor& audioProcessor;

    juce::Label titleLabel;
    juce::ComboBox scaleModeSelector;
    juce::Label modeSelectionLabel;
    juce::Label liveFeedbackLabel;
    
    static const int numStrings = 4;
    static const int numFrets = 7;
    
    void drawFretboard(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawString(juce::Graphics& g, juce::Rectangle<int> bounds, int stringIndex);
    void drawFret(juce::Graphics& g, juce::Rectangle<int> bounds, int fretIndex);
    void drawFretMarker(juce::Graphics& g, juce::Rectangle<int> bounds, int fretIndex);
    void drawNotePlaceholder(juce::Graphics& g, juce::Rectangle<int> bounds, int stringIndex, int fretIndex, bool isRoot, bool isInMode);
    void drawDebugInfo(juce::Graphics& g, juce::Rectangle<int> bounds);

    bool isNoteMatch(const juce::String& currentNote, const juce::String& compareNote);
    bool isNoteInMode(const juce::String& note, const juce::String& root, int modeIndex);
    juce::String getOpenStringNote(int stringIndex);
    juce::String getNoteAtPosition(int stringIndex, int fretIndex);

    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DefaultAudioProcessorEditor)
};