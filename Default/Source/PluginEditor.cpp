#include "PluginProcessor.h"
#include "PluginEditor.h"

DefaultAudioProcessorEditor::DefaultAudioProcessorEditor(DefaultAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)  // Initializing the base class and storing reference to the processor
{
    // Set the size of the plugin editor window (width: 860, height: 330)
    setSize(860, 330);  // Increased height to accommodate title bar

    // Add and configure the title label
    addAndMakeVisible(titleLabel);
    titleLabel.setText("BassBud", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(24.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    // Add and configure the scale mode selector (drop-down menu)
    addAndMakeVisible(scaleModeSelector);
    scaleModeSelector.addItem("Ionian (Major)", 1);
    scaleModeSelector.addItem("Dorian", 2);
    scaleModeSelector.addItem("Phrygian", 3);
    scaleModeSelector.addItem("Lydian", 4);
    scaleModeSelector.addItem("Mixolydian", 5);
    scaleModeSelector.addItem("Aeolian (Natural Minor)", 6);
    scaleModeSelector.addItem("Locrian", 7);
    scaleModeSelector.setSelectedItemIndex(0);  // Default selection to "Ionian (Major)"
    scaleModeSelector.setJustificationType(juce::Justification::centred);
    scaleModeSelector.onChange = [this] { repaint(); };  // Repaint when selection changes

    // Add and configure the mode selection label
    addAndMakeVisible(modeSelectionLabel);
    modeSelectionLabel.setText("Mode Selection", juce::dontSendNotification);
    modeSelectionLabel.setJustificationType(juce::Justification::centred);

    // Add and configure the live feedback label
    addAndMakeVisible(liveFeedbackLabel);
    liveFeedbackLabel.setText("Live Feedback", juce::dontSendNotification);
    liveFeedbackLabel.setJustificationType(juce::Justification::centred);

    // Start a timer with an interval of 50 milliseconds to periodically call timerCallback()
    startTimer(50);
}

DefaultAudioProcessorEditor::~DefaultAudioProcessorEditor()
{
    stopTimer();  // Stop the timer to prevent further callbacks
}

void DefaultAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF275A8A));  // Fill the background with a specific color

    auto bounds = getLocalBounds().reduced(20);  // Define the drawing area with some padding

    // Draw the title bar at the top
    auto titleBounds = bounds.removeFromTop(40);
    g.setColour(juce::Colour(0xFF1E4A6D));  // Set color for the title bar background
    g.fillRect(titleBounds);
    titleLabel.setBounds(titleBounds);  // Set the title label's bounds to match the title bar

    bounds.removeFromTop(10);  // Add some vertical space between the title and the next section

    // Define the area for the mode selection drop-down menu and label
    auto modeSelectionBounds = bounds.removeFromTop(50);
    modeSelectionLabel.setBounds(modeSelectionBounds.removeFromTop(20));  // Set label bounds

    // Add a shadow effect to the drop-down menu
    juce::DropShadow dropShadow(juce::Colours::black.withAlpha(0.5f), 5, juce::Point<int>(0, 2));
    dropShadow.drawForRectangle(g, modeSelectionBounds.reduced(static_cast<int>(modeSelectionBounds.getWidth() * 0.3), 0));
    scaleModeSelector.setBounds(modeSelectionBounds.reduced(static_cast<int>(modeSelectionBounds.getWidth() * 0.3), 0));

    bounds.removeFromTop(10);  // Add vertical space between the drop-down and the next section
    liveFeedbackLabel.setBounds(bounds.removeFromTop(20));  // Set bounds for the live feedback label
    bounds.removeFromTop(10);  // Add more vertical space

    // Define the area for drawing the fretboard
    auto fretboardBounds = bounds.removeFromTop(static_cast<int>(bounds.getHeight() * 0.7));
    int fretboardWidth = static_cast<int>(fretboardBounds.getWidth() * 0.8);
    fretboardBounds = fretboardBounds.withSizeKeepingCentre(fretboardWidth, fretboardBounds.getHeight());
    int extraTopSpace = static_cast<int>(fretboardBounds.getHeight() * 0.1);
    fretboardBounds = fretboardBounds.withTrimmedTop(-extraTopSpace);

    // Add a shadow effect to the fretboard
    juce::DropShadow fretboardShadow(juce::Colours::black.withAlpha(0.5f), 10, juce::Point<int>(5, 5));
    fretboardShadow.drawForRectangle(g, fretboardBounds);

    drawFretboard(g, fretboardBounds);  // Draw the fretboard base

    // Draw the strings on the fretboard
    for (int i = 0; i < numStrings; ++i)
    {
        drawString(g, fretboardBounds, i);
    }

    // Draw the frets on the fretboard
    for (int i = 0; i < numFrets + 1; ++i)
    {
        drawFret(g, fretboardBounds, i);
    }

    // Draw fret markers (dots) on specific frets
    for (int i = 0; i < numFrets; ++i)
    {
        drawFretMarker(g, fretboardBounds, i);
    }

    // Get the current note and the selected mode index from the scale mode selector
    juce::String currentNote = audioProcessor.getCurrentNote();
    int selectedMode = scaleModeSelector.getSelectedItemIndex();

    // First pass: Draw root notes (yellow)
    for (int s = 0; s < numStrings; ++s)
    {
        for (int f = 0; f <= numFrets; ++f)
        {
            juce::String noteAtPosition = getNoteAtPosition(s, f);
            bool isRoot = isNoteMatch(currentNote, noteAtPosition);

            if (isRoot)
            {
                drawNotePlaceholder(g, fretboardBounds, s, f, true, false);  // Draw yellow circle for root notes
            }
        }
    }

    // Second pass: Draw mode notes (red) relative to the root notes
    for (int s = 0; s < numStrings; ++s)
    {
        for (int f = 0; f <= numFrets; ++f)
        {
            juce::String noteAtPosition = getNoteAtPosition(s, f);
            bool isRoot = isNoteMatch(currentNote, noteAtPosition);

            if (isRoot)
            {
                // For each root note, draw the corresponding mode notes
                for (int ms = 0; ms < numStrings; ++ms)
                {
                    for (int mf = 0; mf <= numFrets; ++mf)
                    {
                        juce::String modeNoteAtPosition = getNoteAtPosition(ms, mf);
                        bool isInMode = isNoteInMode(modeNoteAtPosition, noteAtPosition, selectedMode);

                        if (isInMode && !isNoteMatch(noteAtPosition, modeNoteAtPosition))
                        {
                            drawNotePlaceholder(g, fretboardBounds, ms, mf, false, true);  // Draw red circle for mode notes
                        }
                    }
                }
            }
        }
    }

    drawDebugInfo(g, bounds);  // Draw debug information on the bottom part of the editor
}

