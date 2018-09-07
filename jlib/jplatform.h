/*
 * jplatform.h
 *
 * @chuanjiong
 */

#ifndef _J_PLATFORM_H_
#define _J_PLATFORM_H_

#ifdef MINGW32
    #include <windows.h>
    #include <winsock2.h>
#else
    #include <fcntl.h>
    #include <unistd.h>
    #include <netdb.h>
    #include <net/if.h>
    #include <sys/ioctl.h>
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <sys/mman.h>
    #include <sys/vfs.h>
    #include <netinet/tcp.h>
#endif

#ifdef MINGW32
    #define jsleep          Sleep
#else
    #define jsleep(t)       usleep((t)*1000)
#endif

#ifdef MINGW32
    #define jclose          closesocket
#else
    #define jclose          close
#endif

//socket

#ifdef MINGW32
    #define jlocal_ip(s)    { \
                                char name[100]; \
                                gethostname(name, sizeof(name)); \
                                hostent *host; \
                                host = gethostbyname(name); \
                                snprintf(s, IP_STRING_SIZE, "%d.%d.%d.%d", host->h_addr_list[0][0]&0xff, host->h_addr_list[0][1]&0xff, \
                                    host->h_addr_list[0][2]&0xff, host->h_addr_list[0][3]&0xff); \
                            }
#else
    #define jlocal_ip(s)    { \
                                int sock = socket(AF_INET, SOCK_DGRAM, 0); \
                                struct ifconf conf; \
                                char buf[512]; \
                                conf.ifc_len = 512; \
                                conf.ifc_buf = buf; \
                                ioctl(sock, SIOCGIFCONF, &conf); \
                                struct ifreq *ifr = conf.ifc_req; \
                                int i; \
                                for (i=conf.ifc_len/sizeof(struct ifreq); i>0; i--) \
                                { \
                                    ioctl(sock, SIOCGIFFLAGS, ifr); \
                                    if (((ifr->ifr_flags&IFF_LOOPBACK)==0) && (ifr->ifr_flags&IFF_UP)) \
                                    { \
                                        snprintf(s, IP_STRING_SIZE, "%s", \
                                            inet_ntoa(((struct sockaddr_in*)&(ifr->ifr_addr))->sin_addr)); \
                                        break; \
                                    } \
                                    ifr++; \
                                } \
                                close(sock); \
                            }
#endif

#ifdef MINGW32
typedef int socklen_t;
#endif

#endif //_J_PLATFORM_H_


