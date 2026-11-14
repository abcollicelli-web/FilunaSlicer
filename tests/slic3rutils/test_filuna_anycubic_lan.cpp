#include <catch2/catch_test_macros.hpp>

#include <string>

#include <nlohmann/json.hpp>

#include "slic3r/Utils/FilunaAnycubicLanAgent.hpp"

using namespace Slic3r;
using json = nlohmann::json;

// Vetores gerados com a implementacao de referencia em Python (anycubic_ha_local, handshake.py).
TEST_CASE("Filuna Anycubic LAN: assinatura do pedido de controle", "[filuna][anycubic]")
{
    const std::string token = "0123456789abcdefFEDCBA9876543210zzzz";
    REQUIRE(FilunaAnycubicLanAgent::sign(token, 1763160000123LL, "aB3xY9") == "2274a70ff49047f5bfe7ba93f6640404");
}

TEST_CASE("Filuna Anycubic LAN: credenciais do broker decifradas", "[filuna][anycubic]")
{
    const std::string token = "0123456789abcdefFEDCBA9876543210zzzz";
    const std::string b64   = "iIHsGEPrcR01FRPj+pvoaE0TYYDiWSy6mLStBTwvgSRCCDRa09jjyQ+595bIFanmQjkYWoPrVehZ+HXCvnkDH1xg5wN92eYp7mJvxBhTlxW9qIQYB+mKCtrQ5nixpFg4";
    const std::string claro = FilunaAnycubicLanAgent::decrypt_ctrl(b64, token, "tok12345");
    const json        d     = json::parse(claro, nullptr, false);
    REQUIRE(d.is_object());
    REQUIRE(d.value("broker", std::string()) == "mqtts://192.168.1.77:9883");
    REQUIRE(d.value("username", std::string()) == "u1");
    REQUIRE(d.value("password", std::string()) == "p@ss");
    REQUIRE(d.value("deviceId", std::string()) == "dev123");
    // chave errada nao pode devolver texto
    REQUIRE(FilunaAnycubicLanAgent::decrypt_ctrl(b64, "curto", "tok12345").empty());
}

TEST_CASE("Filuna Anycubic LAN: estado traduzido para o DeviceManager", "[filuna][anycubic]")
{
    json e;
    e["info"]      = {{"state", "busy"},
                      {"temp", {{"curr_nozzle_temp", 210}, {"target_nozzle_temp", 215}, {"curr_hotbed_temp", 58}, {"target_hotbed_temp", 60}}},
                      {"project", {{"state", "printing"}, {"pause", 0}, {"progress", 10}, {"filename", "cubo.gcode"}, {"remain_time", 40}}}};
    e["tempature"] = {{"curr_nozzle_temp", 214}};
    e["print"]     = {{"progress", 37}, {"curr_layer", 12}, {"total_layers", 90}, {"remain_time", 33}};
    e["multiColorBox"] = {{"multi_color_box", json::array({{{"id", 0}, {"loaded_slot", 1},
                          {"slots", json::array({{{"index", 0}, {"type", "PLA"}, {"color", json::array({255, 0, 16})}},
                                                 {{"index", 1}, {"type", ""}}})}}})}};
    const json p = FilunaAnycubicLanAgent::to_push_status(e)["print"];
    REQUIRE(p.value("gcode_state", std::string()) == "RUNNING");
    REQUIRE(p.value("mc_percent", 0) == 37);
    REQUIRE(p.value("mc_remaining_time", 0) == 33);
    REQUIRE(p.value("layer_num", 0) == 12);
    REQUIRE(p.value("nozzle_temper", 0.0) == 214.0);
    REQUIRE(p.value("bed_target_temper", 0.0) == 60.0);
    REQUIRE(p["ams"]["ams"][0]["tray"][0].value("tray_color", std::string()) == "FF0010FF");
    REQUIRE(p["ams"]["ams"][0]["tray"][0].value("tray_type", std::string()) == "PLA");
    REQUIRE_FALSE(p["ams"]["ams"][0]["tray"][1].contains("tray_type"));
    REQUIRE(p["ams"].value("tray_now", std::string()) == "1");

    e["info"]["project"]["pause"] = 1;
    REQUIRE(FilunaAnycubicLanAgent::to_push_status(e)["print"].value("gcode_state", std::string()) == "PAUSE");
    e["info"]["state"] = "free";
    e["info"]["project"] = {{"state", "finished"}, {"pause", 0}};
    e.erase("print");
    REQUIRE(FilunaAnycubicLanAgent::to_push_status(e)["print"].value("gcode_state", std::string()) == "FINISH");
}

TEST_CASE("Filuna Anycubic LAN: resposta do info vira descoberta", "[filuna][anycubic]")
{
    const json info = {{"cn", "SERIE123"}, {"modelId", "20025"}, {"modelName", "Anycubic Kobra 3 V2"}, {"ctrlType", "lan"}};
    const json d    = json::parse(FilunaAnycubicLanAgent::info_to_discovery_json(info, "192.168.1.50"), nullptr, false);
    REQUIRE(d.value("dev_id", std::string()) == "SERIE123");
    REQUIRE(d.value("dev_ip", std::string()) == "192.168.1.50");
    REQUIRE(d.value("dev_type", std::string()) == "Anycubic Kobra 3 V2");
    REQUIRE(FilunaAnycubicLanAgent::info_to_discovery_json(json::object(), "192.168.1.50").empty());
}