/**
 * Resized method for the editor.
 * This is called when the editor window is resized and is used to layout subcomponents.
 */
void DefaultAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(20);  // Define the drawing area with padding
    bounds.removeFromTop(100);  // Reserve space for the title, mode selection, and live feedback labels
    auto fretboardBounds = bounds.removeFromTop(static_cast<int>(bounds.getHeight() * 0.7));  // Define the fretboard area
    bounds.removeFromTop(10);  // Add some vertical space for the bottom section
}

/**
 * Draws the fretboard base rectangle.
 */
void DefaultAudioProcessorEditor::drawFretboard(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    g.setColour(juce::Colour(0xFF8B4513));  // Set color for the fretboard (wooden texture color)
    g.fillRect(bounds);  // Draw the fretboard base rectangle

    g.setColour(juce::Colours::black);  // Set color for the outline
    g.drawRect(bounds, 4);  // Draw a thicker black outline around the fretboard
}

/**
 * Draws a string on the fretboard.
 */
void DefaultAudioProcessorEditor::drawString(juce::Graphics& g, juce::Rectangle<int> bounds, int stringIndex)
{
    float stringSpacing = bounds.getHeight() / (numStrings + 1);  // Calculate the spacing between strings
    float stringY = bounds.getY() + (stringIndex + 1) * stringSpacing;  // Calculate the Y position of the string
    g.setColour(juce::Colours::silver);  // Set color for the string (silver)
    g.drawLine(static_cast<float>(bounds.getX()), stringY, static_cast<float>(bounds.getRight()), stringY, 1.5f);  // Draw the string as a line
}

