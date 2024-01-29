#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
int main(int argc, char *argv[])
{

	openlog(NULL,0,LOG_USER);
	if(argc != 3)
	{
		syslog(LOG_ERR,"Error: Usage ./writer.c <filename> <string_input>\n");
		return 1;
	}
	const char * filepath = argv[1];
	char * file_str = argv[2];
	int fd = open(filepath, O_RDWR|O_CREAT|O_TRUNC|O_NONBLOCK, S_IRWXU|S_IRWXG|S_IRWXO);
	char buff[1];
	if(fd == -1)
	{
		syslog(LOG_ERR,"Error: couldn't open file\n");
		return 1;
	}
	int writeout = write(fd, file_str, strlen(file_str));
	int readlen = read(fd, buff, strlen(file_str));
	close(fd);
	if (writeout == -1 || readlen == -1)
	{
		syslog(LOG_ERR,"Error: did not write corretly\n");
		return 1;
	}
	syslog(LOG_DEBUG,"Writing %s to %s\n", file_str, filepath);
	return 0;
}
