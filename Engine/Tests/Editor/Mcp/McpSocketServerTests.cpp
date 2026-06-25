#include "Editor/Mcp/McpProtocol.h"
#include "Editor/McpSocketServer.h"

#include <asio.hpp>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <chrono>
#include <string>
#include <thread>

using json = nlohmann::json;
using namespace DeltaEngine;

namespace
{

std::string ReadLine(asio::ip::tcp::socket& sock)
{
    asio::streambuf buf;
    asio::read_until(sock, buf, '\n');
    std::istream is(&buf);
    std::string line;
    std::getline(is, line);
    return line;
}

void SendLine(asio::ip::tcp::socket& sock, const std::string& line)
{
    std::string payload = line;
    if (payload.empty() || payload.back() != '\n')
        payload += '\n';
    asio::write(sock, asio::buffer(payload));
}

} // namespace

TEST(McpSocketServerTests, Command_TwoPhaseAcceptThenSendResponse)
{
    McpSocketServer server(
        [](const std::string& line) {
            const json inbound = json::parse(line);
            const std::string requestId = inbound.value("request_id", "test-id");
            return MakeAcceptResponse(
                       requestId,
                       "CreateGameObject",
                       json{{"ok", true}, {"queued", true}, {"expects_result", true}})
                .dump();
        },
        [](const std::string&) { return json{{"ok", true}, {"objects", json::array()}}.dump(); });

    const uint16_t port = server.Start(57350);
    ASSERT_NE(port, 0u);

    asio::io_context ioc;
    asio::ip::tcp::socket client(ioc);
    client.connect({asio::ip::make_address("127.0.0.1"), port});

    SendLine(
        client,
        R"({"type":"command","system":"scene","command":"CreateGameObject","params":{},"request_id":"req-sock-1"})");

    const json accept = json::parse(ReadLine(client));
    EXPECT_EQ(accept["phase"].get<std::string>(), "accept");
    EXPECT_EQ(accept["request_id"].get<std::string>(), "req-sock-1");
    EXPECT_TRUE(accept["expects_result"].get<bool>());

    server.SendResponse(
        MakeResultResponse("req-sock-1", json{{"ok", true}, {"objectId", "obj-sock-1"}}).dump());

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    const json result = json::parse(ReadLine(client));
    EXPECT_EQ(result["phase"].get<std::string>(), "result");
    EXPECT_EQ(result["request_id"].get<std::string>(), "req-sock-1");
    EXPECT_EQ(result["objectId"].get<std::string>(), "obj-sock-1");

    server.Stop();
}

TEST(McpSocketServerTests, Query_SingleSyncResponse)
{
    McpSocketServer server(
        [](const std::string&) { return json{{"ok", false}, {"error", "unexpected command"}}.dump(); },
        [](const std::string&) {
            return json{{"ok", true}, {"systems", json::object()}}.dump();
        });

    const uint16_t port = server.Start(57351);
    ASSERT_NE(port, 0u);

    asio::io_context ioc;
    asio::ip::tcp::socket client(ioc);
    client.connect({asio::ip::make_address("127.0.0.1"), port});

    SendLine(client, R"({"type":"query","system":"meta","query":"list_operations","params":{}})");

    const json response = json::parse(ReadLine(client));
    EXPECT_TRUE(response["ok"].get<bool>());
    EXPECT_FALSE(response.contains("phase"));

    server.Stop();
}
