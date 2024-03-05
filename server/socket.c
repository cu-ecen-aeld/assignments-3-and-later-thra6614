/*******************************************************************************
 * aesdsocket.c
 * Date:        28-01-2024
 * Author:      Raghu Sai Phani Sriraj Vemparala, raghu.vemparala@colorado.edu
 * Description: This file has data related to aesdsocket.c
 * References: Beige Guide
 *
 *
 ******************************************************************************/
#include<stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h> 
#include <syslog.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netinet/in.h>


#include <arpa/inet.h>
/***********GLOBAL_VARIBLES*************/
struct addrinfo hints;
struct addrinfo* servinfo;
struct sockaddr* socketaddr;
int sock_fd;
const char* file_aesdsocket = "/var/tmp/aesdsocketdata";

static void signal_handler(int signo)
{
    if(signo == SIGINT)
    {
        syslog(LOG_INFO,"SIGINT detected");
    }
    else if(signo == SIGTERM)
    {
        syslog(LOG_INFO,"SIGTERM detected");
    }
    close(sock_fd);
    unlink(file_aesdsocket);
    exit(EXIT_SUCCESS);
}

/**********DEFINES**********/
#define REC_LEN 128

int main(int argc, char* argv[])
{
    //Receives data from the recv function
    char rec_val[REC_LEN];
    int accepted = 0, data_sent_flag = 0;
    int file_length = 0;
    int push_data = 0;
    int daemon_flag = 0;
    //Port ID
    const char* service = "9000";
    //Logging start
    openlog("socket_check",LOG_PID, LOG_USER);
    if((argc>1) && strcmp(argv[1],"-d")==0)//Deamon mode entry
    {
        if(daemon(0,0)==-1)// set to daemon mode
        {
            syslog(LOG_ERR, "daemon mode failed");
            exit(EXIT_FAILURE);
        }
        else
        {
            daemon_flag = 1;
        }
    }
/*
 * Register signal_handler as our signal handler
 * for SIGINT.
 */
 if (signal (SIGINT, signal_handler) == SIG_ERR) {
 fprintf (stderr, "Cannot handle SIGINT!\n");
 syslog(LOG_ERR, "Cannot handle SIGINT");
 exit (EXIT_FAILURE);
 }
 /*
 * Register signal_handler as our signal handler
 * for SIGTERM.
 */
 if (signal (SIGTERM, signal_handler) == SIG_ERR) {
 fprintf (stderr, "Cannot handle SIGTERM!\n");
  syslog(LOG_ERR, "Cannot handle SIGTERM");
 exit (EXIT_FAILURE);
 }
    //length of socket
    socklen_t soclen = sizeof(struct sockaddr);
    //Creation of socket
    sock_fd = socket(PF_INET,SOCK_STREAM,0);
    memset(&hints,0,sizeof(struct addrinfo));
    if(sock_fd == -1)
    {
        perror("Socketfd failed");
        printf("Socketfd failed\n");
        syslog(LOG_ERR, "Scoket Creation failed");
        exit(EXIT_FAILURE);
    }

    int sock_accept_fd = 0;
    hints.ai_family = AF_INET;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_STREAM;

    //Accept
    struct sockaddr_storage their_addr;
    socklen_t addr_size;

    //Set the socket to an IP address
    int status = getaddrinfo(NULL,service,&hints,&servinfo);
    if(status != 0)
    {
        freeaddrinfo(servinfo);
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }
    socketaddr = servinfo->ai_addr;
    //Bind socket
    int bind_status = bind(sock_fd, socketaddr, soclen);
    if(bind_status != 0)
    {
        freeaddrinfo(servinfo);
        syslog(LOG_ERR, "Bind Socket Failed");
        perror("Bind_error\n");
        printf("Bind_error\n");
        exit(1);
    }
    //Free servinfo
    freeaddrinfo(servinfo);
    /*Referenced from CHAT Gpt with the question
        How to make my process a Daemon?*/
    if(daemon_flag == 1)
    {
         pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        if (pid > 0) {
            // Parent process - exit
            exit(EXIT_SUCCESS);
        }

        // Create a new session
        if (setsid() < 0) {
            perror("setsid");
            exit(EXIT_FAILURE);
        }

        // Fork second time
        pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        if (pid > 0) {
            // Parent process (first child) - exit
            exit(EXIT_SUCCESS);
        }

        // Change working directory to root to avoid keeping
        // any directory in use that could prevent unmounting
        if (chdir("/") < 0) {
            perror("chdir");
            exit(EXIT_FAILURE);
        }

        // Redirect standard I/O to /dev/null
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        open("/dev/null", O_RDONLY);
        open("/dev/null", O_WRONLY);
        open("/dev/null", O_RDWR);

    }
    //Listen to messages
    int listen_status = listen(sock_fd, 10);
    if(listen_status)
    {
        perror("Listen Failed\n");
        syslog(LOG_ERR, "Listen Failed");
        //printf("Listen Failed\n");
        exit(EXIT_FAILURE);
    }
    int file_fd=open(file_aesdsocket, O_WRONLY | O_CREAT | O_TRUNC, 0644); 
        if (file_fd==-1)
        {
                perror("open");
                exit(1);
        }
    close(file_fd);
    //Rec messages
    addr_size = sizeof(their_addr);
    memset(rec_val,0,REC_LEN);
    uint32_t total_length = 0;
    //
    //
    //Start of accepting the connections
    //
    //

    while(1)
    {      
        //malloc performed
        char *store_data = NULL;
        sock_accept_fd = accept(sock_fd, (struct sockaddr *)&their_addr, &addr_size);
        if(sock_accept_fd == -1)
        {
            perror("Accept Failed");
            syslog(LOG_ERR, "Unable to accept socket");
            exit(0);
        }
        else
        {

            accepted = 1;
                store_data = (char*)malloc(sizeof(char)*REC_LEN);
                if(store_data == NULL)
                {
                    syslog(LOG_ERR,"Unable to allocate memory");
                    printf("malloc failed\n");
                    exit(1);
                }
                else
                {
                    syslog(LOG_DEBUG,"Successfully_created_file");
                    //printf("malloc_succ\n");
                }
                syslog(LOG_INFO, "Accepts connection from %s", inet_ntoa(((struct sockaddr_in*)&their_addr)->sin_addr));
        }
        int recv_len = 0;

        while (accepted)
        {   
            /* code */
            int i = 0;
            recv_len = recv(sock_accept_fd,(void*)rec_val,REC_LEN,0);
            if(recv_len == -1)
            {
                perror("recv_error");
                syslog(LOG_ERR, "Recv ERROR");
                //printf("recv_error\n");
                exit(EXIT_FAILURE);
            }   
            else if(recv_len == 0)//Connection failed
            {
                //printf("In recvlen 0\n");
                accepted = 0;
                total_length = 0;
                break;
            }           
            while(i < recv_len)
            {
                if(rec_val[i] == '\n')
                {
                    push_data = 1;
                    i++;
                    break;
                }
                i++;    
            }
            store_data = (char*)realloc(store_data,total_length+i);
            memcpy(store_data+total_length,rec_val,i);
            total_length+=i;

            memset(rec_val,0,recv_len);
            if(recv_len !=0 && push_data == 1)
            {

                //My data is stored in the buffer, now write it to the file file_aesdsocket
                file_fd = open(file_aesdsocket,O_WRONLY|O_APPEND);
                if(file_fd == -1)
                {
                    printf("File opening failed\n");
                    syslog(LOG_ERR, "File opening failed");
                    exit(EXIT_FAILURE);
                }
                //printf("length is %d\n",total_length);
                int nr = write(file_fd, store_data, total_length);
                if(nr == -1)
	            {
		            //Log error if print failed
		            syslog(LOG_ERR, "write is not sucessful");
                    //printf("Write_failed\n");
		            close(file_fd);
		            exit(EXIT_FAILURE);
	            }
	            else
	            {
	                //check if the whole string is written to the  file
		            if(nr == total_length)
		            {
		                syslog(LOG_DEBUG, "Write_Success");
		                //close(file_fd);
		            }
		            else
		            {
		        	    syslog(LOG_ERR, "Incorrect information written.Repeat the process");
                        //printf("Incorrect info written\n");
		        	    close(file_fd);
		        	    exit(EXIT_FAILURE);
		            }
	            }
                close(file_fd);
                //Incrementing file length so that the whole data is pushed
                file_length+=total_length;
                char file_data[file_length];
                file_fd = open(file_aesdsocket,O_RDONLY);
                if(file_fd == -1)
                {
                    printf("File opening failed\n");
                    syslog(LOG_ERR, "Opening Failed");
                    exit(1);
                }
                int rd = read(file_fd,file_data,file_length);
                if(rd == -1)
                {
                    syslog(LOG_ERR,"unable to read data");
                    printf("read_error\n");
                    close(file_fd);
                }
                close(file_fd);
                int data_sent = 0,data_ptr_inc= 0,length_tobe_sent = file_length;
                //Used while to ensure if the total data is transferred to the client
                while(data_sent_flag == 0)
                {       
                    data_sent = send(sock_accept_fd, file_data+data_ptr_inc, length_tobe_sent, 0);
                    if(data_sent == -1)
                    {
                        syslog(LOG_ERR,"Error in sending the data");
                        printf("Send data failed\n");

                    }
                    else if(data_sent < file_length)
                    {
                        data_ptr_inc+= data_sent;
                        length_tobe_sent-=data_sent;
                       //Repeat while loop
                    }
                    else if(data_sent == file_length)
                    {
                        data_sent_flag = 1;
                    }
                }
                data_sent_flag  = 0;
                push_data = 0;
                free(store_data);
                close(file_fd);
                //Close the socket
		        syslog(LOG_ERR, "Closed connection with %s\n", inet_ntoa(((struct sockaddr_in*)&their_addr)->sin_addr));
            }
        }
    }       
}