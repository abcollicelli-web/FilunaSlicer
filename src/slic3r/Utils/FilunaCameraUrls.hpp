#pragma once

// Filuna — endereço da câmera ao vivo que o agente de rede de cada marca descobre (ex.: Elegoo CC2 responde
// ao método 1042 GetLivingVideoUrl). O DeviceManager não guarda esse dado; a tela de Dispositivos lê daqui.

#include <map>
#include <mutex>
#include <string>

namespace Slic3r { namespace Filuna {

inline std::mutex& camera_urls_mutex()
{
    static std::mutex m;
    return m;
}
inline std::map<std::string, std::string>& camera_urls_map()
{
    static std::map<std::string, std::string> m;
    return m;
}
inline void set_camera_url(const std::string& dev_id, const std::string& url)
{
    std::lock_guard<std::mutex> g(camera_urls_mutex());
    camera_urls_map()[dev_id] = url;
}
inline std::string camera_url(const std::string& dev_id)
{
    std::lock_guard<std::mutex> g(camera_urls_mutex());
    auto it = camera_urls_map().find(dev_id);
    return it == camera_urls_map().end() ? std::string() : it->second;
}

// Arquivos guardados na impressora (os mais recentes primeiro), em JSON: [{"nome","impresso_em","vezes","tempo_s"}].
// Cada agente preenche com o que o protocolo da marca oferece; a tela de Dispositivos mostra embaixo da câmera.
inline std::map<std::string, std::string>& arquivos_map()
{
    static std::map<std::string, std::string> m;
    return m;
}
inline void set_arquivos(const std::string& dev_id, const std::string& lista_json)
{
    std::lock_guard<std::mutex> g(camera_urls_mutex());
    arquivos_map()[dev_id] = lista_json;
}
inline std::string arquivos(const std::string& dev_id)
{
    std::lock_guard<std::mutex> g(camera_urls_mutex());
    auto it = arquivos_map().find(dev_id);
    return it == arquivos_map().end() ? std::string() : it->second;
}

// Último erro que a impressora informou (texto pronto para mostrar), por agente de qualquer marca.
inline std::map<std::string, std::string>& erro_map()
{
    static std::map<std::string, std::string> m;
    return m;
}
inline void set_erro(const std::string& dev_id, const std::string& texto)
{
    std::lock_guard<std::mutex> g(camera_urls_mutex());
    erro_map()[dev_id] = texto;
}
inline std::string erro(const std::string& dev_id)
{
    std::lock_guard<std::mutex> g(camera_urls_mutex());
    auto it = erro_map().find(dev_id);
    return it == erro_map().end() ? std::string() : it->second;
}

// Ajustes que a impressora informa de verdade (não o "último comando enviado"), em JSON:
// {"ventilador":0-100,"ventilador_aux":0-100,"ventilador_caixa":0-100,"velocidade":n}. Só as chaves que a marca informa.
inline std::map<std::string, std::string>& ajustes_map()
{
    static std::map<std::string, std::string> m;
    return m;
}
inline void set_ajustes(const std::string& dev_id, const std::string& ajustes_json)
{
    std::lock_guard<std::mutex> g(camera_urls_mutex());
    ajustes_map()[dev_id] = ajustes_json;
}
inline std::string ajustes(const std::string& dev_id)
{
    std::lock_guard<std::mutex> g(camera_urls_mutex());
    auto it = ajustes_map().find(dev_id);
    return it == ajustes_map().end() ? std::string() : it->second;
}

// Última conferência da mesa pela câmera (IA): JSON {"livre":bool,"motivo":"...","em":<segundos unix>}.
inline std::map<std::string, std::string>& mesa_map()
{
    static std::map<std::string, std::string> m;
    return m;
}
inline void set_mesa(const std::string& dev_id, const std::string& veredito_json)
{
    std::lock_guard<std::mutex> g(camera_urls_mutex());
    mesa_map()[dev_id] = veredito_json;
}
inline std::string mesa(const std::string& dev_id)
{
    std::lock_guard<std::mutex> g(camera_urls_mutex());
    auto it = mesa_map().find(dev_id);
    return it == mesa_map().end() ? std::string() : it->second;
}

}} // namespace Slic3r::Filuna
