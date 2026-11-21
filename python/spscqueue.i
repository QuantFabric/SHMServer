%module spscqueue

%{
#include "SHMSwig.hpp"
%}

%include <stdint.i>
%import "pack_message.i"

%ignore SHMSwig::ServerConf;
%ignore SHMSwig::ClientConf;
%ignore SHMSwig::DataClientConf;
%ignore SHMSwig::SHMServer;
%ignore SHMSwig::SHMConnection;
%ignore SHMSwig::SHMDataConnection;

%rename(SPSCQueue) SHMSwig::SPSCQueue;
%include "SHMSwig.hpp"
