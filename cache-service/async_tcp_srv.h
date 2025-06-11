#pragma once

#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>

#include <boost/asio.hpp>

#include "common/tcp_conn.h"

class AsyncTcpServer : private TcpConnection::Observer
{
public:
    struct Observer
    {
        virtual void onConnectionAccepted(int cid);
        virtual void onReceived(int cid, const char *data, size_t size);
        virtual void onConnectionClosed(int cid);
    };

public:
    AsyncTcpServer(boost::asio::io_context &ioContext, Observer &observer);

    bool listen(const boost::asio::ip::tcp &protocol, uint16_t port);

    void startAcceptingConnections();

    void send(int cid, const std::string &data);

    void close();

private:
    void doAccept();

    void onReceived(int cid, const char *data, size_t size) override;
    void onConnectionClosed(int cid) override;

private:
    boost::asio::ip::tcp::acceptor m_acceptor;

    std::unordered_map<int /* client id */, std::shared_ptr<TcpConnection>> m_conns;
    int m_connCount;

    Observer &m_observer;

    std::atomic_bool m_isAccepting;
    std::atomic_bool m_isClosing;
};
