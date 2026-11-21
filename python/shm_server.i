%module(directors="1") shm_server

%{
#include "SHMSwig.hpp"
%}

%include <std_string.i>
%include <stdint.i>
%import "pack_message.i"

namespace SHMIPC
{
template <class T, class Conf>
class SHMServer
{
public:
    virtual ~SHMServer();
    virtual void HandleMsg();
};
}

%ignore SHMSwig::ServerConf;
%ignore SHMSwig::ClientConf;
%ignore SHMSwig::DataClientConf;
%ignore SHMSwig::SHMConnection;
%ignore SHMSwig::SHMDataConnection;
%ignore SHMSwig::SPSCQueue;

%feature("director") SHMServer;
%feature("director") SHMSwig::SHMServer;
%rename(SHMServer) SHMSwig::SHMServer;
%include "SHMSwig.hpp"
