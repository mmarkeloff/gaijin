#include "gtest/gtest.h"

#include "common/proto.h"
#include "cache-service/async_tcp_srv.h"
#include "cache-clnt/cache_clnt.h"

TEST(TcpTest, ServerAcceptsAndClientConnects)
{
    boost::asio::io_context context;

    struct : AsyncTcpServer::Observer
    {
        bool hasAccepted = false;
        void onConnectionAccepted(int) override { hasAccepted = true; }
    } serverObserver;

    AsyncTcpServer server{context, serverObserver};
    server.listen(boost::asio::ip::tcp::v4(), 1234);
    server.startAcceptingConnections();

    struct : TcpClient::Observer
    {
        bool isConnected = false;
        void onConnected() override { isConnected = true; }
    } clientObserver;

    std::thread running{[&context]() { context.run(); }};

    TcpClient client{context, clientObserver};
    client.connect({boost::asio::ip::make_address("127.0.0.1"), 1234});
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_EQ(serverObserver.hasAccepted, true);
    EXPECT_EQ(clientObserver.isConnected, true);

    context.stop();
    running.join();
}

TEST(TcpTest, ClientSends)
{
    boost::asio::io_context context;

    struct : AsyncTcpServer::Observer
    {
        std::string msg;
        size_t messageCount = 0;
        void onReceived(int, const char *data, size_t size) override { msg.append(data, size); };
    } serverObserver;

    AsyncTcpServer server{context, serverObserver};
    server.listen(boost::asio::ip::tcp::v4(), 1234);
    server.startAcceptingConnections();

    std::thread running{[&context]() { context.run(); }};

    TcpClient::Observer clientObserver;
    TcpClient client{context, clientObserver};
    client.connect({boost::asio::ip::make_address("127.0.0.1"), 1234});
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::string msg = "test";
    client.send(msg);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_EQ(msg, serverObserver.msg);

    context.stop();
    running.join();
}

void validateCommand(const Command &command, Action action, Key key, std::optional<Value> val = std::nullopt)
{
    EXPECT_EQ(command.action, action);
    EXPECT_EQ(command.key, key);
    if (val.has_value())
    {
        EXPECT_TRUE(command.val.has_value());
        EXPECT_EQ(command.val.value(), val);
    }
    else
    {
        EXPECT_FALSE(command.val.has_value());
    }
}

TEST(ProtoTest, Simple)
{
    std::string_view data = "$set cat=4$get cat";
    auto commands = parseCommands(data, 1);
    EXPECT_TRUE(commands.size() == 2);
    validateCommand(commands[0], Action::SET, "cat", "4");
    validateCommand(commands[1], Action::GET, "cat");
}

TEST(ProtoTest, Head)
{
    std::string_view data = "ca$set cat=4$get cat";
    auto commands = parseCommands(data, 1);
    EXPECT_TRUE(commands.size() == 2);
    validateCommand(commands[0], Action::SET, "cat", "4");
    validateCommand(commands[1], Action::GET, "cat");
}

TEST(ProtoTest, Tail)
{
    std::string_view data = "$set cat=4$get cat$set";
    auto commands = parseCommands(data, 1);
    EXPECT_TRUE(commands.size() == 2);
    validateCommand(commands[0], Action::SET, "cat", "4");
    validateCommand(commands[1], Action::GET, "cat");
}