/**
 * Draws a fret on the fretboard.
 */
void DefaultAudioProcessorEditor::drawFret(juce::Graphics& g, juce::Rectangle<int> bounds, int fretIndex)
{
    float fretX = bounds.getX() + fretIndex * bounds.getWidth() / static_cast<float>(numFrets);  // Calculate the X position of the fret
    g.setColour(juce::Colours::silver);  // Set color for the fret (silver)
    g.drawLine(fretX, static_cast<float>(bounds.getY()), fretX, static_cast<float>(bounds.getBottom()), 2.0f);  // Draw the fret as a line
}

/**
 * Draws a fret marker (dot) on specific frets.
 */
void DefaultAudioProcessorEditor::drawFretMarker(juce::Graphics& g, juce::Rectangle<int> bounds, int fretIndex)
{
    if (fretIndex == 2 || fretIndex == 4 || fretIndex == 6)  // Only draw markers on the 3rd, 5th, and 7th frets
    {
        float fretX = bounds.getX() + (fretIndex + 0.5f) * bounds.getWidth() / static_cast<float>(numFrets);  // Calculate the X position of the marker
        float markerY = static_cast<float>(bounds.getCentreY());  // Calculate the Y position of the marker (center of the fretboard)
        g.setColour(juce::Colours::ivory);  // Set color for the marker (ivory)
        g.fillEllipse(fretX - 5, markerY - 5, 10, 10);  // Draw the marker as a small circle
    }
}

/**
 * Draws a placeholder for a note on the fretboard.
 */
void DefaultAudioProcessorEditor::drawNotePlaceholder(juce::Graphics& g, juce::Rectangle<int> bounds, int stringIndex, int fretIndex, bool isRoot, bool isInMode)
{
    float fretWidth = static_cast<float>(bounds.getWidth()) / numFrets;  // Calculate the width of a single fret
    float stringSpacing = bounds.getHeight() / (numStrings + 1);  // Calculate the spacing between strings
    float noteX = static_cast<float>(bounds.getX()) + (fretIndex + 0.5f) * fretWidth;  // Calculate the X position of the note
    float noteY = static_cast<float>(bounds.getY()) + (stringIndex + 1) * stringSpacing;  // Calculate the Y position of the note

    if (isRoot)
    {
        g.setColour(juce::Colours::yellow.withAlpha(0.7f));  // Set color to yellow with transparency for root notes
        g.fillEllipse(noteX - 12, noteY - 12, 24, 24);  // Draw the note as a yellow circle
    }
    else if (isInMode)
    {
        g.setColour(juce::Colours::red.withAlpha(0.5f));  // Set color to red with transparency for mode notes
        g.fillEllipse(noteX - 12, noteY - 12, 24, 24);  // Draw the note as a red circle
    }
}

void DefaultAudioProcessorEditor::drawDebugInfo(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    g.setColour(juce::Colours::white);  // Set color for the debug text (white)
    g.setFont(30.0f);  // Set a larger font size for the debug text

    juce::String debugInfo = "Note: " + audioProcessor.getCurrentNote() +
                             "  Frequency: " + juce::String(audioProcessor.getCurrentPitch(), 2) + " Hz";  // Construct the debug string

    g.drawFittedText(debugInfo, bounds.removeFromBottom(30), juce::Justification::centred, 1);  // Draw the debug information at the bottom, centered
}

