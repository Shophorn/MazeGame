/*=============================================================================
Leo Tamminen

Windows platform layer's network things for :MAZEGAME:
=============================================================================*/
#include <ws2tcpip.h>

/*
Todo(Leo):
	- Properly check and react to network status changes in each of these
	functions.
*/
struct WinApiNetwork
{
	/*
	Todo(Leo):
		Store the connected socket's address, so we can try to reach them again in
		case connection fails mid game.
	*/
	bool32 isConnected;
	bool32 isListening;

	SOCKET connectedSocket;
	SOCKET listeningSocket;	
};

constexpr char NETWORK_LOCAL_HOST [] = "127.0.0.1";
global_variable const char * networkOwnIpAddress = NETWORK_LOCAL_HOST;
global_variable const char * networkOtherIpAddress = NETWORK_LOCAL_HOST;
global_variable bool32 networkIsRuined = false;

namespace winapi
{
	internal WinApiNetwork
	CreateNetwork();

	internal void 
	NetworkReceive(WinApiNetwork * network, PlatformNetworkPackage * resultAddress);

	internal void
	NetworkSend(WinApiNetwork * network, PlatformNetworkPackage * packageToSend);

	internal void
	NetworkListen(WinApiNetwork * network);

	internal void
	CloseNetwork(WinApiNetwork * network);
}

internal WinApiNetwork
winapi::CreateNetwork()
{
    // Todo(Leo): Somehow use WinApiLog function for these results too
    using network_result = int;

    WinApiNetwork network = {};

    /*
    Note(Leo): outline of what happens here
        - Initialize winsocket
        - Open socket and try to connect
            On success, move on
            On failure:
                close connection socket
                open listening socket to wait for someone to connect us
    */

    constexpr u_long NON_BLOCKING_MODE = 1;

    network_result RESULT;
    WSAData winsocketData;
    WORD winsocketDllVersion = MAKEWORD(2, 1);
    WSAStartup(winsocketDllVersion, &winsocketData);

    network.connectedSocket = socket(AF_INET, SOCK_STREAM, 0);

    SOCKADDR_IN ownAddress;
    Assert(inet_pton(AF_INET, networkOwnIpAddress, &ownAddress.sin_addr.s_addr));
    // ownAddress.sin_addr.s_addr = inet_addr(networkOwnIpAddress); 
    ownAddress.sin_family = AF_INET;
    ownAddress.sin_port = htons(444);
    s32 ownAddressLength = sizeof(ownAddress);

    SOCKADDR_IN otherAddress;
    Assert(inet_pton(AF_INET, networkOtherIpAddress, &otherAddress.sin_addr.s_addr));
    // otherAddress.sin_addr.s_addr = inet_addr(networkOtherIpAddress); 
    otherAddress.sin_family = AF_INET;
    otherAddress.sin_port = htons(444);
    s32 otherAddressLength = sizeof(otherAddress);            

    RESULT = connect(network.connectedSocket, (PSOCKADDR)&otherAddress, otherAddressLength);
    if (RESULT == 0)
    {
        // TODO(Leo): IMPORTANT super hacky wacky way to play with two controllers locally and with single remotely
        if (strcmp(networkOwnIpAddress, networkOtherIpAddress) == 0)
        {
            globalXinputControllerIndex = 1;
        }
        else
        {
            globalXinputControllerIndex = 0;
        }

        // connect() was succedful
        network.isConnected = true;
        u_long mode = NON_BLOCKING_MODE;
        ioctlsocket(network.connectedSocket, FIONBIO, &mode);

        log_network(1, "Connected on startup");
    }
    else
    {
        globalXinputControllerIndex = 0;

        network.isListening = true;

        // connect() failed, start listening
        closesocket(network.connectedSocket);

        network.listeningSocket = socket(AF_INET, SOCK_STREAM, 0);
        bind(network.listeningSocket, (PSOCKADDR)&ownAddress, ownAddressLength);

        /* Note(Leo); This controls number of possible connections to listen/accept/?? at a time
        'SOMAXCONN' is a special constant value that sets this to "max reasonable", in the range
        of hundreds on windows at least*/
        int backlogCount = 10; 
        listen(network.listeningSocket, backlogCount);

        u_long mode = NON_BLOCKING_MODE;
        ioctlsocket(network.listeningSocket, FIONBIO, &mode);

        log_network(1, "Listening for incoming connections");
    }

    return network;
}

