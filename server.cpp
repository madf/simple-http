#include "request.h"
#include <boost/asio.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <functional> // std::bind
#include <algorithm> // std::search
#include <ctime>
#include <cerrno>
#include <dirent.h> //struct dirent, opendir, readdir, closedir
#include <sys/stat.h> //stat, struct stat, open
#include <sys/types.h> //open
#include <fcntl.h> //open
#include <unistd.h> //read

using boost::asio::ip::tcp;
using boost::system::error_code;

void reference()
{
    std::cout << "Usage:\n";
    std::cout << "server [-a/--address <bind-address>][-d/--dir <work-dir>][-o/--out-log <log-file-name>][-h/--help][-v/--version]\n\n";

    std::cout << "-d, --dir <work-dir> - specifies the working directory with files for the server;\n";
    std::cout << "-o, --out-log <log-file-name> - specifies the file name for logging for the server;\n";
    std::cout << "-a, --address <bind-address> - can be specified in <address>[:<port>] form, where <address> can be IP-address or domain name, <port> - number;\n";
    std::cout << "-v, --version - server version;\n";
    std::cout << "-h, --help - show this text.\n";
}

std::string make_daytime_string()
{
    char buffer[80];
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", timeinfo);
    return buffer;
}

size_t read_complete(const char* buff, const error_code& err, size_t bytes)
{
    if (err) return 0;
    const std::string str = "\r\n\r\n";
    const bool found = std::search(buff, buff + bytes, str.begin(), str.end()) != buff + bytes;
    return found ? 0 : 1;
}

void write_log(const std::string& outfile, const std::string& log_message)
{
    if (!outfile.empty())
    {
        std::ofstream fout(outfile, std::ios::app);
        fout << log_message << "\n";
    }
    else
    {
        std::cout << log_message << "\n";
    }
}

void send_string(tcp::socket& socket, const std::string& str)
{
    error_code ignored_error;
    boost::asio::write(socket, boost::asio::buffer(str), ignored_error);
}

void send_index(tcp::socket& socket, DIR *dir, const std::string& path)
{
    std::string lines;

    for (struct dirent *entry = readdir(dir); entry != NULL; entry = readdir(dir))
    {
        if (strcmp(".", entry->d_name) && strcmp("..", entry->d_name))
        {
            const std::string file_name = entry->d_name;

            struct stat st;
            if (stat((path + "/" + file_name).c_str(), &st) < 0)
            {
                lines = lines + "<tr><td>" + file_name + "</td><td>?</td><td>?</td></tr>";
            }
            else
            {
                const std::string file_date = ctime(&st.st_ctime);
                lines = lines + "<tr><td><p><a href=\"" + file_name + "\">" + file_name + "</a></p></td><td>" + std::to_string(st.st_size) + "</td><td>" + file_date + "</td></tr>";
            }
        }
    }

    const std::string table_html ="<!DOCTYPE html> \
        <html> \
        <body> \
        <table border=\"1\" cellspacing=\"0\" cellpadding=\"5\"> \
        <tr><td>File name</td><td>File size</td><td>Last modification date</td></tr>" + lines + " \
        </table> \
        </body> \
        </html>";

    std::string index =  "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n\r\n" + table_html;

    send_string(socket, index);
}

void write_file(tcp::socket& socket, const std::string& request_path_file, const std::string& path)
{
    int fd = open((path + "/" + request_path_file).c_str(), O_RDONLY);

    std::string str_response;

    if (fd == -1)
    {
        if (errno == ENOENT)
            str_response = "HTTP/1.1 404 File does not exist\r\n";
        else if (errno == EACCES)
            str_response = "HTTP/1.1 403 File access not allowed\r\n";
        else
            str_response = "HTTP/1.1 500 File open error\r\n";

        send_string(socket, str_response);
    }
    else
    {
        str_response = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Disposition: attachment\r\n\r\n";

        send_string(socket, str_response);

        char buff[1024] = {0};
        size_t len;
        error_code ignored_error;

        while ((len = read(fd, buff, 1024)) > 0)
            boost::asio::write(socket, boost::asio::buffer(buff, len), ignored_error);
    }
}

void write_response(tcp::socket& socket, const Request& request, const std::string& work_dir)
{
    std::string error_message;

    if (request.verb() != "GET")
        error_message = "HTTP/1.1 405 Method not allowed\r\n";
    if (request.version() != "HTTP/1.1" && request.version() != "HTTP/1.0")
        error_message += "HTTP/1.1 505 HTTP Version Not Supported\r\n";

    if (!error_message.empty())
    {
        send_string(socket, error_message);
    }
    else
    {
        std::string path;

        if (!work_dir.empty())
            path = work_dir;
        else
            path = ".";

        if (request.path() == "/")
        {
            DIR *dir = opendir(path.c_str());
            if (dir == NULL)
            {
                error_message = "HTTP/1.1 500 Failed to open directory\r\n";
                send_string(socket, error_message);
            }
            else
            {
            send_index(socket, dir, path);
            closedir(dir);
            }
        }
        else
        {
            write_file(socket, request.path(), path);
        }
    }
}

int main(int argc, char* argv[])
{
    const std::string version = "1.2.0";
    std::string address;
    std::string outfile;
    std::string work_dir;

    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];

        if (arg == "-a" || arg == "--address")
        {
            if (i + 1 == argc)
            {
                std::cerr << arg << " needs an argument - an address.\n";
                return 1;
            }
            address = argv[++i];
        }
        else if (arg == "-d" || arg == "--dir")
        {
            if (i + 1 == argc)
            {
                std::cerr << arg << " needs an argument - a filename.\n";
                return 1;
            }
            work_dir = argv[++i];
        }
        else if (arg == "-o" || arg == "--outfile")
        {
            if (i + 1 == argc)
            {
                std::cerr << arg << " needs an argument - a filename.\n";
                return 1;
            }
            outfile = argv[++i];
        }
        else if (arg == "-v" || arg == "--version")
        {
            std::cout << "Version " << version << "\n";
            return 0;
        }
        else if (arg == "-h" || arg == "--help")
        {
            reference();
            return 0;
        }
        else
        {
            std::cerr << "Unknown command line argument\n";
            return 1;
        }
    }

    std::string port = "80";
    std::string host;
    if (!address.empty())
    {
        const size_t pos = address.find(":");
        if (pos != std::string::npos)
        {
            host = address.substr(0, pos);
            port = address.substr(pos + 1, address.length() - pos - 1);
        }
        else
        {
            host = address;
        }
    }

    try
    {
        boost::asio::io_service io_service;

        tcp::resolver resolver(io_service);
        tcp::acceptor acceptor(io_service, *resolver.resolve(tcp::resolver::query(host, port)));

        namespace pls = std::placeholders;

        char buff[1024];

        for (;;)
        {
            tcp::socket socket(io_service);
            acceptor.accept(socket);

            const size_t bytes = read(socket, boost::asio::buffer(buff), std::bind(read_complete, buff, pls::_1, pls::_2));

            const std::string msg(buff, bytes);
            const size_t str_end_pos = msg.find('\r');
            const std::string start_str = msg.substr(0, str_end_pos);

            write_response(socket, Request(start_str), work_dir);

            write_log(outfile, make_daytime_string() + " " + socket.remote_endpoint().address().to_string() + " " + start_str);
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << "\n";
    }
    return 0;
}
