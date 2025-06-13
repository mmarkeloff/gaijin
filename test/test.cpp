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

TEST(TcpTest, ServerSends)
{
    boost::asio::io_context context;

    struct : AsyncTcpServer::Observer
    {
        int cid;
        void onConnectionAccepted(int id) override { cid = id; };
    } serverObserver;

    AsyncTcpServer server{context, serverObserver};
    server.listen(boost::asio::ip::tcp::v4(), 1234);
    server.startAcceptingConnections();

    std::thread running{[&context]() { context.run(); }};

    struct : TcpClient::Observer
    {
        std::string msg{};
        void onReceived(const char *data, size_t size) override { msg.append(data, size); };
    } clientObserver;

    TcpClient client{context, clientObserver};
    client.connect({boost::asio::ip::make_address("127.0.0.1"), 1234});
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::string msg = "test";
    server.send(serverObserver.cid, msg);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_EQ(clientObserver.msg, msg);

    context.stop();
    running.join();
}

TEST(TcpTest, ClientDisconnects)
{
    boost::asio::io_context context;

    struct : AsyncTcpServer::Observer
    {
        bool clientIsConnected = false;
        void onConnectionAccepted(int) override { clientIsConnected = true; };
        void onConnectionClosed(int) override { clientIsConnected = false; };
    } serverObserver;

    AsyncTcpServer server{context, serverObserver};
    server.listen(boost::asio::ip::tcp::v4(), 1234);
    server.startAcceptingConnections();

    std::thread running{[&context]() { context.run(); }};

    TcpClient::Observer clientObserver;
    TcpClient client{context, clientObserver};
    client.connect({boost::asio::ip::make_address("127.0.0.1"), 1234});
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_EQ(serverObserver.clientIsConnected, true);
    client.disconnect();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(serverObserver.clientIsConnected, false);

    context.stop();
    running.join();
}

TEST(TcpTest, ServerCloses)
{
    boost::asio::io_context context;

    AsyncTcpServer::Observer serverObserver;
    AsyncTcpServer server{context, serverObserver};
    server.listen(boost::asio::ip::tcp::v4(), 1234);
    server.startAcceptingConnections();

    std::thread running{[&context]() { context.run(); }};

    struct : TcpClient::Observer
    {
        bool isConnected = false;
        void onConnected() override { isConnected = true; };
        void onDisconnected() override { isConnected = false; };
    } clientObserver;

    TcpClient client{context, clientObserver};
    client.connect({boost::asio::ip::make_address("127.0.0.1"), 1234});
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_EQ(clientObserver.isConnected, true);
    server.close();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(clientObserver.isConnected, false);

    context.stop();
    running.join();
}

void validate(const std::vector<Command> &lhs, const std::vector<Command> &rhs) { 
    EXPECT_EQ(lhs.size(), rhs.size()); 
    for (size_t i = 0; i < lhs.size(); ++i)
    {
        EXPECT_EQ(lhs[i].action, rhs[i].action);
        EXPECT_EQ(lhs[i].key, rhs[i].key);
        EXPECT_EQ(lhs[i].val, rhs[i].val);
    }
 }

TEST(ProtoParseCommandsTest, Simple)
{
    {
        std::string_view data = "$set cat=4";
        auto commands = parseCommands(data, 1);
        validate(commands, {{0, Action::SET, "cat", "4"}});
    }
    {
        std::string_view data = "$set cat=4$get cat";
        auto commands = parseCommands(data, 1);
        validate(commands, {{0, Action::SET, "cat", "4"}, {0, Action::GET, "cat"}});
    }
}

TEST(ProtoParseCommandsTest, Empty)
{
    {
        std::string_view data = "";
        auto commands = parseCommands(data, 1);
        EXPECT_TRUE(commands.empty());
    }
    {
        std::string_view data = "$set";
        auto commands = parseCommands(data, 1);
        EXPECT_TRUE(commands.empty());
    }
    {
        std::string_view data = "$set cat";
        auto commands = parseCommands(data, 1);
        EXPECT_TRUE(commands.empty());
    }
    {
        std::string_view data = "$get";
        auto commands = parseCommands(data, 1);
        EXPECT_TRUE(commands.empty());
    }
}

TEST(ProtoParseCommandsTest, HeadTail)
{
    {
        std::string_view data = "ca$set cat=4$get cat";
        auto commands = parseCommands(data, 1);
        validate(commands, {{0, Action::SET, "cat", "4"}, {0, Action::GET, "cat"}});
    }
    {
        std::string_view data = "$set cat=4$get cat$set";
        auto commands = parseCommands(data, 1);
        validate(commands, {{0, Action::SET, "cat", "4"}, {0, Action::GET, "cat"}});
    }
}
