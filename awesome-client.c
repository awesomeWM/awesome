#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "util.h"

#define CONTROL_UNIX_SOCKET_PATH ".awesome_so_ctl"

int
main()
{
    ssize_t path_len;
    struct sockaddr_un addr;
    char buf[1024];
    const char *homedir;
    int csfd;
    
    homedir = getenv("HOME");
    path_len = a_strlen(homedir) + a_strlen(CONTROL_UNIX_SOCKET_PATH) + 2;
    if(path_len >= (int)sizeof(addr.sun_path))
    {
        fprintf(stderr, "error: path of control UNIX domain socket is too long");
        exit(1);
    }
    a_strcpy(addr.sun_path, path_len, homedir);
    a_strcat(addr.sun_path, path_len, "/");
    a_strcat(addr.sun_path, path_len, CONTROL_UNIX_SOCKET_PATH);
    csfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if(csfd < 0)
        perror("error opening UNIX domain socket");
    addr.sun_family = AF_UNIX;
    while(fgets(buf, sizeof(buf), stdin))
    {
        if(sendto(csfd, buf, strlen(buf), MSG_NOSIGNAL, (struct sockaddr*)&addr, sizeof(addr)) == -1)
            perror("error sending datagram");
    }
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
