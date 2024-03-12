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
#include "queue.h"
#include <arpa/inet.h>
#include <stdbool.h>
#include <pthread.h>
/***********GLOBAL_VARIBLES*************/
struct addrinfo hints;
struct addrinfo* servinfo;
struct sockaddr* socketaddr;
int sock_fd;
pthread_mutex_t rec_lock;
const char* file_aesdsocket = "/var/tmp/aesdsocketdata";
bool sig_trig = false;

static void signal_handler(int signo)
{
    // printf("In_sig_hand\n");
    // printf("%d\n",signo);
    // if(signo == SIGINT)
    // {
    //     syslog(LOG_INFO,"SIGINT detected");
    // }
    // else if(signo == SIGTERM)
    // {
    //     syslog(LOG_INFO,"SIGTERM detected");
    // }
    sig_trig = true;//Trigger signal handling for cleanup
    //printf("sigtrig:%d\n",sig_trig);
    //exit(EXIT_SUCCESS);
}


/**********DEFINES**********/
#define REC_LEN 128
#define FILE_DATA_LEN 1024
#define TIMESTAMP_FORMAT "%Y, %b %a %d %H:%M:%S\n"
typedef struct thread_list{
	pthread_t thread_id; 
	int sock_accept_fd; 
	bool thread_complete_flag;
    struct sockaddr_storage client_addr; 
    /*struct {								\
	struct type *sle_next;	//next element 			\
}*/
	SLIST_ENTRY(thread_list) next; 
}thread_list;

/*struct head_thread_struct {								\
	struct thread_list *slh_first; first element 		\
}*/
SLIST_HEAD(head_thread_struct, thread_list) thread_head;

void* thread_rec_data(void* thread_arg)
{
    int push_data = 0;
    //uint32_t total_length = 0;
    int accepted = 1,file_fd = -1;
    uint32_t total_length = 0;
    char rec_val[REC_LEN];
    memset(rec_val,0,REC_LEN);
    thread_list *thread_info = (thread_list*) thread_arg;
    char* store_data = (char*)malloc(sizeof(char)*REC_LEN);
    if(store_data == NULL)
    {
        syslog(LOG_ERR,"Unable to allocate memory");
        printf("malloc failed\n");
        thread_info->thread_complete_flag = true;
        return NULL;
    }
    else
    {
        syslog(LOG_DEBUG,"Successfully_created_file");
        //thread_info->thread_complete_flag = true;
        //printf("malloc_succ\n");
    }
    int recv_len = 0;
    memset(store_data,0,REC_LEN);
    //total_length = 0;    
    while (accepted)
    {   
            /* code */
            int i = 0;
            recv_len = recv(thread_info->sock_accept_fd,(void*)rec_val,REC_LEN,0);
            if(recv_len == -1)
            {
                perror("recv_error");
                syslog(LOG_ERR, "Recv ERROR");
                //printf("recv_error\n");
                close(thread_info->sock_accept_fd);
                thread_info->thread_complete_flag = true;
                //close(thread_info->sock_accept_fd);
                return NULL;
            }   
            else if(recv_len == 0)//Connection failed
            {
                //printf("In recvlen 0\n");
                accepted = 0;
                total_length = 0;
                thread_info->thread_complete_flag = true;
                close(thread_info->sock_accept_fd);
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
                pthread_mutex_lock(&rec_lock);
                //My data is stored in the buffer, now write it to the file file_aesdsocket
                file_fd = open(file_aesdsocket,O_WRONLY|O_APPEND);
                if(file_fd == -1)
                {
                    printf("File opening failed\n");
                    syslog(LOG_ERR, "File opening failed");
                    close(thread_info->sock_accept_fd);
                    thread_info->thread_complete_flag = true;
                    return NULL;
                    //exit(EXIT_FAILURE);
                }
                //printf("length is %d\n",total_length);
                int nr = write(file_fd, store_data, total_length);
                if(nr == -1)
	            {
		            //Log error if print failed
		            syslog(LOG_ERR, "write is not sucessful");
                    //printf("Write_failed\n");
		            close(file_fd);
                    thread_info->thread_complete_flag = true;
                    close(thread_info->sock_accept_fd);
                    return NULL;
		            //exit(EXIT_FAILURE);
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
                        thread_info->thread_complete_flag = true;
                        close(thread_info->sock_accept_fd);
                        return NULL;
		        	    //exit(EXIT_FAILURE);
		            }
	            }
                close(file_fd);
                pthread_mutex_unlock(&rec_lock);
                //Incrementing file length so that the whole data is pushed
                char file_data[FILE_DATA_LEN];
                pthread_mutex_lock(&rec_lock);
                file_fd = open(file_aesdsocket,O_RDONLY);
                if(file_fd == -1)
                {
                    printf("File opening failed\n");
                    syslog(LOG_ERR, "Opening Failed");
                    close(thread_info->sock_accept_fd);
                    thread_info->thread_complete_flag = true;
                    return NULL;
                }
                int rd = 1;
                while((rd = read(file_fd,file_data,sizeof(file_data))) > 0)
                {
                        if(rd == -1)
                        {
                            syslog(LOG_ERR,"unable to read data");
                            printf("read_error\n");
                            close(file_fd);
                            thread_info->thread_complete_flag = true;
                            close(thread_info->sock_accept_fd);
                            return NULL;
                        }
                        //close(file_fd);
                        int data_sent = 0;//data_ptr_inc= 0;//length_tobe_sent = rd;
                        //Used while to ensure if the total data is transferred to the client
                        //while(data_sent_flag == 0)
                        //{       
                            data_sent = send(thread_info->sock_accept_fd, file_data, rd, 0);
                            if(data_sent == -1)
                            {
                                syslog(LOG_ERR,"Error in sending the data");
                                printf("Send data failed\n");
                                thread_info->thread_complete_flag = true;
                                close(thread_info->sock_accept_fd);
                                return NULL;
                            }

                }
                if(rd == -1)
                {
                    syslog(LOG_ERR,"unable to read file");
                    printf("file_read_error\n");
                     close(thread_info->sock_accept_fd);
                    close(file_fd);
                    return NULL;
                }
                //data_sent_flag  = 0;
                push_data = 0;
                free(store_data);
                close(file_fd);
                pthread_mutex_unlock(&rec_lock);
                //Close the socket
		        //syslog(LOG_ERR, "Closed connection with %s\n", inet_ntoa(((struct sockaddr_in*)&their_addr)->sin_addr));
            }
    }
    thread_info->thread_complete_flag = true;
    close(thread_info->sock_accept_fd);
    return NULL;
}

