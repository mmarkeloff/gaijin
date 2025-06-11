#pragma once

#include <memory>
#include <atomic>

#include <boost/asio.hpp>

class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{
public:
    struct Observer
    {
        virtual void onReceived(int cid, const char *data, size_t size);
        virtual void onConnectionClosed(int cid);
    };

public:
    static std::shared_ptr<TcpConnection>
    create(boost::asio::ip::tcp::socket &&socket, Observer &observer, int cid = 0) noexcept;

    void startReading() noexcept;

    void send(const std::string &data) noexcept;
    void sendAsync(const std::string &data) noexcept;

    void close() noexcept;

private:
    TcpConnection(boost::asio::ip::tcp::socket &&socket, Observer &observer, int cid);

    void doReadAsync() noexcept;

    void doWrite() noexcept;
    void doWriteAsync() noexcept;

private:
    boost::asio::ip::tcp::socket m_socket;

    boost::asio::streambuf m_readBuf;
    std::mutex m_writeBufGuard;
    boost::asio::streambuf m_writeBuf;

    Observer &m_observer;

    std::atomic_bool m_isReading;
    std::atomic_bool m_isWritting;

    int m_cid;
};
