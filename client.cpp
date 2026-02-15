#include "client_in_client.hpp"

using namespace std;

int main()
{
    string name;
    cout << "Enter your name: ";
    getline(cin, name);

    try
    {
        PokerClient client;
        client.connect_to("127.0.0.1", "12345");
        client.join_us(name);
        client.start();
    }
    catch (exception &e)
    {
        cerr << "Error: " << e.what() << endl;
    }
}
