#include <stdexcept>

#include "common/proto.h"
#include "common/logger.h"
#include "common/utils.h"

static Command parse(std::string_view data, int cid)
{
    Command c;
    c.cid = cid;

    auto space = data.find(' ');
    if (space == data.npos)
    {
        throw std::runtime_error{"invalid format (no ' ')"};
    }

    if (data.starts_with(GET_COMMAND))
    {
        c.action = Action::GET;
        c.key = data.substr(space + 1);
    }
    else if (data.starts_with(SET_COMMAND))
    {
        c.action = Action::SET;
        auto equals = data.find('=');
        if (equals == data.npos)
        {
            throw std::runtime_error{"invalid format (no '=')"};
        }

        c.key = data.substr(space + 1, equals - space - 1);
        c.val = data.substr(equals + 1);
    }
    else
    {
        throw std::runtime_error{"unknown action"};
    }

    return c;
}

std::vector<Command> parseCommands(std::string_view data, int cid) noexcept
{
    auto tokens = split(data, REQUEST_DELIM);
    std::vector<Command> commands;
    commands.reserve(tokens.size());
    for (auto token : tokens)
    {
        try
        {
            commands.emplace_back(parse(token, cid));
        }
        catch (const std::runtime_error &e)
        {
            logger().log(LogSeverity::ERR, {"parseCommands()", {"what", e.what()}, {"token", token}});
        }
    }
    return commands;
}