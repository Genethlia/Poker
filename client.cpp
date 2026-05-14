#include "raylib_client.hpp"
#include "client_in_client.hpp"

using namespace std;

int main()
{
    try
    {
        Game game;
        game.start();
    }
    catch (exception &e)
    {
        cerr << "Error: " << e.what() << endl;
    }
}
