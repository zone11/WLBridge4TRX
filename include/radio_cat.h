#include <array>

struct RadioCatSettings {
    const char* radio;
    const char* commandFreq;
    const char* commandMode;
    const char* commandPower;
    int commandFreqPosition;
    int commandFreqLength;
    int commandModePosition;
    int commandModeLength;
    int commandPowerPosition;
    int commandPowerLength;
    std::array<const char*, 9> commandModes;
};

const RadioCatSettings radioConfigs[] = {
    {
        "Elecraft KX",
        "FA",
        "MD",
        "",
        2, 11,
        2, 1,
        0, 0,
        { "ERR", "LSB", "USB", "CW-U", "FM", "AM", "DATA", "CW-REV", "DATA-REV" }
    }, 
    {
        "Yaesu FT991",
        "FA",
        "MD0",
        "",
        2, 9,
        2, 1,
        0, 0,
        {"ERR", "LSB", "USB", "CW","FM","AM"}
    }
};