//I have referenced CHATGPT for obtaining the system wall clock time
//Question asked was: How to obtain minutes and seconds from wall clock time to input for strftime() function
void* thread_time(void* threadarg)
{
     time_t current_time;
     struct tm *time_info;
     char timestamp_buffer[128];
     memset(timestamp_buffer,0,128);
     sleep(10);
     while (!sig_trig) {
        int file_fd;   
        
        // Get current time
        current_time = time(NULL);
        time_info = localtime(&current_time);

        // Format timestamp
        strftime(timestamp_buffer, sizeof(timestamp_buffer), TIMESTAMP_FORMAT, time_info);
        printf("timestamp:%s\n",timestamp_buffer);
        pthread_mutex_lock(&rec_lock);
        // Open the file in append mode
        file_fd = open(file_aesdsocket, O_WRONLY | O_APPEND);
        if (file_fd == -1) {
            perror("Error opening file");
            return NULL;
        }

        // Append timestamp to file
        if (write(file_fd, "timestamp:", strlen("timestamp:")) == -1) {
            perror("Error writing to file");
            close(file_fd);
            return NULL;
        }

        if (write(file_fd, timestamp_buffer, strlen(timestamp_buffer)) == -1) {
            perror("Error writing to file");
            close(file_fd);
            return NULL;
        }
        // Close the file
        close(file_fd);
        pthread_mutex_unlock(&rec_lock);
        // Wait for 10 seconds
        sleep(10);
    }
    printf("Time_thread_complete\n");
    return NULL;
}
 
