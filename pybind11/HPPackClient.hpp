#ifndef HPPACKCLIENT_HPP
#define HPPACKCLIENT_HPP

#include <cstdlib>
#include <stdio.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <signal.h>
#include <vector>
#include <time.h>
#include "HPSocket4C.h"
#include "PackMessage.hpp"

class HPPackClient
{
public:
    HPPackClient(const std::string& ip, unsigned int port)
    {
        m_Connected = false;
        m_ServerIP = ip;
        m_ServerPort = port;
        // 创建监听器对象
        m_pListener = ::Create_HP_TcpPackClientListener();
        // 创建 Socket 对象
        m_pClient = ::Create_HP_TcpPackClient(m_pListener);
        // 设置 Socket 监听器回调函数
        ::HP_Set_FN_Client_OnConnect(m_pListener, OnConnect);
        ::HP_Set_FN_Client_OnSend(m_pListener, OnSend);
        ::HP_Set_FN_Client_OnReceive(m_pListener, OnReceive);
        ::HP_Set_FN_Client_OnClose(m_pListener, OnClose);

        HP_TcpPackClient_SetMaxPackSize(m_pClient, 0xFFFF);
        HP_TcpPackClient_SetPackHeaderFlag(m_pClient, 0x169);

        // 注册信号处理函数
        auto signal_handler = [](int signal) {
            if(signal == SIGINT)
            {
                fprintf(stdout, "HPPackClient recv SIGINT,Quit...\n");
            }
            else if(signal == SIGTERM)
            {
                fprintf(stdout, "HPPackClient recv SIGTERM,Quit...\n");
            }
            else if(signal == SIGKILL)
            {
                fprintf(stdout, "HPPackClient recv SIGKILL,Quit...\n");
            }
            fflush(stdout);
            exit(signal);  // 退出程序
        };
        signal(SIGKILL, signal_handler);
        signal(SIGTERM, signal_handler);
        signal(SIGINT, signal_handler);
    }

    ~HPPackClient()
    {
        Stop();
        // 销毁 Socket 对象
        ::Destroy_HP_TcpPackClient(m_pClient);
        // 销毁监听器对象
        ::Destroy_HP_TcpPackClientListener(m_pListener);
    }

    void Start(const std::string& account)
    {
        if (m_Connected)
            return;
        m_Account = account;
        if (::HP_Client_Start(m_pClient, (LPCTSTR)m_ServerIP.c_str(), m_ServerPort, false))
        {
            m_Connected = true;
            fprintf(stdout, "HPPackClient::Start connected to server[%s:%d]\n", m_ServerIP.c_str(), m_ServerPort);
        }
        else
        {
            m_Connected = false;
            fprintf(stderr, "HPPackClient::Start connected to server[%s:%d] failed, error code:%d error massage:%s\n",
                    m_ServerIP.c_str(), m_ServerPort, ::HP_Client_GetLastError(m_pClient), HP_Client_GetLastErrorDesc(m_pClient));
        }

        m_LoginRequest.MessageType = Message::EMessageType::ELoginRequest;
        strncpy(m_LoginRequest.LoginRequest.Account, account.c_str(), sizeof(m_LoginRequest.LoginRequest.Account));
        SendData(m_LoginRequest);
    }

    void ReConnect()
    {
        if(!m_Connected)
        {
            Start(m_Account);
        }
    }

    void Stop()
    {
        ::HP_Client_Stop(m_pClient);
    }

    void SendData(const Message::PackMessage& msg)
    {
        static std::vector<Message::PackMessage> bufferQueue;
        if(!m_Connected)
        {
            fprintf(stderr, "HPPackClient::SendData failed, disconnected to server\n");
            bufferQueue.push_back(msg);
            return;
        }
        for (size_t i = 0; i < bufferQueue.size(); i++)
        {
            ::HP_Client_Send(m_pClient, reinterpret_cast<const unsigned char *>(&bufferQueue.at(i)), sizeof(bufferQueue.at(i)));
        }
        bufferQueue.clear();
        bool ret = ::HP_Client_Send(m_pClient, (const unsigned char*)&msg, sizeof(msg));
        if (!ret)
        {
            fprintf(stderr, "HPPackClient::SendData failed, sys error code:%d, error code:%d, error message:%s\n",
                    SYS_GetLastError(), HP_Client_GetLastError(m_pClient), HP_Client_GetLastErrorDesc(m_pClient));
        }
    }

