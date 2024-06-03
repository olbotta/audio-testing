#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

//#define SMOOTHING_TIME 0.0f
#define SMOOTHING_TIME 0.04f

// Actual parameters
#define NAME_DW 	"dw"
#define NAME_CUTOFF "co"

#define DEFAULT_DW 0.5f
#define DEFAULT_CO 2000.0f

namespace Parameters
{
	static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
	{
		std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

		params.push_back(std::make_unique<juce::AudioParameterFloat>(NAME_DW, "Dry/Wet", 0.0f, 1.0f, DEFAULT_DW));
		params.push_back(std::make_unique<juce::AudioParameterFloat>(NAME_CUTOFF, "CutOff f", 0.0f, 20000.0f, DEFAULT_CO));

		return {params.begin(), params.end()};
	}
}
