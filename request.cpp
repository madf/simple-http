#include "request.h"

Request::Request(const std::string& start_str)
{
    const size_t pos_space_first = start_str.find_first_of(' ');

    m_verb = start_str.substr(0, pos_space_first);

    const size_t pos_space_second = start_str.find_last_of(' ');

    m_path = start_str.substr(pos_space_first + 1, pos_space_second - pos_space_first - 1);
    m_version = start_str.substr(pos_space_second + 1);
}

const std::string& Request::verb() const
{
    return m_verb;
}

const std::string& Request::path() const
{
    return m_path;
}

const std::string& Request::version() const
{
    return m_version;
}
