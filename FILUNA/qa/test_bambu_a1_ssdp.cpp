#include "../fabricantes/bambu/transport/BambuLanDiscovery.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
int main(int argc,char**argv){
 using namespace Slic3r::Filuna::Bambu;
 if(argc!=2){std::cerr<<"Usage: test_bambu_a1_ssdp <response.txt>\n";return 2;}
 std::ifstream f(argv[1],std::ios::binary);
 if(!f){std::cerr<<"no fixture\n";return 3;}
 std::string raw((std::istreambuf_iterator<char>(f)),std::istreambuf_iterator<char>());
 const auto id=parse_ssdp(raw);
 if(!id.valid_a1 || id.model_name!="Bambu Lab A1"){std::cerr<<"not A1: "<<id.model_code<<"\n";return 4;}
 assert(!parse_ssdp("HTTP/1.1 200 OK\r\nDevName.bambu.com: Bambu Lab A1\r\n").valid_a1);
 assert(!parse_ssdp("HTTP/1.1 200 OK\r\nST: urn:bambulab-com:device:3dprinter:1\r\nDevModel.bambu.com: X1\r\nUSN: test\r\nLocation: 127.0.0.1\r\n").valid_a1);
 std::cout<<"PASS A1 official N2S identity; serial="<<id.device_id<<" host="<<id.host<<"\n";
 return 0;
}
