#ifndef REQUEST_H
#define REQUEST_H

#include <string>

class Request
{
    public:
        Request(const std::string& start_str);
        const std::string& verb() const;
        const std::string& path() const;
        const std::string& version() const;

    private:
        std::string m_verb;
        std::string m_path;
        std::string m_version;
};

#endif
