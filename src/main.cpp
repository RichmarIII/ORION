#include "OrionWebServer.hpp"

using namespace ORION;

int
main(int argc, char* argv[])
{
    OrionWebServer WebServer;
    WebServer.Start(5000);

    // Wait for the web server to stop via a call to WebServer.Stop()
    WebServer.Wait();

    return 0;
}