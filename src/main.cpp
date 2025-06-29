#include"../inc/webserver.h"

int main( int argc, char* argv[] ) {
    int port=8000;
    if( argc > 1 ) {
        port = atoi(argv[1]);
    }
    // 触发组合模式,默认listenfd LT + connfd LT
    int TRIGMode = 1;
    webserver server(port,TRIGMode);

    server.start();
    return 0;
}