void DefaultAudioProcessorEditor::timerCallback()
{
    repaint();  // Repaint the editor to reflect any changes
}

/**
 * Checks if two notes are a match (ignoring octaves).
 * This is used to identify root notes.
 */
bool DefaultAudioProcessorEditor::isNoteMatch(const juce::String& currentNote, const juce::String& compareNote)
{
    juce::String currentNoteName = currentNote.substring(0, currentNote.length() - 1);  // Extract the note name (without octave)
    juce::String compareNoteName = compareNote.substring(0, compareNote.length() - 1);  // Extract the note name (without octave)

    return currentNoteName == compareNoteName;  // Return true if the names match
}

/**
 * Checks if a note belongs to the selected mode relative to the root note.
 */
bool DefaultAudioProcessorEditor::isNoteInMode(const juce::String& note, const juce::String& root, int modeIndex)
{
    static const int modeIntervals[7][7] = {
        {0, 2, 4, 5, 7, 9, 11},  // Ionian (Major)
        {0, 2, 3, 5, 7, 9, 10},  // Dorian
        {0, 1, 3, 5, 7, 8, 10},  // Phrygian
        {0, 2, 4, 6, 7, 9, 11},  // Lydian
        {0, 2, 4, 5, 7, 9, 10},  // Mixolydian
        {0, 2, 3, 5, 7, 8, 10},  // Aeolian (Natural Minor)
        {0, 1, 3, 5, 6, 8, 10}   // Locrian
    };  // Mode intervals in semitones relative to the root note

    juce::String rootNoteName = root.substring(0, root.length() - 1);  // Extract root note name
    juce::String noteToCheck = note.substring(0, note.length() - 1);  // Extract note name to check

    static const juce::String noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};  // Note names
    int rootIndex = -1;
    int noteIndex = -1;

    for (int i = 0; i < 12; ++i)
    {
        if (noteNames[i] == rootNoteName) rootIndex = i;  // Find the index of the root note
        if (noteNames[i] == noteToCheck) noteIndex = i;  // Find the index of the note to check
    }

    if (rootIndex == -1 || noteIndex == -1) return false;  // If either note is not found, return false

    int interval = (noteIndex - rootIndex + 12) % 12;  // Calculate the interval in semitones between the root and the note

    for (int i = 0; i < 7; ++i)
    {
        if (modeIntervals[modeIndex][i] == interval) return true;  // Check if the interval matches any interval in the selected mode
    }

    return false;  // Return false if the note is not in the mode
}

/**
 * Returns the note corresponding to an open string on the bass guitar.
 * 
 * @param stringIndex The index of the string (0 = highest string, 3 = lowest string).
 * @return The note name (including octave) for the open string.
 */
juce::String DefaultAudioProcessorEditor::getOpenStringNote(int stringIndex)
{
    const juce::String openStringNotes[] = {"G3", "D3", "A2", "E2"};  // Note names for open strings on a standard-tuned bass guitar
    return openStringNotes[stringIndex];  // Return the note for the given string
}

/**
 * Returns the note at a specific position on the fretboard.
 */
juce::String DefaultAudioProcessorEditor::getNoteAtPosition(int stringIndex, int fretIndex)
{
    const juce::String notes[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};  // All possible note names
    const juce::String openStringNotes[] = {"G", "D", "A", "E"};  // Note names for open strings

    int noteIndex = 0;
    for (int i = 0; i < 12; ++i)
    {
        if (notes[i] == openStringNotes[stringIndex])
        {
            noteIndex = i;  // Find the index of the open string note in the notes array
            break;
        }
    }

    noteIndex = (noteIndex + fretIndex) % 12;  // Calculate the note index at the given fret

    int octave = 2 + (stringIndex >= 2 ? 1 : 0) + (noteIndex + fretIndex) / 12;  // Calculate the octave of the note

    return notes[noteIndex] + juce::String(octave);  // Return the note name with the correct octave
}
