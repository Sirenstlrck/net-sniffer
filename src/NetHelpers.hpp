#pragma once

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>

#include <arpa/inet.h>
#include <sys/socket.h>

#include "ArgumentsParsing.hpp"

namespace SimpleSniffer
{
static void CloseSocket(int socket)
{
    int closeResult = close(socket);
    if (closeResult == -1)
    {
        std::string errnoStr(std::strerror(errno));
        throw std::runtime_error{ "Failed to close socket: " + errnoStr };
    }
}

static int CreateRawSocket(int protocol)
{
    int sock = socket(AF_INET, SOCK_RAW, protocol);
    if (sock < 0)
    {
        std::string errnoStr(std::strerror(errno));
        throw std::runtime_error{ "Failed to create socket: " + errnoStr };
    }
    return sock;
}

static std::vector<int> CreateRawSockets(InetProtocols protocols, std::optional<uint32_t> port)
{
    std::vector<int> result;
    if ((protocols & InetProtocols::Udp) != 0)
    {
        int sock = CreateRawSocket(IPPROTO_UDP);
        result.push_back(sock);
    }
    if ((protocols & InetProtocols::Tcp) != 0)
    {
        int sock = CreateRawSocket(IPPROTO_TCP);
        result.push_back(sock);
    }

    return result;
}

static void FillSet(fd_set &set, const std::vector<int> &sockets)
{
    FD_ZERO(&set);
    for (int socket : sockets)
    {
        FD_SET(socket, &set);
    }
}

static void
FillVectorFromSet(fd_set &set, const std::vector<int> &full, std::vector<int> &destination)
{
    destination.clear();
    for (int sock : full)
    {
        if (FD_ISSET(sock, &set))
            destination.push_back(sock);
    }
}

static int Select(fd_set *readSet, fd_set *errorSet)
{
    // I hate select
    constexpr int Nfds = 1023;
    return select(Nfds, readSet, nullptr, errorSet, nullptr);
}

static int Select(
    const std::vector<int> &sockets,
    std::vector<int>       &outReadSockets,
    std::vector<int>       &outErrorSockets)
{
    fd_set readSet, errorSet;
    FillSet(readSet, sockets);
    FillSet(errorSet, sockets);
    int selectRes = Select(&readSet, &errorSet);
    FillVectorFromSet(readSet, sockets, outReadSockets);
    FillVectorFromSet(errorSet, sockets, outErrorSockets);
    return selectRes;
}
} // namespace SimpleSniffer