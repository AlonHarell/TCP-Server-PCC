#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>

#define SUCCESS 0
#define FAILURE 1

#define NOT_RECIEVED 1
#define RECIEVED 0

#define printable_LOW 32
#define printable_HIGH 126

void my_SIGINT_handler(int signum);
int register_SIGINT_handler();
void print_pcc();

int sigint = NOT_RECIEVED;
int accepted = NOT_RECIEVED;
unsigned int pcc_total[printable_HIGH+1];

//gcc -O3 -D_POSIX_C_SOURCE=200809 -Wall -std=c11 pcc_server.c -o pcc_server

/* The handler for SIGINT */
void my_SIGINT_handler(int signum)
{
    if (signum == SIGINT)
    {
        sigint = RECIEVED;
        if (accepted == NOT_RECIEVED) //no connected client, so should exit
        {
            print_pcc();
            exit(EXIT_SUCCESS);
        }
    }

    
}

/* Setting a handler for SIGINT */
int register_SIGINT_handler() 
{
    struct sigaction new_action;
    memset(&new_action, 0, sizeof(new_action));
    new_action.sa_handler = my_SIGINT_handler;
    return sigaction(SIGINT, &new_action, NULL);
}

/* Prints counts for the printable characters */
void print_pcc()
{
    int i;
    for (i=printable_LOW; i<=printable_HIGH; i++)
    {
        printf("char '%c' : %u times\n",i,pcc_total[i]);
    }
}


