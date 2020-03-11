#include <boost/asio.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <functional> // std::bind
#include <algorithm> // std::search
#include <ctime>

using boost::asio::ip::tcp;
using boost::system::error_code;

void reference()
{
    std::cout << "Usage:\n";
    std::cout << "server [-a/--address <bind-address][-h/--help][-v/--version]\n\n";
/*    std::cout << "server [-a/--address <bind-address>][-d/--dir <work-dir>][-o/--out-log <log-file-name>][-h/--help][-v/--version]\n\n";

    std::cout << "-d, --dir <work-dir> - specifies the working directory with files for the server;\n";*/
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

size_t read_complete(char* buff, const error_code& err, size_t bytes)
{
    if ( err) return 0;
    const std::string str = "\r\n\r\n";
    const bool found = std::search(buff, buff + bytes, str.begin(), str.end()) != buff + bytes;
    return found ? 0 : 1;
}

int main(int argc, char* argv[])
{
    const std::string version = "1.1.0";
    std::string address;
    std::string outfile;

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

        char buff[1024];

        std::ofstream fout;

        for (;;)
        {
            tcp::socket socket(io_service);
            acceptor.accept(socket);

            const size_t bytes = read(socket, boost::asio::buffer(buff), std::bind(read_complete, buff, std::placeholders::_1, std::placeholders::_2));

            const std::string msg(buff, bytes);

            const size_t str_end_pos = msg.find("\r");
            const std::string start_str = msg.substr(0, str_end_pos);
            std::string http_version = start_str.substr(str_end_pos - 3);
            std::string error_message;

            if (start_str.substr(0, 3) != "GET")
                error_message = "405 Method not allowed\n";
            if (http_version != "1.1" && http_version != "1.0")
                error_message += "505 HTTP Version Not Supported\n";

            boost::system::error_code ignored_error;

            if (!error_message.empty())
                boost::asio::write(socket, boost::asio::buffer(error_message), ignored_error);

            const std::string message = make_daytime_string();

            boost::asio::write(socket, boost::asio::buffer(message), ignored_error);

            std::string log_message = message + " " + socket.remote_endpoint().address().to_string() + " " + start_str;
            if (!outfile.empty())
            {
                fout.open(outfile, std::ios::app);
                fout << log_message << "\n";
                fout.close();
            }
            else
            {
                std::cout << log_message << "\n";
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << "\n";
    }
    return 0;
}
