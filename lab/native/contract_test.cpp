#include "fabricantes/IPrinterAdapter.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <cstdlib>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <ctime>
using namespace filuna::oem;
static std::string jsonescape(const std::string &s) {
 std::string out;
 for(char c:s){
  if(c=='\"'||c=='\\')out.push_back('\\');
  if(c=='\n')out+="\\n";else if(c=='\r')out+="\\r";else if((unsigned char)c<32)out+=" ";else out.push_back(c);
 }return out;
}
int main(int argc,char **argv) {
 if(argc!=4){std::cerr<<"Usage: filuna_contract_test <lab_output_dir> <Bambuddy_CA.crt> <fixture.3mf>\n";return 64;}
 std::filesystem::path out=argv[1];std::filesystem::create_directories(out);
 std::ofstream evidence(out/"bambu-native-contract.jsonl");
 auto obj=create_bambu(argv[2]);
 auto append=[&](const std::string &stage,const Outcome &r) {
  evidence<<"{\"step\":\""<<stage<<"\",\"status\":\""<<(r.attempted?(r.ok?"PASS":"FALHA"):"NÃO EXECUTADO")
          <<"\",\"attempted\":"<<(r.attempted?"true":"false")<<",\"detail\":\""<<jsonescape(r.evidence)<<"\"}\n";
  evidence.flush();std::cout<<stage<<" "<<(r.attempted?(r.ok?"PASS":"FALHA"):"NÃO EXECUTADO")<<" "<<r.evidence<<"\n";
 };
 auto devices=obj->discover();
 auto iter=std::find_if(devices.begin(),devices.end(),[](const Device& x){return x.model=="N2S" && x.ip=="10.77.0.15";});
 if(iter==devices.end()){append("discover",{false,true,"No A1 N2S 10.77.0.15 in SSDP broadcast response"});return 2;}
 auto d=*iter;append("discover",{true,true,"SSDP broadcast discovered A1 serial="+d.serial+" ip="+d.ip});
 const char* code=std::getenv("FILUNA_BAMBU_LAB_ACCESS_CODE");
 if(!code||!*code){append("pair",{false,false,"test credential not injected (no secret is logged)"});return 3;}
 auto paired=obj->pair(d,code);append("pair",paired);
 if(!paired.ok)return 4;
 append("status_stream",obj->status_stream());
 append("materials",obj->materials());
 append("send",obj->send(argv[3]));
 auto base=std::filesystem::path(argv[3]).filename().string();
 append("start",obj->start(base));
 append("pause",obj->pause());
 append("resume",obj->resume());
 append("stop",obj->stop());
 append("camera",obj->camera());
 return 0;
}
