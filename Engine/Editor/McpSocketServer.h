#pragma once

#include "EditorIncludes.h"

#include <asio.hpp>
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>

DELTA_ENGINE_NS_BEGIN

DECLARE_LOG_CATEGORY(LogMcpServer)

class DELTAEDITOR_API McpSocketServer
{
public:
    using RouteHandler   = std::function<std::string(const std::string& json)>;
    using CommandHandler = RouteHandler;
    using QueryHandler   = RouteHandler;

    McpSocketServer(CommandHandler onCommand, QueryHandler onQuery);
    ~McpSocketServer();

    // Starts listening. Returns the bound port, or 0 on failure.
    uint16_t Start(uint16_t preferredPort = 57340);
    void Stop();

private:
    void RunLoop();
    void DoAccept();
    void DoRead();
    void DoWrite(std::string line);
    void HandleIncomingLine(const std::string& line);

    CommandHandler m_onCommand;
    QueryHandler   m_onQuery;

    asio::io_context        m_ioc;
    asio::ip::tcp::acceptor m_acceptor{m_ioc};
    asio::ip::tcp::socket   m_clientSock{m_ioc};
    asio::streambuf         m_readBuf;
    std::atomic<bool>       m_clientConnected{false};
    std::thread             m_thread;
};

DELTA_ENGINE_NS_END
