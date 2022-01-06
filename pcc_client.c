#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/fcntl.h>

#define SUCCESS 0
#define FAILURE 1

//gcc -O3 -D_POSIX_C_SOURCE=200809 -Wall -std=c11 pcc_client.c -o pcc_client

int main(int argc, char *argv[])
{
    int socket_fd = -1, file_fd = -1;
    unsigned int port, val;
    char *filepath, *serv_ip;
    int err, retval, totalsent=0, notwritten=0, bytes_sent=0, bytes_read, not_read, totalread;
    struct sockaddr_in serv_addr;
    unsigned int N, C;
    char *buff_recv, *buff_send;
    struct stat statbuf;

    /* Argument validation, conversion and processing */

    if (argc != 4)
    {
        perror("Error: bad number of arguments.\n");
        exit(FAILURE);
    }

    serv_ip = argv[1];
    filepath = argv[3];
    port = strtol(argv[2],NULL,10); //verifying port argument validty
    err = errno;
    if (err != 0)
    {
        perror(strerror(err));
        exit(FAILURE);
    }

    /* Initializing TCP client socket */
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0)
    {
        err = errno;
        perror(strerror(err));
        exit(FAILURE);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, serv_ip, &serv_addr.sin_addr.s_addr);

    retval = stat(filepath, &statbuf);
    if (retval != SUCCESS)
    {
        err = errno;
        perror(strerror(err));
        exit(FAILURE);
    }
    N = statbuf.st_size; //getting size of file

    file_fd = open(filepath, O_RDONLY);
    if (file_fd < 0)
    {
        err = errno;
        perror(strerror(err));
        exit(FAILURE);
    }


    val = htonl(N);
    buff_send = (char*)&val;


    retval = connect(socket_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)); //connecting to server
    if (retval < 0)
    {
        err = errno;
        perror(strerror(err));
        exit(FAILURE);
    }

    totalsent = 0;
    notwritten = sizeof(unsigned int); //buffer size: size of N which is an unsigned int
    while (notwritten > 0)
    {
        bytes_sent = write(socket_fd, buff_send+totalsent, notwritten); //writing N to server
        if (bytes_sent < 0)
        {
            err = errno;
            perror(strerror(err));
            exit(FAILURE);
        }
        /* updating offsets for pointers */
        totalsent += bytes_sent;
        notwritten -= bytes_sent;
    }


    buff_send = (char*)malloc(N);
    if (buff_send == NULL)
    {
        err = errno;
        perror(strerror(err));
        exit(FAILURE);
    }

    bytes_read = 0;
    totalread = 0;
    not_read = N; //total size to read
    while (not_read > 0)
    {
        bytes_read = read(file_fd, buff_send + totalread, not_read); //N is the size of the file, so it will be read in its whole
        if (bytes_read < 0)
        {
            err = errno;
            perror(strerror(err));
            exit(FAILURE);
        }

        /* updating offsets for pointers */
        totalread += bytes_read;
        not_read -= bytes_read;

    }

    totalsent = 0;
    notwritten = N; 
    bytes_sent = 0;
    while (notwritten > 0)
    {
        bytes_sent = write(socket_fd, buff_send+totalsent, notwritten); //writing file to server
        if (bytes_sent < 0)
        {
            err = errno;
            perror(strerror(err));
            exit(FAILURE);
        }
        /* updating offsets for pointers */
        totalsent += bytes_sent;
        notwritten -= bytes_sent;
    }

    buff_recv = (char*)malloc(sizeof(unsigned int));
    if (buff_recv == NULL)
    {
        err = errno;
        perror(strerror(err));
        exit(FAILURE);
    }

    bytes_read = 0;
    totalread = 0;
    not_read = sizeof(unsigned int);
    while (not_read > 0)
    {
        bytes_read = read(socket_fd, buff_recv + totalread, not_read); //reading N from server
        if (bytes_read < 0)
        {
            err = errno;
            perror(strerror(err));
            exit(FAILURE);
        }
        /* updating offsets for pointers */
        totalread += bytes_read;
        not_read -= bytes_read;
    }
    

    C = ntohl(*(int*)buff_recv);

    printf("# of printable characters: %u\n",C);

    exit(SUCCESS);

}