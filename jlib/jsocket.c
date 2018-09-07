/*
 * jsocket.c
 *
 * @chuanjiong
 */

#include "jsocket.h"
#include "jatomic.h"

static volatile int jsocket_number = 0;

int jsocket_setup(void)
{
    jsocket_number = 0;
    return SUCCESS;
}

void jsocket_shutdown(void)
{
    if (jatomic_get(&jsocket_number))
        jerr("[jsocket] %d socket do not closed\n", jatomic_get(&jsocket_number));
}

void jsocket_close(jsocket sock)
{
    if (sock >= 0)
    {
        jclose(sock);
        jatomic_sub(&jsocket_number, 1);
    }
}

int jsocket_set_nonblock(jsocket sock)
{
    if (sock < 0)
        return ERROR_FAIL;

    int flag;

    if ((flag=fcntl(sock, F_GETFL, 0)) < 0)
        return ERROR_FAIL;

    flag |= O_NONBLOCK;

    if (fcntl(sock, F_SETFL, flag) < 0)
        return ERROR_FAIL;

    return SUCCESS;
}

// tcp

jsocket tcp_listen(int port)
{
    jsocket sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        return -1;

    jatomic_add(&jsocket_number, 1);

    int flag = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

    jsocket_set_nonblock(sock);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        jsocket_close(sock);
        return -1;
    }

    listen(sock, MAX_LISTEN_Q);

    return sock;
}

jsocket tcp_accept(jsocket sock)
{
    if (sock < 0)
        return -1;

    struct sockaddr_in addr;
    socklen_t addrLen = sizeof(struct sockaddr_in);

    jsocket asock = accept(sock, (struct sockaddr *)&addr, &addrLen);
    if (asock < 0)
        return -1;

    int keep_alive = 1;
    int keep_idle = 3;
    int keep_interval = 1;
    int keep_count = 3;
    setsockopt(asock, SOL_SOCKET, SO_KEEPALIVE, &keep_alive, sizeof(keep_alive));
    setsockopt(asock, IPPROTO_TCP, TCP_KEEPIDLE, &keep_idle, sizeof(keep_idle));
    setsockopt(asock, IPPROTO_TCP, TCP_KEEPINTVL, &keep_interval, sizeof(keep_interval));
    setsockopt(asock, IPPROTO_TCP, TCP_KEEPCNT, &keep_count, sizeof(keep_count));

    jsocket_set_nonblock(asock);

    jatomic_add(&jsocket_number, 1);

    return asock;
}

jsocket tcp_connect(const char *ip, int port)
{
    if ((ip==NULL) || (port<0))
        return -1;

    jsocket sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        return -1;

    jatomic_add(&jsocket_number, 1);

    jsocket_set_nonblock(sock);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        if (errno != EINPROGRESS)
        {
            jsocket_close(sock);
            return -1;
        }
        else
        {
            fd_set wfds;
            FD_ZERO(&wfds);
            FD_SET(sock, &wfds);
            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 200000;
            int ret = select(sock+1, NULL, &wfds, NULL, &timeout);
            if (ret <= 0)
            {
                jsocket_close(sock);
                return -1;
            }
        }
    }

    int keep_alive = 1;
    int keep_idle = 3;
    int keep_interval = 1;
    int keep_count = 3;
    setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keep_alive, sizeof(keep_alive));
    setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keep_idle, sizeof(keep_idle));
    setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keep_interval, sizeof(keep_interval));
    setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keep_count, sizeof(keep_count));

    return sock;
}

int tcp_write(jsocket sock, uint8_t *buf, int32_t size)
{
    if ((sock<0) || (buf==NULL) || (size<=0))
        return -1;

    int wsize = 0;
    while (size > wsize)
    {
        int w = send(sock, (const char *)(buf+wsize), size-wsize, 0);
        if ((w<=0) && (errno!=EINTR) && (errno!=EAGAIN))
            return -1;
        if (w > 0)
            wsize += w;
    }

    return wsize;
}

int tcp_read(jsocket sock, uint8_t *buf, int32_t size)
{
    if ((sock<0) || (buf==NULL) || (size<=0))
        return -1;

    return recv(sock, (char *)buf, size, 0);
}

// udp

jsocket udp_bind(int port)
{
    if (port < 0)
        return -1;

    jsocket sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
        return -1;

    //int flag = 1;
    //setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

    jsocket_set_nonblock(sock);

    jatomic_add(&jsocket_number, 1);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        //jerr("[jsocket] udp_bind: udp bind fail: %d\n", errno);
        jsocket_close(sock);
        return -1;
    }

    return sock;
}

jsocket udp_bind_local_remote(int local_port, const char *remote_ip, int remote_port)
{
    if ((local_port<0) || (remote_ip==NULL) || (remote_port<0))
        return -1;

    jsocket sock = udp_bind(local_port);
    if (sock < 0)
        return -1;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(remote_port);
    addr.sin_addr.s_addr = inet_addr(remote_ip);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        jerr("[jsocket] udp_bind_local_remote: connect fail: %d\n", errno);
        jsocket_close(sock);
        return -1;
    }

    return sock;
}

jsocket udp_connect(const char *ip, int port)
{
    if ((ip==NULL) || (port<0))
        return -1;

    jsocket sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
        return -1;

    jatomic_add(&jsocket_number, 1);

    jsocket_set_nonblock(sock);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        jerr("[jsocket] udp_connect: connect fail: %d\n", errno);
        jsocket_close(sock);
        return -1;
    }

    return sock;
}

int udp_write(jsocket sock, uint8_t *buf, int32_t size)
{
    if ((sock<0) || (buf==NULL) || (size<=0))
        return -1;

    return send(sock, (const char *)buf, size, 0);
}

int udp_read(jsocket sock, uint8_t *buf, int32_t size)
{
    if ((sock<0) || (buf==NULL) || (size<=0))
        return -1;

    return udp_read_2(sock, buf, size, NULL, NULL);
}

int udp_read_2(jsocket sock, uint8_t *buf, int32_t size, char *ip, int *port)
{
    if ((sock<0) || (buf==NULL) || (size<=0))
        return -1;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    socklen_t addrLen = sizeof(struct sockaddr_in);

    ssize_t len = recvfrom(sock, (char *)buf, size, 0, (struct sockaddr *)&addr, &addrLen);

    if (ip)
        strcpy(ip, inet_ntoa(addr.sin_addr));
    if (port)
        *port = ntohs(addr.sin_port);

    return len;
}


