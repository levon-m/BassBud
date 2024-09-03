#pragma once
#include <JuceHeader.h>
#include <vector>
#include <cmath>

class YinPitchDetector
{
public:
    YinPitchDetector(float sampleRate, int bufferSize)
        : sampleRate(sampleRate), bufferSize(bufferSize)
    {
        // Initialize the yinBuffer with the buffer size.
        yinBuffer.resize(bufferSize);
        prevSample = 0.0f;  // Initialize previous sample for the low-pass filter.
    }

    float detectPitch(const float* buffer)
    {
        // Apply a low-pass filter to the input buffer to reduce high-frequency noise.
        std::vector<float> filteredBuffer(bufferSize);
        for (int i = 0; i < bufferSize; ++i)
        {
            // Low-pass filter formula: filtered[i] = 0.95 * previous filtered sample + 0.05 * current sample
            filteredBuffer[i] = 0.95f * prevSample + 0.05f * buffer[i];
            prevSample = filteredBuffer[i];  // Update the previous sample for the next iteration.
        }

        int tauEstimate = -1;  // Estimate of the period (in samples).
        float pitchInHz = 0.0f;  // Detected pitch in Hertz.

        // Step 1: Calculate the difference function for the filtered buffer.
        difference(filteredBuffer.data());

        // Step 2: Calculate the cumulative mean normalized difference function.
        cumulativeMeanNormalizedDifference();

        // Step 3: Find the first minimum that passes the absolute threshold.
        tauEstimate = absoluteThreshold();

        // Step 4: If a valid tau estimate was found, apply parabolic interpolation for a more accurate estimate.
        if (tauEstimate != -1) {
            float betterTau = parabolicInterpolation(tauEstimate);
            pitchInHz = sampleRate / betterTau;  // Convert tau to frequency (Hz) using the sample rate.
        }

        // Ensure the detected pitch is within the bass guitar range (40 Hz to 400 Hz).
        if (pitchInHz < 40.0f || pitchInHz > 400.0f) {
            pitchInHz = 0.0f;  // Discard pitches outside the valid range.
        }

        return pitchInHz;  // Return the detected pitch in Hz.
    }

private:
    float sampleRate;  // The sample rate of the audio signal.
    int bufferSize;  // The size of the audio buffer to analyze.
    std::vector<float> yinBuffer;  // Buffer used for storing intermediate results of the YIN algorithm.
    float prevSample;  // The previous sample, used in the low-pass filter.

    /**
     * Step 1: Calculates the difference function of the input buffer.
     * The difference function is part of the YIN algorithm, which compares delayed versions of the signal.
     */
    void difference(const float* buffer)
    {
        // Initialize the yinBuffer to zero.
        for (int tau = 0; tau < bufferSize; tau++) {
            yinBuffer[tau] = 0;
        }

        // Calculate the squared difference for each lag (tau).
        for (int tau = 1; tau < bufferSize; tau++) {
            for (int i = 0; i < bufferSize - tau; i++) {
                float delta = buffer[i] - buffer[i + tau];
                yinBuffer[tau] += delta * delta;  // Accumulate the squared differences.
            }
        }
    }

    /**
     * Step 2: Calculates the cumulative mean normalized difference function (CMND).
     * This function normalizes the difference function to help identify the period of the signal.
     */
    void cumulativeMeanNormalizedDifference()
    {
        float runningSum = 0;  // Running sum of differences.
        yinBuffer[0] = 1;  // Set the first value of the CMND to 1 (as per the YIN algorithm).

        // Calculate the cumulative mean normalized difference.
        for (int tau = 1; tau < bufferSize; tau++) {
            runningSum += yinBuffer[tau];
            yinBuffer[tau] *= tau / runningSum;  // Normalize the difference by the running mean.
        }
    }

    /**
     * Step 3: Finds the first minimum value in the CMND that is below a given threshold.
     * This method is used to estimate the period of the signal.
     */
    int absoluteThreshold()
    {
        float threshold = 0.03f;  // Threshold for detecting the first minimum. Lower threshold increases sensitivity.

        // Search for the first value in the CMND that is below the threshold.
        for (int tau = 2; tau < bufferSize; tau++) {
            if (yinBuffer[tau] < threshold) {
                // Continue to the next tau if the current value decreases further.
                while (tau + 1 < bufferSize && yinBuffer[tau + 1] < yinBuffer[tau]) {
                    tau++;
                }
                return tau;  // Return the tau value corresponding to the first minimum.
            }
        }
        return -1;  // Return -1 if no valid tau value is found.
    }

    /**
     * Step 4: Refines the tau estimate using parabolic interpolation.
     * This method improves the accuracy of the pitch estimate by interpolating between points in the CMND.
     */
    float parabolicInterpolation(int tauEstimate)
    {
        float betterTau;
        int x0 = (tauEstimate < 1) ? tauEstimate : tauEstimate - 1;  // Ensure x0 is within bounds.
        int x2 = (tauEstimate + 1 < bufferSize) ? tauEstimate + 1 : tauEstimate;  // Ensure x2 is within bounds.

        // Handle the edge case where tauEstimate is at the boundary.
        if (x0 == tauEstimate)
            return (yinBuffer[tauEstimate] <= yinBuffer[x2]) ? tauEstimate : x2;

        float s0 = yinBuffer[x0];  // Value at x0 (previous sample).
        float s1 = yinBuffer[tauEstimate];  // Value at tauEstimate (current sample).
        float s2 = yinBuffer[x2];  // Value at x2 (next sample).

        // Apply parabolic interpolation formula.
        betterTau = tauEstimate + (s2 - s0) / (2 * (2 * s1 - s2 - s0));
        return betterTau;  // Return the refined tau value.
    }
};