internal void
winapi::NetworkSend(WinApiNetwork * network, PlatformNetworkPackage * packageToSend)
{
	if (network->isConnected == false)
	{
		log_network(1, "Trying to send but we are not connected.");
		return;
	}


    int RESULT;

    char * outData = reinterpret_cast<char *>(packageToSend);
    RESULT = send(network->connectedSocket, outData, NETWORK_PACKAGE_SIZE, 0);
}

internal void
winapi::NetworkReceive(WinApiNetwork * network, PlatformNetworkPackage * resultAddress)
{
	if (network->isConnected == false)
	{
		log_network(1, "We are trying to receive but are not connected.");
		return;
	}

	PlatformNetworkPackage package;
   	char * inData = reinterpret_cast<char *>(resultAddress);
    int RESULT = recv(network->connectedSocket, inData, NETWORK_PACKAGE_SIZE, 0);
    if (RESULT == NETWORK_PACKAGE_SIZE)
    {
        // Note(Leo): this means success
    }
    else
    {
        if (RESULT == 0)
        {
            log_network(1, "Other side disconnected");
            closesocket(network->connectedSocket);
            network->isConnected = false;
        }
        else
        {
            auto error = WSAGetLastError();
            
            switch (error)
            {
                case WSAEWOULDBLOCK:
                    // Note(Leo): this is okay, it means there was nothing to receive
                    break;

                case WSAECONNABORTED:
                    // Todo(Leo): This shouldn't occur when we exit other side
                    log_network(0, "RECEIVE error (handled): ", WinSocketErrorString(error), " (", error, ")");
                    RESULT = shutdown(network->connectedSocket, SD_BOTH);
                    RESULT = closesocket(network->connectedSocket);
                        
                    network->isConnected = false;

                    break;

                case WSAECONNRESET:
                    // Todo(Leo): This shouldn't occur when we exit other side
                    log_network(0, "RECEIVE error (handled): ", WinSocketErrorString(error), " (", error, ")");
                    RESULT = shutdown(network->connectedSocket, SD_BOTH);
                    RESULT = closesocket(network->connectedSocket);
                        
                    network->isConnected = false;
                    
                    break;


                default:
                    log_network(0, "RECEIVE error: ", WinSocketErrorString(error), " (", error, ")");
                    networkIsRuined = true;
                    break;
            }
        }
    }
}

internal void
winapi::NetworkListen(WinApiNetwork * network)
{
    if (network->isListening == false)
    {
    	log_network(1, "We are trying to listen but we are not listening.");
    	return;
    }

    int RESULT;
    network->connectedSocket = accept(network->listeningSocket, nullptr, nullptr);

    if (network->connectedSocket != INVALID_SOCKET)
    {
        network->isConnected = true;
        network->isListening = false;
        
        // Todo(Leo): if this does not close properly, try again later
        RESULT = closesocket(network->listeningSocket);
    }
    else
    {
        RESULT = WSAGetLastError();
        if (RESULT != WSAEWOULDBLOCK)
        {
            log_network(1, "Listening socket accept error: ", WinSocketErrorString(RESULT), "(", RESULT, ")");
            networkIsRuined = true;
        }
    }
}

internal void
winapi::CloseNetwork(WinApiNetwork * network)
{
    /* Note(Leo): sockets return sometimes meaningful values, these stand for reminder
    to handle those at some point

    Todo(Leo): handle results */
    s32 RESULT;
    RESULT = shutdown(network->connectedSocket, SD_BOTH);
    RESULT = closesocket(network->connectedSocket);

    RESULT = shutdown(network->listeningSocket, SD_BOTH);
    RESULT = closesocket(network->listeningSocket);
    
    RESULT = WSACleanup();

    *network = {};

    log_network(0, "Shut down");
}