    void UpdateAppStatus(const std::string& cmd, const std::string& app_name)
    {
        Message::PackMessage message;
        memset(&message, 0, sizeof(message));
        message.MessageType = Message::EMessageType::EAppStatus;
        strncpy(message.AppStatus.Account, m_Account.c_str(), sizeof(message.AppStatus.Account));
        strncpy(message.AppStatus.AppName, app_name.c_str(), sizeof(message.AppStatus.AppName));
        message.AppStatus.PID = getpid();
        strncpy(message.AppStatus.Status, "Start", sizeof(message.AppStatus.Status));

        char command[512] = {0};
        std::string AppLogPath;
        char* p = getenv("APP_LOG_PATH");
        if(p == NULL)
        {
            AppLogPath = "./log/";
        }
        else
        {
            AppLogPath = p;
        }
        sprintf(command, "nohup %s > %s/%s_%s_run.log 2>&1 &", cmd.c_str(), AppLogPath.c_str(), 
                app_name.c_str(), message.AppStatus.Account);
        strncpy(message.AppStatus.StartScript, command, sizeof(message.AppStatus.StartScript));
        strncpy(message.AppStatus.CommitID, APP_COMMITID, sizeof(message.AppStatus.CommitID));
        strncpy(message.AppStatus.StartTime, getCurrentTimeUs(), sizeof(message.AppStatus.StartTime));
        strncpy(message.AppStatus.LastStartTime, getCurrentTimeUs(), sizeof(message.AppStatus.LastStartTime));
        strncpy(message.AppStatus.UpdateTime, getCurrentTimeUs(), sizeof(message.AppStatus.UpdateTime));

        SendData(message);
    }

    static const char *getCurrentTimeUs()
    {
        struct timespec spec = {0, 0};
        clock_gettime(CLOCK_REALTIME, &spec);
        unsigned long n = (spec.tv_sec * 1e9 + spec.tv_nsec) / 1000;
        time_t current = n / 1e6;
        struct tm timeStamp;
        localtime_r(&current, &timeStamp);
        static char szBuffer[64] = {0};
        strftime(szBuffer, sizeof(szBuffer), "%Y-%m-%d %H:%M:%S", &timeStamp);
        unsigned long mod = 1e6;
        unsigned long ret = n % mod;
        sprintf(szBuffer, "%s.%06u", szBuffer, ret);
        return szBuffer;
    }
protected:
    static En_HP_HandleResult __stdcall OnConnect(HP_Client pSender, HP_CONNID dwConnID)
    {
        TCHAR szAddress[50];
        int iAddressLen = sizeof(szAddress) / sizeof(TCHAR);
        USHORT usPort;
        ::HP_Client_GetLocalAddress(pSender, szAddress, &iAddressLen, &usPort);
        fprintf(stdout, "HPPackClient::OnConnect %s:%d connected, dwConnID:%d\n", szAddress, usPort, dwConnID);
        return HR_OK;
    }

    static En_HP_HandleResult __stdcall OnSend(HP_Server pSender, HP_CONNID dwConnID, const BYTE* pData, int iLength)
    {
        return HR_OK;
    }

    static En_HP_HandleResult __stdcall OnReceive(HP_Server pSender, HP_CONNID dwConnID, const BYTE* pData, int iLength)
    {
        return HR_OK;
    }

    static En_HP_HandleResult __stdcall OnClose(HP_Server pSender, HP_CONNID dwConnID, En_HP_SocketOperation enOperation, int iErrorCode)
    {
        TCHAR szAddress[50];
        int iAddressLen = sizeof(szAddress) / sizeof(TCHAR);
        USHORT usPort;
        ::HP_Client_GetLocalAddress(pSender, szAddress, &iAddressLen, &usPort);
        fprintf(stderr, "HPPackClient::OnClose %s:%d disconnected, connID:%d\n", szAddress, usPort, dwConnID);
        return HR_OK;
    }
private:
    std::string m_ServerIP;
    unsigned int m_ServerPort;
    HP_TcpPackClient m_pClient;
    HP_TcpPackClientListener m_pListener;
    bool m_Connected;
    std::string m_Account;
    Message::PackMessage m_LoginRequest;
};


#endif // HPPACKCLIENT_HPP