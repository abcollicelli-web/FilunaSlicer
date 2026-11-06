#include "../../IPrinterAdapter.hpp"
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/sha.h>
#include <curl/curl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <sstream>
#include <set>
#include <algorithm>
#include <iomanip>
#include <array>
#include <filesystem>
#include <thread>
#include <chrono>

namespace filuna::oem {
namespace {
std::string header(const std::string &wire,const std::string &key) {
  std::istringstream in(wire); std::string line;
  while(std::getline(in,line)) {
    if(!line.empty() && line.back()=='\r') line.pop_back();
    if(line.rfind(key+":",0)==0) {
      auto pos=line.find(':'); auto value=line.substr(pos+1);
      auto n=value.find_first_not_of(" \t");return n==std::string::npos?"":value.substr(n);
    }
  }return "";
}
bool lab_ip(const std::string &ip) {
  return ip.rfind("10.77.0.",0)==0;
}
void len(std::string &out,size_t n) {
  do {unsigned char c=n%128;n/=128;out.push_back(char(c|(n?128:0)));}while(n);
}
std::string str2(const std::string &s) {
  std::string o;o.push_back(char((s.size()>>8)&0xff));o.push_back(char(s.size()&0xff));o+=s;return o;
}
std::string hex(const unsigned char *b,size_t n) {
  std::ostringstream o;for(size_t i=0;i<n;i++)o<<std::hex<<std::setw(2)<<std::setfill('0')<<(unsigned int)b[i];return o.str();
}
std::string sha_file(const std::string &path) {
  std::ifstream f(path,std::ios::binary);
  SHA256_CTX ctx;SHA256_Init(&ctx);std::array<char,8192> b;
  while(f.good()) {f.read(b.data(),b.size());if(f.gcount()>0) SHA256_Update(&ctx,b.data(),f.gcount());}
  unsigned char d[SHA256_DIGEST_LENGTH];SHA256_Final(d,&ctx);return hex(d,sizeof(d));
}
struct SslSession {
  SSL_CTX *ctx=nullptr;SSL *ssl=nullptr;int fd=-1;std::string problem;
  ~SslSession(){if(ssl){SSL_shutdown(ssl);SSL_free(ssl);}if(ctx)SSL_CTX_free(ctx);if(fd>=0)close(fd);}
  bool connect_to(const std::string &ip,const std::string &ca) {
    if(!lab_ip(ip)){problem="only private Filuna LAB subnet is permitted";return false;}
    ctx=SSL_CTX_new(TLS_client_method());
    if(!ctx){problem="SSL_CTX_new failed";return false;}
    SSL_CTX_set_min_proto_version(ctx,TLS1_2_VERSION);
    SSL_CTX_set_verify(ctx,SSL_VERIFY_PEER,nullptr);
    if(SSL_CTX_load_verify_locations(ctx,ca.c_str(),nullptr)!=1){problem="Bambuddy CA load failed";return false;}
    fd=socket(AF_INET,SOCK_STREAM,0);
    sockaddr_in target{};target.sin_family=AF_INET;target.sin_port=htons(8883);
    if(inet_pton(AF_INET,ip.c_str(),&target.sin_addr)!=1){problem="invalid IP";return false;}
    timeval tv{4,0};setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof(tv));
    if(::connect(fd,(sockaddr*)&target,sizeof(target))!=0){problem=std::strerror(errno);return false;}
    ssl=SSL_new(ctx);SSL_set_fd(ssl,fd);
    X509_VERIFY_PARAM *param=SSL_get0_param(ssl);
    X509_VERIFY_PARAM_set1_ip_asc(param,ip.c_str());
    if(SSL_connect(ssl)!=1){problem="SSL_connect or certificate validation failed";return false;}
    if(SSL_get_verify_result(ssl)!=X509_V_OK){problem="unverified TLS certificate";return false;}
    return true;
  }
  bool write_all(const std::string &data) {
    size_t off=0;
    while(off<data.size()) {
      int n=SSL_write(ssl,data.data()+off,int(data.size()-off));
      if(n<=0){problem="SSL_write failed";return false;}off+=n;
    }return true;
  }
  bool exact(char *data,size_t size) {
    size_t off=0;
    while(off<size) {
      int n=SSL_read(ssl,data+off,int(size-off));
      if(n<=0){problem="SSL_read failed / timeout";return false;}off+=n;
    }return true;
  }
  bool recv_packet(unsigned char &kind,std::string &payload) {
    char c;
    if(!exact(&c,1))return false;
    kind=uint8_t(c);
    size_t n=0,mul=1;
    do{if(!exact(&c,1))return false;n+=size_t(uint8_t(c)&127)*mul;mul*=128;}while((uint8_t(c)&128) && mul<=128*128*128*128);
    if(n>1024*1024){problem="MQTT packet > 1 MiB limit";return false;}
    payload.resize(n);return n==0||exact(payload.data(),n);
  }
  bool send_packet(unsigned char kind,const std::string &data) {
    std::string packet(1,char(kind));len(packet,data.size());packet+=data;return write_all(packet);
  }
};
size_t curl_string(char *data,size_t size,size_t nm,void *ptr) {
  static_cast<std::string*>(ptr)->append(data,size*nm);return size*nm;
}
}
class BambuAdapter final : public IPrinterAdapter {
  std::string ca_, ip_, serial_, model_, access_,report_, problem_;
  std::unique_ptr<SslSession> session_;
  int seq_=1;
  Outcome result(bool ok,bool attempted,const std::string& note){return {ok,attempted,note};}
  Outcome publish(const std::string &json) {
    if(!session_)return result(false,false,"no authenticated MQTT session");
    const std::string topic="device/"+serial_+"/request";
    std::string body=str2(topic)+json;
    if(!session_->send_packet(0x30,body))return result(false,true,session_->problem);
    return result(true,true,"MQTT PUBLISH request topic="+topic+" bytes="+std::to_string(json.size()));
  }
  std::string control_json(const std::string &action,const std::string &file="") {
    std::string json="{\"print\":{\"command\":\""+action+"\",\"sequence_id\":\""+std::to_string(++seq_)+"\"";
    if(!file.empty())json+=",\"file\":\""+file+"\",\"subtask_name\":\""+file+"\",\"param\":\"\"";
    return json+"}}";
  }
public:
  explicit BambuAdapter(std::string ca):ca_(std::move(ca)){}
  std::vector<Device> discover() override {
    std::vector<Device> found;
    int fd=socket(AF_INET,SOCK_DGRAM,0);
    if(fd<0)return found;
    int yes=1;setsockopt(fd,SOL_SOCKET,SO_BROADCAST,&yes,sizeof(yes));
    sockaddr_in bind_addr{};bind_addr.sin_family=AF_INET;bind_addr.sin_port=0;inet_pton(AF_INET,"10.77.0.1",&bind_addr.sin_addr);
    if(bind(fd,(sockaddr*)&bind_addr,sizeof(bind_addr))!=0){close(fd);return found;}
    timeval tv{1,0};setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));
    sockaddr_in broadcast{};broadcast.sin_family=AF_INET;broadcast.sin_port=htons(2021);inet_pton(AF_INET,"10.77.0.255",&broadcast.sin_addr);
    const std::string msg="M-SEARCH * HTTP/1.1\r\nHOST: 239.255.255.250:2021\r\nMAN: \"ssdp:discover\"\r\nMX: 1\r\nST: urn:bambulab-com:device:3dprinter:1\r\n\r\n";
    sendto(fd,msg.data(),msg.size(),0,(sockaddr*)&broadcast,sizeof(broadcast));
    std::set<std::string> unique;
    char buffer[4096];sockaddr_in source{};socklen_t sl=sizeof(source);
    while(true){
      ssize_t n=recvfrom(fd,buffer,sizeof(buffer),0,(sockaddr*)&source,&sl);
      if(n<=0)break;
      const std::string wire(buffer,n);
      std::string ip=header(wire,"Location"),serial=header(wire,"USN"),model=header(wire,"DevModel.bambu.com"),name=header(wire,"DevName.bambu.com");
      if(lab_ip(ip)&&!serial.empty()&&unique.insert(serial).second)found.push_back({ip,serial,model,name});
    }
    close(fd);return found;
  }
  Outcome pair(const Device &device,const std::string &access_code) override {
    if(access_code.empty())return result(false,false,"access code not supplied to app adapter");
    if(!lab_ip(device.ip)||device.serial.empty())return result(false,false,"invalid LAB target");
    auto s=std::make_unique<SslSession>();
    if(!s->connect_to(device.ip,ca_))return result(false,true,s->problem);
    std::string payload=str2("MQTT");
    payload+=std::string("\x04\xc2\x00\x3c",4); // version=4, username+password+clean-session, keepalive=60
    payload+=str2("filuna_contract_native_qa");
    payload+=str2("bblp");
    payload+=str2(access_code);
    if(!s->send_packet(0x10,payload))return result(false,true,s->problem);
    unsigned char kind=0;std::string body;
    if(!s->recv_packet(kind,body))return result(false,true,s->problem);
    if(kind!=0x20||body.size()!=2||uint8_t(body[1])!=0)return result(false,true,"MQTT CONNACK rejected or malformed");
    ip_=device.ip;serial_=device.serial;model_=device.model;access_=access_code;
    session_=std::move(s);
    // Subscribe to virtual printer status channel
    auto subscribe_topic="device/"+serial_+"/report";
    std::string sub=std::string("\x00\x01",2)+str2(subscribe_topic)+std::string(1,'\0');
    if(!session_->send_packet(0x82,sub))return result(false,true,session_->problem);
    return result(true,true,"TLS verified; MQTT CONNACK 0; subscribed to report; serial="+serial_);
  }
  Outcome status_stream() override {
    auto req=publish("{\"pushing\":{\"command\":\"pushall\",\"sequence_id\":\""+std::to_string(++seq_)+"\"}}");
    if(!req.ok)return req;
    for(int i=0;i<12;i++){
      unsigned char kind=0;std::string body;
      if(!session_->recv_packet(kind,body))return result(false,true,session_->problem);
      if((kind&0xf0)!=0x30||body.size()<2)continue;
      size_t topic_len=(uint8_t(body[0])<<8)|uint8_t(body[1]);
      if(body.size()<2+topic_len)continue;
      std::string topic=body.substr(2,topic_len);
      std::string json=body.substr(2+topic_len);
      if(topic.find("/report")!=std::string::npos&&json.find("\"print\"")!=std::string::npos){
        report_=json;return result(true,true,"MQTT status received on "+topic+" bytes="+std::to_string(json.size())+" has_ams="+std::to_string(json.find("\"ams\"")!=std::string::npos));
      }
    }
    return result(false,true,"No MQTT print status after pushall");
  }
  Outcome send(const std::string &path) override {
    if(!session_)return result(false,false,"not paired");
    if(path.find("/srv/site/incoming/filuna-local-oem-qa-20261106/")!=0)return result(false,false,"only LAB fixture paths allowed");
    std::ifstream in(path,std::ios::binary);if(!in)return result(false,false,"input missing");
    const std::string basename=path.substr(path.find_last_of('/')+1);
    CURL *c=curl_easy_init();if(!c)return result(false,true,"curl init failed");
    const auto file_size=std::filesystem::file_size(path);
    const auto url="ftps://"+ip_+":990/"+basename;
    curl_easy_setopt(c,CURLOPT_URL,url.c_str());
    curl_easy_setopt(c,CURLOPT_USERNAME,"bblp");
    curl_easy_setopt(c,CURLOPT_PASSWORD,access_.c_str());
    curl_easy_setopt(c,CURLOPT_CAINFO,ca_.c_str());
    curl_easy_setopt(c,CURLOPT_SSL_VERIFYPEER,1L);
    curl_easy_setopt(c,CURLOPT_SSL_VERIFYHOST,2L); // require server certificate host match
    curl_easy_setopt(c,CURLOPT_UPLOAD,1L);
    curl_easy_setopt(c,CURLOPT_READDATA,&in);
    curl_easy_setopt(c,CURLOPT_INFILESIZE_LARGE,(curl_off_t)file_size);
    curl_easy_setopt(c,CURLOPT_CONNECTTIMEOUT,5L);curl_easy_setopt(c,CURLOPT_TIMEOUT,25L);
    CURLcode e=curl_easy_perform(c);curl_easy_cleanup(c);
    if(e!=CURLE_OK)return result(false,true,std::string("FTPS failed: ")+curl_easy_strerror(e));
    return result(true,true,"FTPS upload complete, bytes="+std::to_string(file_size)+", local_sha256="+sha_file(path)+" (receiver-side SHA not verified)");
  }
  Outcome start(const std::string& name) override {return publish(control_json("project_file",name));}
  Outcome pause() override {return publish(control_json("pause"));}
  Outcome resume() override {return publish(control_json("resume"));}
  Outcome stop() override {return publish(control_json("stop"));}
  Outcome camera() override {return result(false,false,"native BambuTunnel video integration not validated in this adapter");}
  Outcome materials() override {
    if(report_.empty())return result(false,false,"status not yet read");
    if(report_.find("\"ams\"")==std::string::npos)return result(false,true,"AMS lite info absent from report");
    return result(true,true,"AMS lite info present in authenticated MQTT report");
  }
};
std::unique_ptr<IPrinterAdapter> create_bambu(const std::string &ca_file) {
  return std::make_unique<BambuAdapter>(ca_file);
}
}
