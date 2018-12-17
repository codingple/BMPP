#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/select.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <net/if.h>
#define max_buf 255
#define sock_info 50

void error_handling(char *message);
void * handler (void * arg);
void * connect_write (void * arg);
void * connect_read (void * arg);
void * find_handler(void * arg);
void * accept_write(void * arg);
void * accept_read(void * arg);

int fd_max=0;
int sock, serv_sock, clnt_sock, clnt_adr_sz, swit=0;
fd_set reads, cpy_reads;
pthread_t t_id;
char pb_ip[sock_info];
char pb_ip2[sock_info];
char pv_ip[sock_info];
char pv_ip2[sock_info];
char filename[sock_info];
char port[sock_info];
char port_num[sock_info];
struct sockaddr_in addr, serv_addr, clnt_addr;

const char upload[] = "up";
const char download[] = "down";
const char no_file[] = "no";
const char shr_dir[] = "./BMPP";
const char bmpp_port[] = "9190";
const char client_port[] = "9191";

char* arg2 = "[BMPP] Start to connecting thread...\n";
char* arg1 = "[BMPP] Select thread is created\n";

int main(int argc, char* argv[])
{
	int i;
	char buf[max_buf];
	int str_len;
	char ipadd[sock_info];
	char ipadd2[sock_info];
        struct stat st;
        int fd;
        struct ifreq ifr;

        // get private ip
        fd = socket(AF_INET, SOCK_DGRAM, 0);
        ifr.ifr_addr.sa_family = AF_INET;
        strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);
        ioctl(fd, SIOCGIFADDR, &ifr);
        close(fd);
        strcpy(ipadd2, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
	
	// insert the server ip
	printf("[BMPP] Insert the server IP : ");
	scanf("%s", ipadd);

	// set socket for connecting to server
	sock=socket(PF_INET, SOCK_STREAM, 0);
	if(sock == -1)
		error_handling("socket() error");
	
	memset(&addr, 0, sizeof(addr));
	addr.sin_family=AF_INET; // IPv4
	addr.sin_addr.s_addr=inet_addr(ipadd); // IP address
	addr.sin_port=htons(atoi(bmpp_port)); // port
	
	// set socket for client
	serv_sock=socket(PF_INET, SOCK_STREAM, 0);

        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family=AF_INET;
        serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
        serv_addr.sin_port=htons(atoi(client_port));

        if(bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr))==-1)
                error_handling("bind() error");
        if(listen(serv_sock, 5)==-1)
                error_handling("listen() error");

        // initial fd_set
	FD_ZERO(&reads);
	FD_SET(sock, &reads);
        FD_SET(serv_sock, &reads);
        if( sock < serv_sock )
		fd_max=serv_sock;
	else
		fd_max=sock;

	
	// connect to server	
	if(connect(sock, (struct sockaddr*)&addr, sizeof(addr))==-1)
		error_handling("connect() error!");
	strcat(ipadd2, "\0");
        write(sock, ipadd2, sizeof(ipadd2));
	printf("[BMPP] Server is connected! -----\n");

	// if this peer doesn't have directory
	if ( stat(shr_dir, &st) == -1 ){
		if ( mkdir(shr_dir, 0755) == 0 ){
			printf("[BMPP] New to Bonjour Monde P2P!\n");
			printf("[BMPP] Created BMPP directory.\n");
		}

		else{
			error_handling("mkdir() error!");
		}
	}
	// create select handler
	pthread_create(&t_id, NULL, handler, (void *)arg1);
	pthread_detach(t_id);

	while (1) {
	// search the file
	scanf("%s", filename);

	// send filename to server
	write(sock, filename, sizeof(filename));
	} // end of while

	return 0;

}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}


