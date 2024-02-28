#include <sys/socket.h>
#include <sys/types.h>
#include <syslog.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#define PORT 9000
int main(int argc, char *argv[])
{
	int status = 0;
	struct addrinfo hints;
	struct addrinfo *servinfo;
	memset (&hints, 0, sizeof(hints)); //Will point to results
	int opt = 1;
	pid_t process_id;
	char buffer[1024];
	char temp_file[100] = "/var/tmp/aesdsocketdata";
	if(argc == 2)
	{
		if(strcpy(argv[1], "-d"))
		{
			process_id = fork();
			if(process_id < 0)
			{
				printf("ERROR: Fork Failed.\n");
				syslog(LOG_ERR,"ERROR : Fork Failed");
				exit(0);
			}
			//Parent Process. Need to kill it.
			if(process_id > 0)
			{				
				printf("The process ID of child is %d.\n", process_id);
				syslog(LOG_DEBUG,"The process ID of child is %d.\n", process_id);
				exit(0);		
			}
		}
	}
	openlog(NULL,0,LOG_USER);
	struct sockaddr_in server_add, client_add;
	socklen_t server_clientlen = sizeof(server_add);
	int fd = socket(PF_INET,SOCK_STREAM,0);
	int fd_client;
	if (fd < 0)
	{
		printf("Error: socket connection failed\n");
		syslog(LOG_ERR,"Error: socket connection failed\n");
		return -1;
	}
	status = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
	
	if (status < 0)
	{
		printf("Error: socket opt failed\n");
		syslog(LOG_ERR,"Error: socket opt failed\n");
		return -1;
	}
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	status = getaddrinfo(NULL, "9000", &hints, &servinfo);
	if (status < 0)
	{
		printf("Error: getaddr fail\n");
		syslog(LOG_ERR,"Error: getaddr fail\n");
		return -1;
	}
	status = bind(fd, servinfo->ai_addr, server_clientlen);
	if (status < 0)
	{
		printf("Error: binding failed\n");
		syslog(LOG_ERR,"Error: binding failed\n");
		freeaddrinfo(servinfo);
		return -1;
	}
	int backlog_cnt = 20;
	status = listen(fd, backlog_cnt);
	printf("listen\n");
	if (status < 0)
	{
		printf("Error: listening failed\n");
		syslog(LOG_ERR,"Error: listening failed\n");
		freeaddrinfo(servinfo);
		return -1;
	}
	int fd_open = open(temp_file, O_RDWR|O_CREAT|O_TRUNC, S_IRWXU|S_IRWXG|S_IRWXO);
	if(fd_open == -1)
	{
		printf("Error: couldn't open file\n");
		syslog(LOG_ERR,"Error: couldn't open file\n");
		return -1;
	}
	
	fd_client = accept(fd, (struct sockaddr *)&client_add, &server_clientlen);
	printf("accept\n");
	if (fd_client < 0)
	{
		printf("Error: accepting failed\n");
		syslog(LOG_ERR,"Error: accepting failed\n");
		freeaddrinfo(servinfo);
		return -1;
	}
	
ssize_t rev_amount;
    do{
        rev_amount = recv(fd_client, buffer, sizeof(buffer) - 1, 0);
        if(rev_amount < 0 )
        {
            perror("ERROR : recv().\n");
            return -1;
        }

        buffer[rev_amount] = '\0';
        printf("\n%s\n", buffer);
        status = write(fd_open, buffer, rev_amount);

        if(rev_amount != status && status != 0)
        {
            printf("\n%ld %d\n", sizeof(buffer), status);
            perror("ERROR : write().\n");
            return -1;
        }
        ssize_t read_amount = 0;

        read_amount = read(fd_open, buffer, sizeof(buffer));
        if(rev_amount != read_amount && read_amount != 0)
        {
            printf("\n%ld %d\n", sizeof(buffer), status);
            perror("ERROR : read().\n");
            return -1;
        }
        status = send(fd_client, buffer, rev_amount, 0);
        if(rev_amount != status)
        {
            printf("\n%d\n", status);
            perror("ERROR : send().\n");
            return -1;
        }
    } while(rev_amount != 0);

	
}
