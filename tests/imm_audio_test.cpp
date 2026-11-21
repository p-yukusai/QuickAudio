#include "../src/imm_audio.h"

#include <QCoreApplication>
#include <cstdlib>

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(com)) return EXIT_FAILURE;
    int result = EXIT_SUCCESS;
    {
        ImmAudio audio;
        std::vector<ImmAudio::OutputDevice> devices;
        if (audio.outputDevices(devices) != E_UNEXPECTED || audio.selectOutputDevice({}) != E_UNEXPECTED)
            result = EXIT_FAILURE;
        if (audio.setSelectedOutputDeviceAsDefault() != E_UNEXPECTED) result = EXIT_FAILURE;
        audio.initialize();
        if (audio.setSelectedOutputDeviceAsDefault() != E_INVALIDARG) result = EXIT_FAILURE;
        if (FAILED(audio.outputDevices(devices))) result = EXIT_FAILURE;
        if (!audio.selectedOutputDevice().isEmpty()) result = EXIT_FAILURE;
        if (SUCCEEDED(audio.selectOutputDevice(QStringLiteral("not-an-audio-endpoint"))) ||
            !audio.selectedOutputDevice().isEmpty()) result = EXIT_FAILURE;
        unsigned defaults = 0;
        for (const auto &device : devices) {
            if (device.id.isEmpty()) result = EXIT_FAILURE;
            if (device.isDefault) ++defaults;
            if (FAILED(audio.selectOutputDevice(device.id)) || audio.selectedOutputDevice() != device.id)
                result = EXIT_FAILURE;
        }
        if (defaults > 1) result = EXIT_FAILURE;
        audio.selectOutputDevice({});
        if (!audio.selectedOutputDevice().isEmpty()) result = EXIT_FAILURE;
        if (audio.setSelectedOutputDeviceAsDefault() != E_INVALIDARG) result = EXIT_FAILURE;
    }
    CoUninitialize();
    return result;
}