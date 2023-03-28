#ifndef UNICODE
#define UNICODE
#endif

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <WinSock2.h>
#include <ws2tcpip.h>

#include <stdio.h>

int printdata(u_char *data, int data_len);
void CALLBACK onWSAReceiveFrom(
  IN DWORD dwError, 
  IN DWORD cbTransferred, 
  IN LPWSAOVERLAPPED lpOverlapped, 
  IN DWORD dwFlags
);

typedef struct {
    WSAOVERLAPPED overlapped;
    SOCKET sock;
    struct sockaddr_in RecvAddr;
    struct sockaddr_in SenderAddr;
    int SenderAddrSize;
    char buf[1024];
    WSABUF RecvBuf;
    DWORD flags;
} OVL_DHCP;

int start_WSARecvFrom(OVL_DHCP* ovl_dhcp)
{
    DWORD numberOfBytesReceived;
    if (WSARecvFrom(
            ovl_dhcp->sock
        , &(ovl_dhcp->RecvBuf)
        , 1 // BufferCount
        , &numberOfBytesReceived
        , &(ovl_dhcp->flags)
        , (SOCKADDR*)&(ovl_dhcp->SenderAddr)
        , &(ovl_dhcp->SenderAddrSize)
        , &(ovl_dhcp->overlapped)
        , onWSAReceiveFrom 
        ) == 0 )
    {
        onWSAReceiveFrom(0, numberOfBytesReceived, &ovl_dhcp->overlapped, ovl_dhcp->flags);
    }
    else if ( WSAGetLastError() != WSA_IO_PENDING )
    {
        printf("WSARecvFrom error: %d\n", WSAGetLastError());
    }
    else {
        
        return 0;
    }
    return WSAGetLastError();
}

void CALLBACK onWSAReceiveFrom(
  IN DWORD dwError, 
  IN DWORD cbTransferred, 
  IN LPWSAOVERLAPPED lpOverlapped, 
  IN DWORD dwFlags
) {
    OVL_DHCP* ovl_dhcp = (OVL_DHCP*)lpOverlapped;
    printdata((unsigned char*)(ovl_dhcp->RecvBuf.buf), cbTransferred);
    start_WSARecvFrom(ovl_dhcp);
}

int listen_udp_broadcast(OVL_DHCP ovl_dhcp[])
{
    DWORD flags = 0;


    for ( int i=0; i < 2; ++i) 
    {
        const USHORT port = 67+i;
        ovl_dhcp[i].RecvBuf.buf                   = ovl_dhcp[i].buf;
        ovl_dhcp[i].RecvBuf.len                   = sizeof(ovl_dhcp[i].buf);
        ovl_dhcp[i].RecvAddr.sin_family           = AF_INET;
        ovl_dhcp[i].RecvAddr.sin_port             = htons(port);
        ovl_dhcp[i].RecvAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
        ovl_dhcp[i].SenderAddrSize                = sizeof(SOCKADDR_IN);
        ovl_dhcp[i].flags = 0;

        if ( (ovl_dhcp[i].sock = WSASocketW(
            AF_INET
            , SOCK_DGRAM
            , IPPROTO_UDP
            , NULL
            , 0
            , WSA_FLAG_OVERLAPPED
            )) == INVALID_SOCKET ) {
            printf("E: WSASocketW (port: %d): %ld\n", ovl_dhcp[i].RecvAddr.sin_port, WSAGetLastError());
        }
        else if ( bind(
            ovl_dhcp[i].sock
            , (SOCKADDR*)&(ovl_dhcp[i].RecvAddr)
            ,       sizeof(ovl_dhcp[i].RecvAddr)
        ) != 0 ) {
            printf("E: bind (port: %d): %d\n",  ovl_dhcp[i].RecvAddr.sin_port, WSAGetLastError());
        }
        else if ( start_WSARecvFrom(&(ovl_dhcp[i]) ) != 0)
        {
            printf("E: WSARecvFrom (port: %d): %d\n",  ovl_dhcp[i].RecvAddr.sin_port, WSAGetLastError());
        }
        else
        {
            WCHAR tmp[64];
            DWORD dwAddressStringLength = sizeof(tmp) / sizeof(WCHAR);
            WSAAddressToStringW(
                    (SOCKADDR*)&(ovl_dhcp[i].RecvAddr)
                , sizeof(ovl_dhcp->RecvAddr)
                , NULL
                , tmp
                , &dwAddressStringLength
            );
            printf("I: WSARecvFrom listening to: %S/udp\n", tmp);
        }
    }

    return WSAGetLastError();
}

int wmain(int argc, wchar_t* argv[])
{
    WSADATA wsaData;
    int err = 0;

    OVL_DHCP ovl_dhcp[2];
    ZeroMemory(ovl_dhcp, sizeof(ovl_dhcp));

    if ( (err = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0) {
        printf("WSAStartup failed with error: %d\n", err);
    }
    else if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
        /* Tell the user that we could not find a usable */
        /* WinSock DLL.                                  */
        printf("Could not find a usable version of Winsock.dll\n");
    }
    else {
        listen_udp_broadcast(ovl_dhcp);
        SleepEx(INFINITE, TRUE);
    }
    
    WSACleanup();
    return err;
}