int main(int argc, char* argv[])
{

    int daemon_flag = 0;
    //Port ID
    const char* service = "9000";
    //Logging start
    openlog(NULL,LOG_PID, LOG_USER);

    //Initialize head to NULL
    SLIST_INIT(&thread_head);
    if((argc>1) && strcmp(argv[1],"-d")==0)//Deamon mode entry
    {
       
        daemon_flag = 1;
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
    int flags = fcntl(sock_fd, F_GETFL, 0);
    fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK);
    int sock_accept_fd = 0;
    hints.ai_family = AF_INET;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_STREAM;
    int value = 1;
    //Accept
    struct sockaddr_storage their_addr;
    socklen_t addr_size = 0;
    // Set socket options for reusing address and port
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &value, sizeof(int)))
    {
      printf("Failed to set socket options:%s\n", strerror(errno));
      syslog(LOG_ERR, "Failed to set socket options:%s\n", strerror(errno));
      return -1;
    }
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
    if (pthread_mutex_init(&rec_lock, NULL) != 0) 
    {
        // Mutex initialization failed
         syslog(LOG_ERR, "Mutex_init_failed");
         printf("Mutex_init_failed\n");
         exit(EXIT_FAILURE);
        // Handle error
    }
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
    //memset(rec_val,0,REC_LEN);
    pthread_t tid = 0;
    if(pthread_create(&tid, NULL, thread_time, NULL) != 0) 
    {
                syslog(LOG_ERR, "Thread_creation _failed: %s", strerror(errno));
                //return 1;
                printf("thread_creation_failed\n");
    }
    
    //
    //Start of accepting the connections
    //
    // 
    int threads_created  = 0; //Debug variable
    while(!sig_trig)
    {      
        //malloc performed
        //printf("Before_accept\n");
        sock_accept_fd = accept(sock_fd, (struct sockaddr *)&their_addr, &addr_size);

        if(sock_accept_fd == -1)
        {
            //perror("Accept Failed");
            //syslog(LOG_ERR, "Unable to accept socket");
            //exit(0);
            continue;
        }
        else
        {
            thread_list *new_thread = (thread_list*)malloc(sizeof(thread_list));
            if(new_thread == NULL)
            {
                 syslog(LOG_ERR,"Unable to allocate memory");
                printf("Nospace avaialble\n");
        
        //exit(1);
            }
            else
            {
                syslog(LOG_DEBUG,"Successfully_created_file");
                printf("malloc_succ\n");
            }   
            new_thread->sock_accept_fd = sock_accept_fd;
            if(pthread_create(&(new_thread->thread_id), NULL, thread_rec_data, (void *)new_thread) != 0) 
            {
                syslog(LOG_ERR, "Thread_creation _failed: %s", strerror(errno));
                //return 1;
                printf("Error with thread creationnnn\n");
                free(new_thread);
            }
            else
            {
                threads_created++;
                SLIST_INSERT_HEAD(&thread_head, new_thread, next);
                
            //create pthread store thread id in the               
            }
        }
        thread_list* node,*next_val;
                SLIST_FOREACH_SAFE(node, &thread_head, next, next_val)
                {
				    if(node -> thread_complete_flag)
                    {
                        printf("At_join_in_while\n");
				        pthread_join(node->thread_id,NULL); 
                        SLIST_REMOVE(&thread_head, node, thread_list, next);
                        free(node);
                        threads_created--;
                        printf("remaining_threads:%d\n",threads_created);
                    }
                    printf("In_thread_removal_out\n");
                }
        
                //data_sent_flag  = 0;
                //push_data = 0;
        printf("In_while_sig\n"); 
    }
    printf("Iamout\n"); 
    if(sig_trig)
    {
        printf("At_join_in_sig\n");
        //pthread_join(tid,NULL);
        thread_list* node,*next_val;
                SLIST_FOREACH_SAFE(node, &thread_head, next, next_val)
                {
				    if(node -> thread_complete_flag)
                    {
                        printf("At_join_in_sig\n");
				        pthread_join(node->thread_id,NULL); 
                        SLIST_REMOVE(&thread_head, node, thread_list, next);
                        free(node);
                        threads_created--;
                        printf("remaining_threads:%d\n",threads_created);
                    }
                    printf("In_thread_removal_if_sig\n");
                }

        //exit(EXIT_SUCCESS);
    }
    //SIGHANDLING_EXIT_SEQ
    close(sock_fd);   
    unlink(file_aesdsocket);
    pthread_join(tid,NULL);
    close(sock_accept_fd);
    pthread_mutex_destroy(&rec_lock);
    shutdown(sock_fd, SHUT_RDWR);
    closelog();

    exit(EXIT_SUCCESS);

}