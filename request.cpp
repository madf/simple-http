#include "request.h"

Request::Request(const std::string& start_str)
{
    m_verb = start_str.substr(0, 3);
    m_path = start_str.substr(4, start_str.length() - 4 - 9);
    m_version = start_str.substr(start_str.length() - 8);
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
