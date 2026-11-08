#pragma once

#include <utility>
#include <vector>

// Factory presets. Each starts from its instrument's design defaults and the default
// sampler chain, then applies the listed parameter values.
struct FactoryPreset
{
    const char* name;
    const char* instrument;
    std::vector<std::pair<const char*, float>> values;
    const char* drawbars = nullptr; // organ registration such as "888000000"
};

const std::vector<FactoryPreset>& factoryPresets();
