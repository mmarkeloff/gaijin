#include <cstdlib>
#include <ctime>
#include <random>

#include "cache_clnt.h"
#include "common/logger.h"
#include "common/utils.h"
#include "common/proto.h"


std::string getCommand(const Key &key)
{
    int val = std::rand() % 1000;
    std::string command;
    if (std::rand() % 100 == 0)
    {
        command = "$set " + key + "=" + std::to_string(val);
    }
    else
    {
        command = "$get " + key;
    }
    return command;
}

int main()
{
    std::random_device rd;
    std::mt19937 gen(rd());

    struct : TcpClient::Observer
    {
        void onConnected() { logger().log(LogSeverity::DEBUG, {"connected"}); }
        void onDisconnected() { logger().log(LogSeverity::DEBUG, {"disconnected"}); }
        void onReceived(const char *data, size_t size)
        {
            auto tokens = split(std::string_view{data, size}, RESPONSE_DELIM);
            for (auto token : tokens)
            {
                logger().log(LogSeverity::INFO, {"received", {"data", token}});
            }
        }
    } observer;

    boost::asio::io_context context;
    std::thread running{[&context]() { context.run(); }};

    TcpClient client{context, observer};

    std::vector<Key> keys = {"cat", "zat", "tat", "mat", "oao", "rat", "bat", "qat", "aot"};
    std::uniform_int_distribution<> d(0, keys.size() - 1);

    for (int64_t i = 0; i < 5000; ++i)
    {
        while (!client.send(getCommand(keys[d(gen)])))
        {
            {
                client.disconnect();
                context.restart();
                running.join();
                running = std::thread{[&context]() { context.run(); }};
                client.connect({boost::asio::ip::make_address("127.0.0.1"), 1234});
                std::this_thread::sleep_for(std::chrono::milliseconds{10000});
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
    std::this_thread::sleep_for(std::chrono::seconds{5});

    client.disconnect();
    context.stop();
    running.join();

    return 0;
}