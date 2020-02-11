#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <ctime>

using boost::asio::ip::tcp;

void reference()
{
    std::cout << "Usage:\n";
    std::cout << "server [-a/--address <bind-address][-h/--help][-v/--version]\n\n";
/*    std::cout << "server [-a/--address <bind-address>][-d/--dir <work-dir>][-o/--out-log <log-file-name>][-h/--help][-v/--version]\n\n";

    std::cout << "-d, --dir <work-dir> - specifies the working directory with files for the server;\n";
    std::cout << "-o, --out-log <log-file-name> - specifies the file name for logging for the server;\n";*/
    std::cout << "-a, --address <bind-address> - can be specified in <address>[:<port>] form, where <address> can be IP-address or domain name, <port> - number;\n";
    std::cout << "-v, --version - server version;\n";
    std::cout << "-h, --help - show this text.\n";
}

std::string make_daytime_string()
{
    time_t now = time(nullptr);
    return ctime(&now);
}

int main(int argc, char* argv[])
{
    const std::string version = "1.0.0";
    std::string address;

    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];

        if (arg == "-a" || arg == "--address")
        {
            if (i + 1 == argc)
            {
                std::cerr << argv[i] << " needs and argument - an address.\n";
                return 1;
            }
            address = argv[++i];
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
        size_t pos = address.find(":");
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
        tcp::resolver::query query(host, port);
        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
        tcp::endpoint ep = *endpoint_iterator;

        tcp::acceptor acceptor(io_service, ep);

        for (;;)
        {
            tcp::socket socket(io_service);
            acceptor.accept(socket);

            std::string message = make_daytime_string();

            boost::system::error_code ignored_error;
            boost::asio::write(socket, boost::asio::buffer(message), ignored_error);
            std::cout << message << "\n";

            tcp::endpoint remote_ep = socket.remote_endpoint();
            boost::asio::ip::address remote_ad = remote_ep.address();
            std::string s = remote_ad.to_string();

            std::cout << s << "\n";
        }
    }

    catch (std::exception& e)
    {
        std::cerr << e.what() << "\n";
    }

    return 0;
}
