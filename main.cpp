#include "game.hpp"

void startClient()
{
    try
    {
        Game game;
        game.start();
    }
    catch (exception &e)
    {
        std::cerr << "Error: " << e.what() << endl;
    }
}
void startServer()
{
    try
    {
        Server server;
        server.start();
    }
    catch (exception &e)
    {
        std::cerr << "Error: " << e.what() << endl;
    }
}

int main(int argc, char *argv[])
{
    if (argc > 1)
    {
        string mode = argv[1];
        if (mode == "--client")
        {
            startClient();
            return 0;
        }
        else if (mode == "--server")
        {
            startServer();
            return 0;
        }
    }
    startClient(); // default
}
