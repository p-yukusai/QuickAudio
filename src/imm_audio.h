#pragma once

#include <Windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <propsys.h>
#include <wrl/client.h>
#include <QObject>
#include <QString>
#include <QThread>
#include <atomic>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

class ImmAudio : public QObject {
    Q_OBJECT
public:
    explicit ImmAudio(QObject *parent = nullptr) : QObject(parent), gate(std::make_shared<Gate>()) {
        gate->owner = this;
    }

    ~ImmAudio() override {
        {
            std::lock_guard<std::mutex> lock(gate->mutex);
            gate->owner = nullptr;
        }
        if (endpoint && callback) endpoint->UnregisterControlChangeNotify(callback.Get());
        if (enumerator && callback) enumerator->UnregisterEndpointNotificationCallback(callback.Get());
    }

    HRESULT initialize() {
        if (!onThread()) return RPC_E_WRONG_THREAD;
        if (!enumerator) {
            Microsoft::WRL::ComPtr<IMMDeviceEnumerator> next;
            HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                          IID_PPV_ARGS(next.GetAddressOf()));
            if (FAILED(hr)) return hr;
            Microsoft::WRL::ComPtr<Callback> cb;
            cb.Attach(new Callback(gate));
            hr = next->RegisterEndpointNotificationCallback(cb.Get());
            if (FAILED(hr)) return hr;
            enumerator = next;
            callback = cb;
        }
        return refresh();
    }

    HRESULT getVolumes(std::vector<float> &volumes) const {
        volumes.clear();
        if (!onThread()) return RPC_E_WRONG_THREAD;
        if (!endpoint) return E_NOTFOUND;
        UINT32 count = 0;
        HRESULT hr = endpoint->GetChannelCount(&count);
        if (FAILED(hr)) return hr;
        if (count == 0) return E_FAIL;
        volumes.resize(count);
        for (UINT32 i = 0; i < count; ++i) {
            hr = endpoint->GetChannelVolumeLevelScalar(i, &volumes[i]);
            if (FAILED(hr)) {
                volumes.clear();
                return hr;
            }
        }
        return S_OK;
    }

    HRESULT setVolume(UINT32 channel, float volume) const {
        if (!onThread()) return RPC_E_WRONG_THREAD;
        if (!endpoint) return E_NOTFOUND;
        if (!(volume >= 0.0f && volume <= 1.0f)) return E_INVALIDARG;
        return endpoint->SetChannelVolumeLevelScalar(channel, volume, nullptr);
    }

    HRESULT masterVolume(float &volume) const {
        if (!onThread()) return RPC_E_WRONG_THREAD;
        if (!endpoint) return E_NOTFOUND;
        return endpoint->GetMasterVolumeLevelScalar(&volume);
    }

    HRESULT setMasterVolume(float volume) const {
        if (!onThread()) return RPC_E_WRONG_THREAD;
        if (!endpoint) return E_NOTFOUND;
        if (!(volume >= 0.0f && volume <= 1.0f)) return E_INVALIDARG;
        return endpoint->SetMasterVolumeLevelScalar(volume, nullptr);
    }

    QString deviceName() const {
        Q_ASSERT(onThread());
        return name;
    }

signals:
    void stateChanged();

private:
    struct Gate {
        std::mutex mutex;
        ImmAudio *owner = nullptr;
    };

    class Callback final : public IAudioEndpointVolumeCallback, public IMMNotificationClient {
    public:
        explicit Callback(std::shared_ptr<Gate> gate) : gate(std::move(gate)) {}

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **object) override {
            if (!object) return E_POINTER;
            *object = nullptr;
            if (iid == __uuidof(IUnknown) || iid == __uuidof(IAudioEndpointVolumeCallback))
                *object = static_cast<IAudioEndpointVolumeCallback *>(this);
            else if (iid == __uuidof(IMMNotificationClient))
                *object = static_cast<IMMNotificationClient *>(this);
            else return E_NOINTERFACE;
            AddRef();
            return S_OK;
        }
        ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
        ULONG STDMETHODCALLTYPE Release() override {
            const ULONG remaining = --refs;
            if (!remaining) delete this;
            return remaining;
        }
        HRESULT STDMETHODCALLTYPE OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA) override {
            post(false);
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR) override {
            if (flow == eRender && role == eConsole) post(true);
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR) override { return S_OK; }
        HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR) override { return S_OK; }
        HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR, DWORD) override { return S_OK; }
        HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR, const PROPERTYKEY) override {
            post(true);
            return S_OK;
        }

    private:
        void post(bool deviceChanged) {
            std::lock_guard<std::mutex> lock(gate->mutex);
            ImmAudio *owner = gate->owner;
            if (!owner) return;
            QMetaObject::invokeMethod(owner, [owner, deviceChanged] {
                if (deviceChanged) owner->refresh();
                else emit owner->stateChanged();
            }, Qt::QueuedConnection);
        }

        std::atomic<ULONG> refs{1};
        std::shared_ptr<Gate> gate;
    };

    bool onThread() const { return QThread::currentThread() == thread(); }

    HRESULT refresh() {
        if (endpoint && callback) endpoint->UnregisterControlChangeNotify(callback.Get());
        endpoint.Reset();
        Microsoft::WRL::ComPtr<IMMDevice> device;
        Microsoft::WRL::ComPtr<IAudioEndpointVolume> next;
        QString nextName;
        HRESULT hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, device.GetAddressOf());
        if (SUCCEEDED(hr)) {
            hr = device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr,
                                  reinterpret_cast<void **>(next.GetAddressOf()));
            if (SUCCEEDED(hr)) {
                hr = next->RegisterControlChangeNotify(callback.Get());
                if (FAILED(hr)) next.Reset();
            }
            if (SUCCEEDED(hr)) {
                Microsoft::WRL::ComPtr<IPropertyStore> properties;
                if (SUCCEEDED(device->OpenPropertyStore(STGM_READ, properties.GetAddressOf()))) {
                    const PROPERTYKEY friendlyName = {{0xa45c254e, 0xdf1c, 0x4efd,
                                                       {0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0}}, 14};
                    PROPVARIANT value;
                    PropVariantInit(&value);
                    if (SUCCEEDED(properties->GetValue(friendlyName, &value)) && value.vt == VT_LPWSTR && value.pwszVal)
                        nextName = QString::fromWCharArray(value.pwszVal);
                    PropVariantClear(&value);
                }
            }
        }
        endpoint = next;
        name = nextName;
        emit stateChanged();
        return hr;
    }

    std::shared_ptr<Gate> gate;
    Microsoft::WRL::ComPtr<IMMDeviceEnumerator> enumerator;
    Microsoft::WRL::ComPtr<Callback> callback;
    Microsoft::WRL::ComPtr<IAudioEndpointVolume> endpoint;
    QString name;
};
