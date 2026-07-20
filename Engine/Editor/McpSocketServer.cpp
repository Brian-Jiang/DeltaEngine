#include "McpSocketServer.h"

#include "Runtime/IO/IOManager.h"

#include <nlohmann/json.hpp>

#include <format>
#include <fstream>
#include <filesystem>

using namespace DeltaEngine;

DEFINE_LOG_CATEGORY(DeltaEngine::LogMcpServer);

McpSocketServer::McpSocketServer(CommandHandler onCommand, QueryHandler onQuery)
    : m_onCommand(std::move(onCommand)), m_onQuery(std::move(onQuery))
{
}

McpSocketServer::~McpSocketServer()
{
    Stop();
}

uint16_t McpSocketServer::Start(uint16_t preferredPort)
{
    asio::error_code ec;
    uint16_t port = preferredPort;

    for (int attempt = 0; attempt < 10; ++attempt, ++port)
    {
        m_acceptor.open(asio::ip::tcp::v4(), ec);
        if (ec)
            break;
        m_acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
        m_acceptor.bind({asio::ip::tcp::v4(), port}, ec);
        if (!ec)
        {
            m_acceptor.listen(1, ec);
            break;
        }
        m_acceptor.close();
    }

    if (ec)
    {
        DLOG(LogMcpServer, ELogLevel::Error, "Failed to bind MCP socket: {}", ec.message());
        return 0;
    }

    auto portFile =
        std::filesystem::path(IOManager::GetIntermediateFolder()) / "EditorState" / "DeltaEditor.port";
    std::filesystem::create_directories(portFile.parent_path());
    {
        std::ofstream out(portFile);
        if (!out || !(out << port))
        {
            DLOG(LogMcpServer,
                 ELogLevel::Error,
                 "Failed to write MCP port file at '{}': expected writable path with port {}",
                 portFile.string(),
                 port);
        }
    }

    DoAccept();
    m_thread = std::thread(&McpSocketServer::RunLoop, this);
    DLOG(LogMcpServer, ELogLevel::Log, "MCP server listening on port {}", port);
    return port;
}

void McpSocketServer::Stop()
{
    asio::post(m_ioc, [this] {
        asio::error_code ec;
        m_acceptor.close(ec);
        m_clientSock.close(ec);
    });
    if (m_thread.joinable())
        m_thread.join();

    auto portFile =
        std::filesystem::path(IOManager::GetIntermediateFolder()) / "EditorState" / "DeltaEditor.port";
    std::error_code rmEc;
    std::filesystem::remove(portFile, rmEc);
    if (rmEc)
        DLOG(LogMcpServer,
             ELogLevel::Verbose,
             "MCP port file remove '{}' failed: {}",
             portFile.string(),
             rmEc.message());
}

void McpSocketServer::RunLoop()
{
    m_ioc.run();
}

void McpSocketServer::DoAccept()
{
    m_acceptor.async_accept(m_clientSock, [this](asio::error_code ec) {
        if (ec)
        {
            if (ec != asio::error::operation_aborted)
                DLOG(LogMcpServer, ELogLevel::Warning, "MCP async_accept failed: {}", ec.message());
            return;
        }

        if (m_clientConnected.exchange(true))
        {
            std::string remote = "(unknown)";
            try
            {
                remote =
                    std::format("{}:{}", m_clientSock.remote_endpoint().address().to_string(),
                               m_clientSock.remote_endpoint().port());
            }
            catch (...)
            {}

            DLOG(LogMcpServer,
                 ELogLevel::Warning,
                 "Rejected additional MCP connection from {} while one client already connected",
                 remote);
            m_clientSock.close();
            DoAccept();
            return;
        }

        DLOG(LogMcpServer, ELogLevel::Log, "MCP client connected: {}",
             m_clientSock.remote_endpoint().address().to_string());
        DoRead();
    });
}

void McpSocketServer::DoRead()
{
    asio::async_read_until(m_clientSock, m_readBuf, '\n',
        [this](asio::error_code ec, std::size_t) {
            if (ec)
            {
                DLOG(LogMcpServer, ELogLevel::Log, "MCP client disconnected");
                m_clientSock.close();
                m_clientConnected = false;
                m_clientSock = asio::ip::tcp::socket(m_ioc);
                DoAccept();
                return;
            }

            std::istream is(&m_readBuf);
            std::string line;
            std::getline(is, line);
            if (!line.empty() && line.back() == '\r')
                line.pop_back();

            HandleIncomingLine(line);
            DoRead();
        });
}

void McpSocketServer::HandleIncomingLine(const std::string& line)
{
    try
    {
        auto j = nlohmann::json::parse(line);
        if (j.value("type", "") == "query")
        {
            std::string response = m_onQuery(line);
            DoWrite(std::move(response));
        }
        else
        {
            std::string response = m_onCommand(line);
            DoWrite(std::move(response));
        }
    }
    catch (const std::exception& e)
    {
        DLOG(LogMcpServer,
             ELogLevel::Warning,
             "MCP inbound line rejected (exception): {} bytes={}",
             e.what(),
             line.size());
        DoWrite(nlohmann::json{{"ok", false}, {"error", e.what()}}.dump());
    }
}

void McpSocketServer::DoWrite(std::string line)
{
    if (!line.empty() && line.back() != '\n')
        line += '\n';
    auto buf = std::make_shared<std::string>(std::move(line));
    asio::async_write(m_clientSock, asio::buffer(*buf),
        [buf](asio::error_code, std::size_t) {});
}
