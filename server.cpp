#include "request.h"
#include <boost/asio.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <functional> // std::bind
#include <algorithm> // std::search
#include <ctime>
#include <dirent.h> //struct dirent, opendir, readdir, closedir
#include <sys/stat.h> //stat, struct stat
#include <time.h> //localtime, asctime

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

std::string make_message(DIR *dir, const std::string& path, const std::string& date)
{
        std::string line;

        for (struct dirent *entry = readdir(dir); entry != NULL; entry = readdir(dir))
        {
            if (strcmp(".", entry->d_name) && strcmp("..", entry->d_name))
            {
                const std::string file_name = entry->d_name;

                struct stat st;
                if (stat((path + "/" + file_name).c_str(), &st) < 0)
                {
                    line = line + "<tr><td>" + file_name + "</td><td>?</td><td>?</td></tr>";
                }
                else
                {
                    std::string file_date = asctime(localtime(&st.st_ctime));
                    line = line + "<tr><td><p><a href=\"" + file_name + "\">" + file_name + "</a></p></td><td>" + std::to_string(st.st_size) + "</td><td>" + file_date + "</td></tr>";
                }
            }
        }
        closedir(dir);

        const std::string table_html ="<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\"> \
            <html> \
            <body> \
            <table border=\"1\" cellspacing=\"0\" cellpadding=\"5\"> \
            <tr><td>File name</td><td>File size</td><td>Last modified date</td></tr>" + line + " \
            </table> \
            </body> \
            </html>";

        return "HTTP/1.1 200 OK\r\nHost: localhost\r\nContent-Type: text/html; charset=utf-8\r\n\r\n" + date + "\r\n" + table_html;
}

void write_file(tcp::socket& socket, const std::string& request_path_file, const std::string& path, error_code ignored_error)
{
    std::ifstream fin(path + "/" + request_path_file);

    char buff[1024] = {0};
    while (!fin.eof())
    {
        fin.read(buff, 1024);

        std::streamsize len = fin.gcount();
        if (len > 0)
        {
            std::string msg(buff, len);
            boost::asio::write(socket, boost::asio::buffer(msg), ignored_error);
        }
    }
}

void write_response(tcp::socket& socket, const Request& request, const std::string& date, const std::string& work_dir)
{
    std::string error_message;

    if (request.verb() != "GET")
        error_message = "HTTP/1.1 405 Method not allowed\r\n";
    if (request.version() != "HTTP/1.1" && request.version() != "HTTP/1.0")
        error_message += "HTTP/1.1 505 HTTP Version Not Supported\r\n";

    error_code ignored_error;

    if (!error_message.empty())
    {
        boost::asio::write(socket, boost::asio::buffer(error_message), ignored_error);
    }
    else
    {
        std::string path;

        if (!work_dir.empty())
            path = work_dir;
        else
            path = ".";

        DIR *dir = opendir(path.c_str());
        if (dir == NULL)
        {
            error_message = "HTTP/1.1 500 Directory failed to open\r\n";
            boost::asio::write(socket, boost::asio::buffer(error_message), ignored_error);
        }
        else
        {
            boost::asio::write(socket, boost::asio::buffer(make_message(dir, path, date)), ignored_error);

            const std::string request_path_file = request.path();
            if (request_path_file != "/")
                write_file(socket, request_path_file, path, ignored_error);
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

            const std::string date = make_daytime_string();

            write_response(socket, Request(start_str), date, work_dir);

            write_log(outfile, date + " " + socket.remote_endpoint().address().to_string() + " " + start_str);
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << "\n";
    }
    return 0;
}
