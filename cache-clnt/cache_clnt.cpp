#include "cache_clnt.h"
#include "common/logger.h"

void TcpClient::Observer::onConnected() { }
void TcpClient::Observer::onReceived([[maybe_unused]] const char *data, [[maybe_unused]] size_t size) { }
void TcpClient::Observer::onDisconnected() { }

TcpClient::TcpClient(boost::asio::io_context &ioContext, Observer &observer)
    : m_ioContext{ioContext}
    , m_observer{observer}
{ }

void TcpClient::connect(const boost::asio::ip::tcp::endpoint &endpoint)
{
    if (m_conn)
    {
        return;
    }
    auto socket = std::make_shared<boost::asio::ip::tcp::socket>(m_ioContext);
    socket->async_connect(endpoint, [this, socket](const boost::system::error_code &error) {
        if (error)
        {
            logger().log(LogSeverity::ERR, {"unable to connect to server", {"reason", error.value()}});
            m_observer.onDisconnected();
            return;
        }
        m_conn = TcpConnection::create(std::move(*socket), *this);
        if (!m_conn)
        {
            logger().log(LogSeverity::ERR, {"unable to create connection"});
            m_observer.onDisconnected();
            return;
        }
        m_conn->startReading();
        logger().log(LogSeverity::DEBUG, {"client connected"});
        m_observer.onConnected();
    });
}

bool TcpClient::send(const std::string &data)
{
    if (!m_conn)
    {
        logger().log(LogSeverity::ERR, {"unable to send data to server", {"reason", "no connection"}});
        return false;
    }
    m_conn->send(data);
    return true;
}

void TcpClient::disconnect()
{
    if (m_conn)
    {
        m_conn->close();
    }
}

void TcpClient::onReceived([[maybe_unused]] int cid, const char *data, size_t size)
{
    m_observer.onReceived(data, size);
}
void TcpClient::onConnectionClosed([[maybe_unused]] int cid)
{
    if (m_conn)
    {
        m_conn.reset();
        logger().log(LogSeverity::INFO, {"client was disconnected"});
        m_observer.onDisconnected();
    }
}