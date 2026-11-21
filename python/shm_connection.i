%module shm_connection

%{
#include "SHMSwig.hpp"
%}

%include <std_string.i>
%include <stdint.i>
%import "pack_message.i"

%ignore SHMSwig::ServerConf;
%ignore SHMSwig::ClientConf;
%ignore SHMSwig::DataClientConf;
%ignore SHMSwig::SHMServer;
%ignore SHMSwig::SPSCQueue;

%rename(SHMConnection) SHMSwig::SHMConnection;
%rename(SHMDataConnection) SHMSwig::SHMDataConnection;
%include "SHMSwig.hpp"
