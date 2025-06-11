#include <boost/asio/signal_set.hpp>

#include "cache_svc.h"

void handler(const boost::system::error_code &error, int n)
{
    exit(0);
}

int main()
{
    boost::asio::io_context context;
    CacheService svc{context};
    boost::asio::signal_set signals(context, SIGINT);
    signals.async_wait(handler);
    context.run();
    return 0;
}
