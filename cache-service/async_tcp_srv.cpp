#include <string>

#include "common/logger.h"
#include "async_tcp_srv.h"

void AsyncTcpServer::Observer::onConnectionAccepted([[maybe_unused]] int cid) { }
void AsyncTcpServer::Observer::onReceived([[maybe_unused]] int cid,
                                          [[maybe_unused]] const char *data,
                                          [[maybe_unused]] const size_t size)
{ }
void AsyncTcpServer::Observer::onConnectionClosed([[maybe_unused]] int cid) { }

AsyncTcpServer::AsyncTcpServer(boost::asio::io_context &ioContext, Observer &observer)
    : m_acceptor{ioContext}
    , m_observer{observer}
    , m_connCount{0}
{
    m_isAccepting.store(false);
    m_isClosing.store(false);
}

bool AsyncTcpServer::listen(const boost::asio::ip::tcp &protocol, uint16_t port)
{
    try
    {
        m_acceptor.open(protocol);
        m_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        m_acceptor.bind({protocol, port});
        m_acceptor.listen(boost::asio::socket_base::max_listen_connections);
    }
    catch (const std::exception &e)
    {
        logger().log(LogSeverity::ERR, {"AsyncTcpServer::listen()", {"what", e.what()}, {"port", port}});
        return false;
    }
    return true;
}

void AsyncTcpServer::startAcceptingConnections()
{
    if (!m_isAccepting.load())
    {
        doAccept();
    }
}

void AsyncTcpServer::send(int cid, const std::string &data)
{
    if (!m_conns.contains(cid))
    {
        logger().log(LogSeverity::ERR, {"AsyncTcpServer::send()", {"what", "connection not found"}, {"id", cid}});
        return;
    }
    m_conns.at(cid)->send(data);
}

void AsyncTcpServer::close()
{
    m_isClosing.store(true);

    m_acceptor.cancel();
    for (const auto &[cid, conn] : m_conns)
    {
        conn->close();
    }
    m_conns.clear();

    logger().log(LogSeverity::INFO, {"server was closed"});

    m_isClosing.store(false);
}

void AsyncTcpServer::doAccept()
{
    m_isAccepting.store(true);
    m_acceptor.async_accept([this](const boost::system::error_code &error, auto socket) {
        if (error)
        {
            logger().log(LogSeverity::ERR, {"AsyncTcpServer::doAccept()", {"what", error.value()}});
            m_isAccepting.store(false);
            return;
        }
        else
        {
            auto conn{TcpConnection::create(std::move(socket), *this, m_connCount)};
            if (!conn)
            {
                logger().log(LogSeverity::ERR, {"unable to create new connection"});
                m_isAccepting.store(false);
                return;
            }

            conn->startReading();
            m_conns.insert({m_connCount, std::move(conn)});
            m_observer.onConnectionAccepted(m_connCount);
            logger().log(LogSeverity::INFO, {"new connection accepted", {"cid", m_connCount}});
            m_connCount++;
        }

        doAccept();
    });
}

void AsyncTcpServer::onReceived(int cid, const char *data, size_t size) { m_observer.onReceived(cid, data, size); }
void AsyncTcpServer::onConnectionClosed(int cid)
{
    if (m_isClosing.load())
    {
        return;
    }

    if (m_conns.erase(cid) > 0)
    {
        logger().log(LogSeverity::INFO, {"connection was closed", {"id", cid}});
        m_observer.onConnectionClosed(cid);
    }
}