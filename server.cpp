#include <iostream>
#include <cstring>

void reference()
{
    std::cout << "Usage:\n";
    std::cout << "server [-a/--address <bind-address>][-d/--dir <work-dir>][-o/--out-log <long-file-name>][-h/--help][-v/--version]\n\n";

    std::cout << "-a, --address <bind-address> - can be specified in <address>[:<port>] form, where <address> can be IP-address or domain name, <port> - number;\n";
    std::cout << "-d, --dir <work-dir> - specifies the working directory with files for the server;\n";
    std::cout << "-o, --out-log <long-file-name> - specifies the file name for logging for the server;\n";
    std::cout << "-v, --version - server version;\n";
    std::cout << "-h, --help - show this text.\n";
}

int main(int argc, char* argv[])
{
    std::string version = "1.0.0";

    for (int i = 1; i < argc; ++i)
    {
        if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--version"))
        {
            std::cout << "Version " << version << "\n";
            return 0;
        }
        else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help"))
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
    return 0;
}
