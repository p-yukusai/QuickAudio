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
    struct OutputDevice {
        QString id;
        QString name;
        bool isDefault = false;
    };
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

    HRESULT outputDevices(std::vector<OutputDevice> &devices) const {
        devices.clear();
        if (!onThread()) return RPC_E_WRONG_THREAD;
        if (!enumerator) return E_UNEXPECTED;
        Microsoft::WRL::ComPtr<IMMDeviceCollection> collection;
        HRESULT hr = enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, collection.GetAddressOf());
        if (FAILED(hr)) return hr;
        Microsoft::WRL::ComPtr<IMMDevice> defaultDevice;
        QString defaultId;
        if (SUCCEEDED(enumerator->GetDefaultAudioEndpoint(eRender, eConsole, defaultDevice.GetAddressOf())))
            deviceId(defaultDevice.Get(), defaultId);
        UINT count = 0;
        hr = collection->GetCount(&count);
        if (FAILED(hr)) return hr;
        std::vector<OutputDevice> result;
        for (UINT i = 0; i < count; ++i) {
            Microsoft::WRL::ComPtr<IMMDevice> device;
            hr = collection->Item(i, device.GetAddressOf());
            if (FAILED(hr)) return hr;
            OutputDevice item;
            hr = deviceId(device.Get(), item.id);
            if (FAILED(hr)) return hr;
            item.name = friendlyName(device.Get());
            item.isDefault = !defaultId.isEmpty() && item.id == defaultId;
            result.push_back(std::move(item));
        }
        devices = std::move(result);
        return S_OK;
    }

    HRESULT selectOutputDevice(const QString &id) {
        if (!onThread()) return RPC_E_WRONG_THREAD;
        if (!enumerator) return E_UNEXPECTED;
        if (!id.isEmpty()) {
            HRESULT hr = validateOutputDevice(id);
            if (FAILED(hr)) return hr;
        }
        selectedId = id;
        return refresh();
    }

    // On failure, roles already updated are not rolled back.
    HRESULT setSelectedOutputDeviceAsDefault() {
        if (!onThread()) return RPC_E_WRONG_THREAD;
        if (!enumerator) return E_UNEXPECTED;
        if (selectedId.isEmpty() || selectedId.contains(QChar(u'\0'))) return E_INVALIDARG;
        HRESULT hr = validateOutputDevice(selectedId);
        if (FAILED(hr)) return hr;
        const CLSID policyClient = {0x870af99c, 0x171d, 0x4f9e,
                                    {0xaf, 0x0d, 0xe6, 0x3d, 0xf4, 0x0c, 0x2b, 0xc9}};
        Microsoft::WRL::ComPtr<PolicyConfig> policy;
        hr = CoCreateInstance(policyClient, nullptr, CLSCTX_ALL, IID_PPV_ARGS(policy.GetAddressOf()));
        if (FAILED(hr)) return hr;
        for (const ERole role : {eConsole, eMultimedia, eCommunications}) {
            hr = policy->SetDefaultEndpoint(reinterpret_cast<LPCWSTR>(selectedId.utf16()), role);
            if (FAILED(hr)) return hr;
        }
        return S_OK;
    }

    QString selectedOutputDevice() const {
        Q_ASSERT(onThread());
        return selectedId;
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
    void outputDevicesChanged();

private:
    // Windows exposes no documented API for changing the system default endpoint.
    struct __declspec(uuid("f8679f50-850a-41cf-9c72-430f290290c8")) PolicyConfig : IUnknown {
        virtual HRESULT STDMETHODCALLTYPE GetMixFormat() = 0;
        virtual HRESULT STDMETHODCALLTYPE GetDeviceFormat() = 0;
        virtual HRESULT STDMETHODCALLTYPE ResetDeviceFormat() = 0;
        virtual HRESULT STDMETHODCALLTYPE SetDeviceFormat() = 0;
        virtual HRESULT STDMETHODCALLTYPE GetProcessingPeriod() = 0;
        virtual HRESULT STDMETHODCALLTYPE SetProcessingPeriod() = 0;
        virtual HRESULT STDMETHODCALLTYPE GetShareMode() = 0;
        virtual HRESULT STDMETHODCALLTYPE SetShareMode() = 0;
        virtual HRESULT STDMETHODCALLTYPE GetPropertyValue() = 0;
        virtual HRESULT STDMETHODCALLTYPE SetPropertyValue() = 0;
        virtual HRESULT STDMETHODCALLTYPE SetDefaultEndpoint(LPCWSTR id, ERole role) = 0;
    };
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
        HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR) override { post(true); return S_OK; }
        HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR) override { post(true); return S_OK; }
        HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR, DWORD) override { post(true); return S_OK; }
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
                if (deviceChanged) {
                    owner->refresh();
                    emit owner->outputDevicesChanged();
                }
                else emit owner->stateChanged();
            }, Qt::QueuedConnection);
        }

        std::atomic<ULONG> refs{1};
        std::shared_ptr<Gate> gate;
    };

    bool onThread() const { return QThread::currentThread() == thread(); }

    HRESULT validateOutputDevice(const QString &id) const {
        if (id.contains(QChar(u'\0'))) return E_INVALIDARG;
        Microsoft::WRL::ComPtr<IMMDevice> device;
        HRESULT hr = enumerator->GetDevice(reinterpret_cast<LPCWSTR>(id.utf16()), device.GetAddressOf());
        if (FAILED(hr)) return hr;
        DWORD state = 0;
        hr = device->GetState(&state);
        if (FAILED(hr)) return hr;
        if (state != DEVICE_STATE_ACTIVE) return E_NOTFOUND;
        Microsoft::WRL::ComPtr<IMMEndpoint> audioEndpoint;
        hr = device.As(&audioEndpoint);
        if (FAILED(hr)) return hr;
        EDataFlow flow;
        hr = audioEndpoint->GetDataFlow(&flow);
        if (FAILED(hr)) return hr;
        return flow == eRender ? S_OK : E_INVALIDARG;
    }

    static HRESULT deviceId(IMMDevice *device, QString &id) {
        LPWSTR value = nullptr;
        HRESULT hr = device->GetId(&value);
        if (SUCCEEDED(hr)) {
            id = QString::fromWCharArray(value);
            CoTaskMemFree(value);
        }
        return hr;
    }

    static QString friendlyName(IMMDevice *device) {
        QString name;
        Microsoft::WRL::ComPtr<IPropertyStore> properties;
        if (SUCCEEDED(device->OpenPropertyStore(STGM_READ, properties.GetAddressOf()))) {
            const PROPERTYKEY key = {{0xa45c254e, 0xdf1c, 0x4efd,
                                      {0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0}}, 14};
            PROPVARIANT value;
            PropVariantInit(&value);
            if (SUCCEEDED(properties->GetValue(key, &value)) && value.vt == VT_LPWSTR && value.pwszVal)
                name = QString::fromWCharArray(value.pwszVal);
            PropVariantClear(&value);
        }
        return name;
    }

    HRESULT refresh() {
        if (endpoint && callback) endpoint->UnregisterControlChangeNotify(callback.Get());
        endpoint.Reset();
        Microsoft::WRL::ComPtr<IMMDevice> device;
        Microsoft::WRL::ComPtr<IAudioEndpointVolume> next;
        QString nextName;
        HRESULT hr = selectedId.isEmpty()
            ? enumerator->GetDefaultAudioEndpoint(eRender, eConsole, device.GetAddressOf())
            : enumerator->GetDevice(reinterpret_cast<LPCWSTR>(selectedId.utf16()), device.GetAddressOf());
        if (SUCCEEDED(hr)) {
            DWORD state = 0;
            hr = device->GetState(&state);
            if (SUCCEEDED(hr) && state != DEVICE_STATE_ACTIVE) hr = E_NOTFOUND;
        }
        if (SUCCEEDED(hr)) {
            hr = device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr,
                                  reinterpret_cast<void **>(next.GetAddressOf()));
            if (SUCCEEDED(hr)) {
                hr = next->RegisterControlChangeNotify(callback.Get());
                if (FAILED(hr)) next.Reset();
            }
            if (SUCCEEDED(hr)) {
                nextName = friendlyName(device.Get());
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
    QString selectedId;
};
