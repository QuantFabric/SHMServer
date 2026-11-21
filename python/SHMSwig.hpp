#ifndef SHMSWIG_HPP
#define SHMSWIG_HPP

#include <cstdint>
#include <string>

#include "Common.hpp"
#include "PackMessage.hpp"
#include "SHMConnection.hpp"
#include "SHMServer.hpp"
#include "SPSCQueue.hpp"

namespace SHMSwig
{

struct ServerConf : public SHMIPC::CommonConf
{
    static constexpr bool Publish = false;
    static constexpr bool Performance = false;
};

struct ClientConf : public SHMIPC::CommonConf
{
    static constexpr bool Publish = false;
};

struct DataClientConf : public SHMIPC::CommonConf
{
    static constexpr bool Publish = true;
};

class SHMServer
    : public SHMIPC::SHMServer<Message::PackMessage, ServerConf>
{
public:
    using Base = SHMIPC::SHMServer<Message::PackMessage, ServerConf>;

    SHMServer() = default;
    virtual ~SHMServer()
    {
        this->Stop();
        this->Join();
    }

    void Start(const std::string& server_name, int32_t cpu_id = -1)
    {
        Base::Start(server_name, cpu_id);
    }

    void Stop()
    {
        Base::Stop();
    }

    bool Push(const Message::PackMessage& msg)
    {
        return Base::Push(msg);
    }

    bool Pop(Message::PackMessage& msg)
    {
        return Base::Pop(msg);
    }

    void PollMsg()
    {
        Base::PollMsg();
    }

    virtual void HandleMsg()
    {
        Base::HandleMsg();
    }
};

class SHMConnection
    : public SHMIPC::SHMConnection<Message::PackMessage, ClientConf>
{
public:
    using Base = SHMIPC::SHMConnection<Message::PackMessage, ClientConf>;

    explicit SHMConnection(const std::string& client_name)
        : Base(client_name)
    {
    }

    virtual ~SHMConnection()
    {
        this->Stop();
        this->Join();
    }

    void Start(const std::string& server_name)
    {
        Base::Start(server_name);
    }

    bool Push(const Message::PackMessage& msg)
    {
        return Base::Push(msg);
    }

    bool Pop(Message::PackMessage& msg)
    {
        return Base::Pop(msg);
    }

    bool IsConnected() const
    {
        return Base::IsConnected();
    }
};

class SHMDataConnection
    : public SHMIPC::SHMConnection<Message::PackMessage, DataClientConf>
{
public:
    using Base =
        SHMIPC::SHMConnection<Message::PackMessage, DataClientConf>;

    explicit SHMDataConnection(const std::string& client_name)
        : Base(client_name)
    {
    }

    virtual ~SHMDataConnection()
    {
        this->Stop();
        this->Join();
    }

    void Start(const std::string& server_name)
    {
        Base::Start(server_name);
    }

    bool Push(const Message::PackMessage& msg)
    {
        return Base::Push(msg);
    }

    bool Pop(Message::PackMessage& msg)
    {
        return Base::Pop(msg);
    }

    bool IsConnected() const
    {
        return Base::IsConnected();
    }
};

class SPSCQueue
    : public SHMIPC::SPSCQueue<Message::PackMessage,
                                SHMIPC::CommonConf::ShmQueueSize>
{
public:
    using Base =
        SHMIPC::SPSCQueue<Message::PackMessage,
                          SHMIPC::CommonConf::ShmQueueSize>;

    SPSCQueue() = default;

    bool Push(const Message::PackMessage& msg)
    {
        return Base::Push(msg);
    }

    bool Pop(Message::PackMessage& msg)
    {
        return Base::Pop(msg);
    }

    void Reset()
    {
        Base::Reset();
    }
};

} // namespace SHMSwig

#endif // SHMSWIG_HPP
