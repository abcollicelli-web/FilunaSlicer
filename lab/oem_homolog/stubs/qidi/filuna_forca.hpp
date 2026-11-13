// Filuna — incluído à força (-include) nos alvos da interface do QIDIStudio, só na régua de paridade.
// Com QDT_RELEASE_TO_PUBLIC=0 a fonte publicada ainda usa estes tipos, mas só os inclui quando a
// opção vale 1; o flag de compilação resolve isso sem editar nenhum arquivo do fabricante.
#pragma once
#include "QIDINetworkTypes.hpp"
#include "P2PManager.hpp"
#include "QIDINetwork.hpp"
#include "QIDIDeviceApi.hpp"
