#include "common/logger.h"

#include "tcp_conn.h"

namespace
{
constexpr auto readBufferSize{1024};
} // namespace

void TcpConnection::Observer::onConnectionClosed([[maybe_unused]] int cid) { }
void TcpConnection::Observer::onReceived([[maybe_unused]] int cid,
                                         [[maybe_unused]] const char *data,
                                         [[maybe_unused]] const size_t size)
{ }

TcpConnection::TcpConnection(boost::asio::ip::tcp::socket &&socket, Observer &observer, int cid)
    : m_socket{std::move(socket)}
    , m_observer{observer}
    , m_cid{cid}
{
    m_isReading.store(false);
    m_isWritting.store(false);
}

std::shared_ptr<TcpConnection>
TcpConnection::create(boost::asio::ip::tcp::socket &&socket, Observer &observer, int cid) noexcept
{
    return std::shared_ptr<TcpConnection>(new (std::nothrow) TcpConnection{std::move(socket), observer, cid});
}

void TcpConnection::startReading() noexcept
{
    if (!m_isReading.load())
    {
        doReadAsync();
    }
}

void TcpConnection::send(const std::string &data) noexcept
{
    std::lock_guard guard{m_writeBufGuard};
    std::ostream buf{&m_writeBuf};
    buf.write(data.data(), data.size());
    doWrite();
}
void TcpConnection::sendAsync(const std::string &data) noexcept
{
    {
        std::lock_guard guard{m_writeBufGuard};
        std::ostream buf{&m_writeBuf};
        buf.write(data.data(), data.size());
    }
    if (!m_isWritting.load())
    {
        doWriteAsync();
    }
}

void TcpConnection::close() noexcept
{
    try
    {
        m_socket.cancel();
        m_socket.close();
    }
    catch (const std::exception &e)
    {
        logger().log(LogSeverity::ERR, {"TcpConnection::close()", {"what", e.what()}, {"id", m_cid}});
        return;
    }
    m_observer.onConnectionClosed(m_cid);
}

void TcpConnection::doReadAsync() noexcept
{
    m_isReading.store(true);
    auto buffers{m_readBuf.prepare(readBufferSize)};
    auto self{shared_from_this()};
    m_socket.async_read_some(buffers, [this, self](const boost::system::error_code &error, auto bytesTransferred) {
        if (error)
        {
            logger().log(LogSeverity::ERR, {"TcpConnection::doRead()", {"code", error.value()}, {"id", m_cid}});
            return close();
        }
        m_readBuf.commit(bytesTransferred);
        m_observer.onReceived(m_cid, static_cast<const char *>(m_readBuf.data().data()), bytesTransferred);
        m_readBuf.consume(bytesTransferred);
        doReadAsync();
    });
}

void TcpConnection::doWrite() noexcept
{
    while (m_writeBuf.size() != 0)
    {
        auto bytesTransferred = m_socket.write_some(m_writeBuf.data());
        m_writeBuf.consume(bytesTransferred);
    }
}
void TcpConnection::doWriteAsync() noexcept
{
    m_isWritting.store(true);
    auto self{shared_from_this()};
    m_socket.async_write_some(m_writeBuf.data(), [this, self](const auto &error, auto bytesTransferred) {
        if (error)
        {
            logger().log(LogSeverity::ERR, {"TcpConnection::doWrite()", {"code", error.value()}, {"id", m_cid}});
            return close();
        }
        {
            std::lock_guard guard{m_writeBufGuard};
            m_writeBuf.consume(bytesTransferred);
            if (m_writeBuf.size() == 0)
            {
                m_isWritting.store(false);
                return;
            }
            doWrite();
        }
    });
}