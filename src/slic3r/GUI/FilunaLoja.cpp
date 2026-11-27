#include "FilunaLoja.hpp"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <roapi.h>
#include <shobjidl.h>
#include <windows.foundation.collections.h>
#include <windows.services.store.h>
#include <wrl/client.h>
#include <wrl/event.h>
#include <wrl/wrappers/corewrappers.h>

#include <cstdio>

#pragma comment(lib, "runtimeobject.lib")

namespace Slic3r { namespace GUI { namespace FilunaLoja {

namespace {

using Microsoft::WRL::ComPtr;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Foundation::Collections;
using namespace ABI::Windows::Services::Store;

using Lista      = IVectorView<StorePackageUpdate*>;
using OpBusca    = IAsyncOperation<Lista*>;
using OpInstalar = IAsyncOperationWithProgress<StorePackageUpdateResult*, StorePackageUpdateStatus>;

// A thread que chama entra no apartamento multithread do WinRT só durante a chamada.
struct Apartamento
{
    bool meu = false;
    Apartamento() { meu = SUCCEEDED(::RoInitialize(RO_INIT_MULTITHREADED)); }
    ~Apartamento()
    {
        if (meu)
            ::RoUninitialize();
    }
};

std::string motivo(const char* onde, HRESULT hr)
{
    char t[160];
    std::snprintf(t, sizeof(t), "%s (0x%08lX)", onde, static_cast<unsigned long>(hr));
    return t;
}

// Espera a operação assíncrona terminar (a thread já é de trabalho; consulta o estado a cada 200 ms).
HRESULT esperar(IInspectable* op)
{
    ComPtr<IAsyncInfo> info;
    HRESULT hr = op->QueryInterface(IID_PPV_ARGS(&info));
    if (FAILED(hr))
        return hr;
    AsyncStatus estado = AsyncStatus::Started;
    for (;;) {
        hr = info->get_Status(&estado);
        if (FAILED(hr))
            return hr;
        if (estado != AsyncStatus::Started)
            break;
        ::Sleep(200);
    }
    if (estado == AsyncStatus::Completed)
        return S_OK;
    if (estado == AsyncStatus::Canceled)
        return HRESULT_FROM_WIN32(ERROR_CANCELLED);
    HRESULT erro = E_FAIL;
    info->get_ErrorCode(&erro);
    return erro;
}

HRESULT contexto(HWND janela, ComPtr<IStoreContext>& ctx)
{
    ComPtr<IStoreContextStatics> fabrica;
    HRESULT hr = ::RoGetActivationFactory(Microsoft::WRL::Wrappers::HStringReference(RuntimeClass_Windows_Services_Store_StoreContext).Get(),
                                          IID_PPV_ARGS(&fabrica));
    if (FAILED(hr))
        return hr;
    hr = fabrica->GetDefault(&ctx);
    if (FAILED(hr) || janela == nullptr)
        return hr;
    // programa de desktop: a confirmação da Store precisa saber qual janela é a dona
    ComPtr<IInitializeWithWindow> comJanela;
    hr = ctx.As(&comJanela);
    return SUCCEEDED(hr) ? comJanela->Initialize(janela) : hr;
}

HRESULT buscar(IStoreContext* ctx, ComPtr<Lista>& lista)
{
    ComPtr<OpBusca> op;
    HRESULT hr = ctx->GetAppAndOptionalStorePackageUpdatesAsync(&op);
    if (SUCCEEDED(hr))
        hr = esperar(op.Get());
    if (SUCCEEDED(hr))
        hr = op->GetResults(&lista);
    return hr;
}

} // namespace

int procurar(std::string& erro)
{
    Apartamento a;
    ComPtr<IStoreContext> ctx;
    HRESULT hr = contexto(nullptr, ctx);
    if (FAILED(hr)) {
        erro = motivo("Store indisponível", hr);
        return -1;
    }
    ComPtr<Lista> lista;
    hr = buscar(ctx.Get(), lista);
    if (FAILED(hr)) {
        erro = motivo("a Store não respondeu", hr);
        return -1;
    }
    unsigned n = 0;
    lista->get_Size(&n);
    return n > 0 ? 1 : 0;
}

bool instalar(void* janela, const std::function<void(int)>& progresso, std::string& erro)
{
    Apartamento a;
    ComPtr<IStoreContext> ctx;
    HRESULT hr = contexto(static_cast<HWND>(janela), ctx);
    if (FAILED(hr)) {
        erro = motivo("Store indisponível", hr);
        return false;
    }
    ComPtr<Lista> lista;
    hr = buscar(ctx.Get(), lista);
    unsigned n = 0;
    if (SUCCEEDED(hr))
        lista->get_Size(&n);
    if (FAILED(hr) || n == 0) {
        erro = FAILED(hr) ? motivo("a Store não respondeu", hr) : "A Store não tem versão nova para esta conta agora.";
        return false;
    }
    ComPtr<IIterable<StorePackageUpdate*>> itens;
    hr = lista.As(&itens);
    if (FAILED(hr)) {
        erro = motivo("lista da Store", hr);
        return false;
    }
    // depois de instalar, o Windows fecha o programa para trocar os arquivos: registra para abrir de novo sozinho
    // (só depois de atualização; não depois de travar, congelar ou reiniciar o PC)
    ::RegisterApplicationRestart(L"", RESTART_NO_CRASH | RESTART_NO_HANG | RESTART_NO_REBOOT);
    ComPtr<OpInstalar> op;
    hr = ctx->RequestDownloadAndInstallStorePackageUpdatesAsync(itens.Get(), &op);
    if (FAILED(hr)) {
        erro = motivo("a Store recusou o pedido", hr);
        return false;
    }
    auto ao_progredir = Microsoft::WRL::Callback<IAsyncOperationProgressHandler<StorePackageUpdateResult*, StorePackageUpdateStatus>>(
        [&progresso](OpInstalar*, StorePackageUpdateStatus s) -> HRESULT {
            if (progresso) {
                double f = s.TotalDownloadProgress;
                progresso(static_cast<int>((f < 0 ? 0 : f > 1 ? 1 : f) * 100.0 + 0.5));
            }
            return S_OK;
        });
    op->put_Progress(ao_progredir.Get());
    hr = esperar(op.Get());
    if (FAILED(hr)) {
        erro = hr == HRESULT_FROM_WIN32(ERROR_CANCELLED) ? "A atualização foi cancelada." : motivo("a Store não instalou", hr);
        return false;
    }
    ComPtr<IStorePackageUpdateResult> resultado;
    StorePackageUpdateState estado = StorePackageUpdateState_Pending;
    if (SUCCEEDED(op->GetResults(&resultado)) && resultado)
        resultado->get_OverallState(&estado);
    switch (estado) {
    case StorePackageUpdateState_Completed: return true;
    case StorePackageUpdateState_Canceled: erro = "A atualização foi cancelada na Store."; return false;
    case StorePackageUpdateState_OtherError: erro = "A Store não conseguiu instalar. Tente pela página do Filuna na Store."; return false;
    case StorePackageUpdateState_ErrorLowBattery: erro = "Bateria fraca: ligue o carregador e tente de novo."; return false;
    case StorePackageUpdateState_ErrorWiFiRecommended: erro = "A Store pede Wi-Fi para baixar a atualização."; return false;
    case StorePackageUpdateState_ErrorWiFiRequired: erro = "A Store exige Wi-Fi para baixar a atualização."; return false;
    default: erro = "A Store não terminou a instalação."; return false;
    }
}

}}} // namespace Slic3r::GUI::FilunaLoja

#else

namespace Slic3r { namespace GUI { namespace FilunaLoja {
int procurar(std::string& erro)
{
    erro = "Microsoft Store só no Windows";
    return -1;
}
bool instalar(void*, const std::function<void(int)>&, std::string& erro)
{
    erro = "Microsoft Store só no Windows";
    return false;
}
}}} // namespace Slic3r::GUI::FilunaLoja

#endif
