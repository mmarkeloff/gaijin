#pragma once

#include <memory>
#include <utility>

#include <boost/asio.hpp>

#include "../common/tcp_conn.h"

class TcpClient : private TcpConnection::Observer
{
public:
    struct Observer
    {
        virtual void onConnected();
        virtual void onReceived(const char *data, size_t size);
        virtual void onDisconnected();
    };

public:
    TcpClient(boost::asio::io_context &ioContext, Observer &observer);

    void connect(const boost::asio::ip::tcp::endpoint &endpoint);

    bool send(const std::string &data);

    void disconnect();

private:
    void onReceived(int connectionId, const char *data, size_t size) override;
    void onConnectionClosed(int connectionId) override;

private:
    boost::asio::io_context &m_ioContext;

    std::shared_ptr<TcpConnection> m_conn;

    Observer &m_observer;
};