// select handler
void * handler (void * arg){
	int fd_num, i, str_len;
	char mes[max_buf];
	strcpy(mes, (char*)arg);
	printf("%s", mes);

	while(1){
		cpy_reads = reads;
		if((fd_num=select(fd_max+1, &cpy_reads, 0, 0, NULL))==-1)
                        break;
                if(fd_num==0)
                        continue;
                for(i=0; i<fd_max+1; i++){
		if(FD_ISSET(i, &cpy_reads)){
		// from server
		if ( i == sock ){
			char buf_in_sock[sock_info];
			char *token;
			str_len=read(i, buf_in_sock, sizeof(buf_in_sock));
			token = strtok(buf_in_sock,",");
			
			// if acknowledge to this peer
			if( strcmp( token, download) == 0 ){
				token = strtok(NULL, ",");

				// if there is no file
				if ( strcmp(token, no_file) == 0 )
					printf("[BMPP] We can't find the file\n");

				// or success to find peer
				else{
					printf("[BMPP] Success to find the file!\n");
					// recieve private ip and public ip
                                        strcpy(pv_ip, token);
					printf("[BMPP] Connecting to peer...\n");
					// create thread to download file
                                        pthread_create(&t_id, NULL, connect_read, (void *)pv_ip);
                                        pthread_detach(t_id);
				} // end of else
			}

			// if request to find file from server
			else if ( strcmp(token, upload) == 0 ){
				token = strtok(NULL, ",");
				pthread_create(&t_id, NULL, find_handler, (void *)token);
				pthread_detach(t_id);
			}
		} // end of sock

		// from other peer (client)
		else if ( i == serv_sock ){
			clnt_adr_sz = sizeof(clnt_addr);
			clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_adr_sz);

			printf("[BMPP] Other peer is connected!\n");			
			// if write
			pthread_create(&t_id, NULL, accept_write, (void *)&clnt_sock);
			pthread_detach(t_id);
		} // end of serv_sock
		} // end of FD_ISSET
		} // end of for
	} // end of while
}// end of handler


// handler connecting to peer for download
void * connect_read(void * arg){
	int fd, down_sock, str_len;
	struct sockaddr_in down_addr;
	char ip[max_buf];
	char buf_in_con[max_buf];
	char new_dir[max_buf];
	strcpy(new_dir, shr_dir);
	strcat(new_dir, "/");
        strcpy(ip, (char*)arg);
strcpy(port_num, client_port);

	// set up socket
	down_sock=socket(PF_INET, SOCK_STREAM, 0);
        if(sock == -1)
                error_handling("socket() error");

        memset(&down_addr, 0, sizeof(down_addr));
        down_addr.sin_family=AF_INET; // IPv4
        down_addr.sin_addr.s_addr=inet_addr(ip); // IP address
        down_addr.sin_port=htons(atoi(port_num)); // port

	if(connect(down_sock, (struct sockaddr*)&down_addr, sizeof(down_addr))==-1){
                printf("[BMPP] connect_read() closed\n");
		return;
	}

	printf("%s", arg2);

	// find directory and open fd
	strcat( new_dir, filename );
        fd=open(new_dir, O_CREAT|O_RDWR|O_TRUNC,0644);

	if(fd==-1)
        	error_handling("open() error!");
	
	// send filename
	write(down_sock, &filename, sizeof(filename));

	// download
        while( 1 ){
        	str_len = read(down_sock, &buf_in_con, sizeof(buf_in_con));


                if(str_len==0)
                	break;
        
                write(fd, &buf_in_con, str_len);
                }
		printf("[BMPP] Download is done!\n");
                close(fd);
                close(down_sock);
                return NULL;
}


// request from server to find file
void * find_handler(void * arg){
	DIR *dir_info;
	struct dirent *dir_entry;
	char yes[] = "yes\0";
	char no[] = "no\0";
	char file[sock_info];
	char *token;
	char buf_in_sock[max_buf];
	int check = 0;
	int str_len;
	pthread_t t_id;
	strcpy(file, (char*)arg);
	printf("[BMPP] Server is finding file in your directory...\n");
	dir_info = opendir(shr_dir);

	// read shared directory
	if ( dir_info != NULL ){
		while( dir_entry = readdir(dir_info) ){
			// if found
			if ( strcmp(file, dir_entry->d_name) == 0 ){
				check = 1;
				break;
			} // end of if
		}// end of while
	}

	// if there is not the file
	if ( check == 0 ){
		write(sock, no, sizeof(no));
}

	// if found the file
	else{
		write(sock, yes, sizeof(yes));
		}

	return NULL;
}// end of find_handler


// accept handler to write
void * accept_write(void * arg){
	int tmp_sock = *((int*)arg);
	int fd, check;
	int str_len=0, i;
	char buf_in_acc[max_buf];
	char file[sock_info];
        strcpy(file, shr_dir);
	strcat(file, "/");
	
	// receive file name
	read(tmp_sock, &buf_in_acc, sizeof(buf_in_acc));
	strcat(file, buf_in_acc);
	// open file
	fd = open(file, O_RDONLY);
	
	// open file error
	if ( fd == -1 ){
		printf("[BMPP] ERROR : Send file to peer\n");
		return NULL;
	}
	
	// send file
	else{
		while(1){
			check = read(fd, &buf_in_acc, sizeof(buf_in_acc));

                        // finish
                        if ( check == 0 )
                                break;

                        // send file
                        write(tmp_sock, &buf_in_acc, check);
		} // end of while
	}//end of else
	close(tmp_sock);
	return NULL;
}
