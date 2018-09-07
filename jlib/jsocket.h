/*
 * jsocket.h
 *
 * @chuanjiong
 */

#ifndef _J_SOCKET_H_
#define _J_SOCKET_H_

#include "jcommon.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef int jsocket;

int jsocket_setup(void);

void jsocket_shutdown(void);

void jsocket_close(jsocket sock);

int jsocket_set_nonblock(jsocket sock);

// tcp

jsocket tcp_listen(int port);

jsocket tcp_accept(jsocket sock);

jsocket tcp_connect(const char *ip, int port);

int tcp_write(jsocket sock, uint8_t *buf, int32_t size);

int tcp_read(jsocket sock, uint8_t *buf, int32_t size);

// udp

jsocket udp_bind(int port);

jsocket udp_bind_local_remote(int local_port, const char *remote_ip, int remote_port);

jsocket udp_connect(const char *ip, int port);

int udp_write(jsocket sock, uint8_t *buf, int32_t size);

int udp_read(jsocket sock, uint8_t *buf, int32_t size);

int udp_read_2(jsocket sock, uint8_t *buf, int32_t size, char *ip, int *port);

#ifdef __cplusplus
}
#endif

#endif //_J_SOCKET_H_


