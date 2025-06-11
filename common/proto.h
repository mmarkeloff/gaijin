#pragma once

#include <string_view>
#include <optional>
#include <vector>
#include <optional>
#include <string>

enum class Action
{
    GET,
    SET
};

template <typename K, typename V>
struct CommandTempl
{
    int cid;
    Action action;
    K key; // must be set for Action::SET Action::GET
    std::optional<V> val; // must be set for Action::SET
};

using Key = std::string;
using Value = std::string;
using Command = CommandTempl<Key, Value>;

static constexpr const char *REQUEST_DELIM = "$";
static constexpr const char *RESPONSE_DELIM = "+";
static constexpr const char *GET_COMMAND = "get"; 
static constexpr const char *SET_COMMAND = "set"; 

// possible loss data when data='$set cat=4$get'
// TODO: save tail='get' for each client and append it to beginning of next transport data portion
std::vector<Command> parseCommands(std::string_view data /* $set cat=4$get cat */, int cid) noexcept;