int main(int argc, char *argv[])
{
    unsigned int pcc_temp[printable_HIGH+1];
    int socket_fd = -1, client_fd = -1;
    int i,err, retval, totalsent=0, notwritten=0, bytes_sent=0, enable = 1, bytes_read=0, not_read=0, totalread=0;
    struct sockaddr_in serv_addr, client_addr;
    unsigned int N, port, count_printable, C, Nbuffer = 0;
    char *buff_recv, *buff_send;
    char char_cur;
    socklen_t addrsize = sizeof(struct sockaddr_in);


    for (i=printable_LOW; i<=printable_HIGH; i++) //Initializing the arrays of printable characters counts to 0s
    {
        pcc_total[i] = 0;
        pcc_temp[i] = 0;
    }


    /* Argument validation, conversion and processing */
    if (argc != 2)
    {
        perror("Error: bad number of arguments.\n");
        exit(FAILURE);
    }

    errno = SUCCESS;
    port = strtol(argv[1],NULL,10); //verifying port argument validty
    err = errno;
    if (err != SUCCESS)
    {
        perror(strerror(err));
        exit(FAILURE);
    }

    /* Registering signal handler */
    retval = register_SIGINT_handler();
    if (retval == -1)
    {
        err = errno;
        perror(strerror(err));
        exit(FAILURE);
    }


    /* Initializing TCP server socket */
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
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    retval = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR,&enable, sizeof(int));
    if (retval != SUCCESS)
    {
        err = errno;
        perror(strerror(err));
        exit(FAILURE);
    }

    retval = bind(socket_fd, (struct sockaddr*)&serv_addr, addrsize);
    if (retval != SUCCESS)
    {
        err = errno;
        perror(strerror(err));
        exit(FAILURE);
    }


    retval = listen(socket_fd, 10);
    if (retval != SUCCESS)
    {
        err = errno;
        perror(strerror(err));
        exit(FAILURE);
    }

    /* Ready to accept connections */
    while(sigint == NOT_RECIEVED)
    {
        accepted = NOT_RECIEVED;
        client_fd = accept (socket_fd, (struct sockaddr*)&client_addr, &addrsize);
        if (client_fd < 0)
        {
            err = errno;
            if (err == EINTR)
            {
                break; //exit loop, recieved sigint
            }
            else
            {
                perror(strerror(err));
                exit(FAILURE);
            }
            
            
        }
        accepted = RECIEVED;

        Nbuffer = 0;
        bytes_read = 0;
        not_read = sizeof(unsigned int);
        totalread = 0;
        while (retval < not_read)
        {
            bytes_read = read(client_fd, (&Nbuffer)+totalread, sizeof(unsigned int)); //reading N from client
            if (bytes_read <= 0) //Error handling
            {
                err = errno;
                
                if (bytes_read == 0)
                {
                    perror(strerror(err));
                    if ((err == ETIMEDOUT) || (err == ECONNRESET) || (err == EPIPE))
                    {
                        if (sigint == NOT_RECIEVED)
                        {
                            continue; //Problem with client. Will skip to next iteration, meaning next client
                        }
                        else
                        {
                            break;
                        }
                    }
                    else
                    {
                        exit(FAILURE);
                    }
                }
                else
                {
                    if (err == EINTR)
                    {
                        //try reading again. After finishing handling client, since sigint == RECIEVED, will exit loop
                    }
                    else
                    {
                        perror(strerror(err));
                        exit(FAILURE);
                    }
                    
                }
            }
            else
            {
                /* updating offsets for pointers */
                totalread += bytes_read;
                not_read -= bytes_read;
            }
            
        }

        N = ntohl(Nbuffer);


        buff_recv = (char*)malloc(N); //creating a buffer to read the file (size: N bytes)
        if (buff_recv == NULL)
        {
            err = errno;
            perror(strerror(err));
            exit(FAILURE);
        }

        bytes_read = 0;
        not_read = N;
        totalread = 0;
        while (not_read > 0)
        {
            bytes_read = read(client_fd, buff_recv + totalread, not_read); //reading file from client
            if (bytes_read <= 0) //Error handling
            {
                err = errno;
                if (bytes_read == 0)
                {
                    perror(strerror(err));
                    if ((err == ETIMEDOUT) || (err == ECONNRESET) || (err == EPIPE))
                    {
                        if (sigint == NOT_RECIEVED)
                        {
                            
                            continue; //Problem with client. Will skip to next iteration, meaning next client
                        }
                        else
                        {
                            break;
                        }
                    }
                    else
                    {
                        
                        exit(FAILURE);
                    }
                }
                else
                {
                    if (err == EINTR)
                    {
                        //try reading again. After finishing handling client, since sigint == RECIEVED, will exit loop
                    }
                    else
                    {
                        perror(strerror(err));
                        exit(FAILURE);
                    }
                }
            }
            else
            {
                /* updating offsets for pointers */
                totalread += bytes_read;
                not_read -= bytes_read;
            }
            
        }
        

        count_printable=0;
        for (i=0; i<N; i++)
        {
            char_cur = buff_recv[i];
            if (((int)char_cur >= printable_LOW) && ((int)char_cur <= printable_HIGH)) //if byte in file is a printable character
            {
                pcc_temp[(int)char_cur]+=1;
                count_printable+=1;
            }
        }

        free(buff_recv);

        C = htonl(count_printable);
        buff_send = (char*)&C;
        totalsent = 0;
        notwritten = sizeof(unsigned int); //buffer size: size of C which is an unsigned int
        while (notwritten > 0)
        {
            bytes_sent = write(client_fd, buff_send + totalsent, notwritten); //sending count of printable chars to client
            if (bytes_sent <= 0) //Error handling
            {
                err = errno;
                if (bytes_sent == 0) 
                {
                    perror(strerror(err));
                    if ((err == ETIMEDOUT) || (err == ECONNRESET) || (err == EPIPE)) 
                    {
                        if (sigint == NOT_RECIEVED)
                        {
                            continue; //Problem with client. Will skip to next iteration, meaning next client
                        }
                        else
                        {
                            break;
                        }
                    }
                    else
                    {
                        exit(FAILURE);
                    }
                }
                else
                {
                    if (err == EINTR)
                    {
                        //try writing again. After finishing handling client, since sigint == RECIEVED, will exit loop
                    }
                    else
                    {
                        perror(strerror(err));
                        exit(FAILURE);
                    }
                }
            }
            else
            {
                /* updating offsets for pointers */
                totalsent += bytes_sent;
                notwritten -= bytes_sent;
            }
            
        }

        for (i=printable_LOW; i<=printable_HIGH; i++) //updating pcc_total which holds all counts
        {
            pcc_total[i] += pcc_temp[i];
            pcc_temp[i] = 0;
        }


    }

    print_pcc();
    exit(SUCCESS);
}