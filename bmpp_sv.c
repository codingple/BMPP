#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/select.h>
#include <memory.h>
#include <string.h>
#define max_buf 255
#define sock_info 50

typedef struct node{
	int sock;
	char pv_ip[sock_info];
	char pb_ip[sock_info];
	struct node *next;
}node;

typedef struct LinkedList{
	node *head;
	int length;
}LinkedList;



void error_handling(char *message);
void init_list(LinkedList *list);
void add_list(LinkedList *list, int new_sock, char *pb_ip, char *pv_ip);
void del_list(LinkedList *list, int cmp_sock);


int main(int argc, char *argv[])
{
	int serv_sock, clnt_sock;
	int i, j, fd_max;
        struct sockaddr_in serv_adr, clnt_adr;
        int clnt_adr_sz,fd_num=0, str_len=0;
	LinkedList list;
	node *p;
	node *q;
	unsigned long tmp_ip;
	char *buf_ip;
	char *buf_port;
	char filename[sock_info];
	char r_buf[sock_info];
	char up[sock_info];
	char down[sock_info];
	char s_buf[sock_info];
	char buf[sock_info];
	fd_set reads, cpy_reads;
	const char bmpp_port[] = "9190";
	
	init_list(&list);
        serv_sock=socket(PF_INET, SOCK_STREAM, 0);

        memset(&serv_adr, 0, sizeof(serv_adr));
        serv_adr.sin_family=AF_INET;
        serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
        serv_adr.sin_port=htons(atoi(bmpp_port));

        if(bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr))==-1)
                error_handling("bind() error");
        if(listen(serv_sock, 5)==-1)
                error_handling("listen() error");

        // initial fd_set
        FD_ZERO(&reads);
        FD_SET(serv_sock, &reads);
        fd_max=serv_sock;


        while(1) {
        cpy_reads=reads;
        if((fd_num=select(fd_max+1, &cpy_reads, 0, 0, NULL))==-1){
		error_handling("select() error");
	}

	else{                
                                        
                if(fd_num==0)
                        continue;
                for(i=0; i<fd_max+1; i++) {
                if(FD_ISSET(i, &cpy_reads)){
			// if new connection
			if(i==serv_sock){
                                int n,c;
                                char tmpip[sock_info];
                                char tmpip2[sock_info];
                                clnt_adr_sz=sizeof(clnt_adr);
                                clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr,&clnt_adr_sz);
                                // store clnt information
                                // ip address
                                read(clnt_sock, tmpip2, sizeof(tmpip2));
                                strcpy(tmpip, inet_ntoa(clnt_adr.sin_addr));
                                printf("[%d : connect] private : %s, public : %s\n", clnt_sock, tmpip2, tmpip);

                                add_list(&list, clnt_sock, tmpip, tmpip2);

				FD_SET(clnt_sock, &reads);
                                if(fd_max<clnt_sock){
                                        fd_max=clnt_sock;
                                }
                        }

			// file name
			else{
				char filename[sock_info];
				p = list.head;
				str_len = read(i, filename, sizeof(filename));

				// if dead sock
                                if ( str_len == 0){
					printf("[Close] %d\n", i);
					del_list(&list, p->sock);
					FD_CLR(i, &reads);
					close(i);
                                        break;
                                }
				strcpy(up, "up,");
				strcpy(down, "down,");
				strcpy(s_buf, up);
				strcat(s_buf, filename);

				printf("[%d : Filename] %s\n", i, filename);
				// send filename to all peer
				for(j=0; j<list.length; j++){
					if ( i != p->sock ){
					write(p->sock, &s_buf, sizeof(s_buf));
					str_len = read(p->sock, &r_buf, sizeof(r_buf));
					// if find the file
					if ( strcmp(r_buf, "yes") == 0 ){
						int k;

						// server information
						strcpy(buf, "down,");
                                                strcat(buf, p->pb_ip);
						strcat(buf, "\0");
						
						write(i, &buf, sizeof(buf));
						break;

						} // end of write
					} // end of if
					p = p->next;
				}// end of for

				// if no file
				if ( j == list.length ){
					strcpy(buf, "down,");
					strcat(buf, "no");
					strcat(buf, "\0");

					write(i, &buf, sizeof(buf));
				}// end of if
			}// end of file name
		} //end of FD_ISSET
	}// end of for
	}// end of else
}// end of while
	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}


// init list
void init_list(LinkedList *list){
	list->head = NULL;
	list->length = 0;
}

// add list
void add_list(LinkedList *list, int new_sock, char *npb_ip, char *npv_ip){
        node *new_node;
        node *p;
        p = list->head;

        // initialize new node
        new_node = (node *)malloc(sizeof(node));
        new_node->sock = new_sock;
        strcpy(new_node->pb_ip, npb_ip);
        strcpy(new_node->pv_ip, npv_ip);
        new_node->next = NULL;

        // first of list
        if ( p == NULL ){
                list->head = new_node;
        }

        // not first of list
        else{
                while( p->next != NULL )
                        p = p->next;

                p->next = new_node;
        }

        list->length++;
}

void del_list(LinkedList *list, int cmp_sock){
	int i;
	int max = list->length;
	node *p;
	node *tmp;
	p = list->head;
	
	for(i=0; i<max; i++){
		if ( p->sock == cmp_sock ){
			// first of list
			if ( i == 0 ){
				list->head = p->next;
				break;
			}

			else{
				tmp->next = p->next;
				break;
			}
		}

		tmp = p;
		p = p->next;
	}

	free(p);
	list->length--;
}
