/* C Programming Language Libraries*/
#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
/* Custom structures */
#include "bin_nums.h"
#include "hash.h"
#include "lists.h"
/* Definition of binary files */
#define AUX_FILE "aux_file.bin"
#define UPDATED_HSFILE "upd_hsfile.bin"
/* Error codes */
#define HASH_TABLE_SIZE 103
#define MEM_ERROR 1
#define ARGS_ERROR 2
#define FILE_ERROR 3

/* Thread mutex for locking the objects */
pthread_mutex_t counter_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t number_threads_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cvar ;

/* Will hold needle_index items which are pointed by hash_index */
hash hashTable;
list_t	needle_index_list;

/* Socket structures and time variables */
struct sockaddr_in server ,client ;
struct tm *current;
time_t now;

/* Prototypes Function */
void * thread_server(void* newso);
void perror_exit(char * message);
void sigchld_handler(int sig);

int main(int argc, char* argv []){
	bin_nums bn;
	pthread_cond_init(&cvar,NULL);
	int port , sock , newsock,true = 1,sin_size,i=1;

	//struct sockaddr_in server ,client ;
	socklen_t clientlen ;
	struct sockaddr * clientptr =( struct sockaddr *) & client ;
	char *t;

	int hash_index, hsfile_ok;

	size_t		size;
	Needle 		*needle;
	NeedleIndex	needle_index;
	FILE 		*h_file_d,
					/* Haystack File */
					*upd_h_file_d,
					*offset_file_d;
					/* ID-Indexed auxiliary File with offsets(inside HayStack) for each Needle */
	list_t	new_needle_index;

	if(argc != 5){
		fprintf(stderr, "Usage:\n\t %s -f <path_to_haystack_file> -p <port>\n", argv[0]);
		exit(ARGS_ERROR);
	}

	for(i = 1; i < argc; i++){
		if( strcmp(argv[i], "-p") == 0 ) {
			if((port = atoi(argv[i+1])) == 0) {
				fprintf(stderr, "Usage:\n\t %s -f <path_to_haystack_file> -p <port>\n", argv[0]);
				exit(ARGS_ERROR);
			}
		}

		if( strcmp(argv[i], "-f") == 0 ) {
			file_b=malloc((strlen(argv[i+1])+1)*sizeof(char));
			strcpy(file_b,argv[i+1]);
		}
	}

	time(&now);
	current = localtime(&now);
	char * path = getwd(argv[2]);
	t = asctime(current);
	t[strlen(t)-1] = '\0';
	printf("[%s] Server is starting. Working directory: %s\n", t , path);
	printf("[%s] Creating in-memory datastructures...\n", t);

	hash_init(&hashTable, HASH_TABLE_SIZE);
	needle_index_list = list_create();

	if((h_file_d = fopen(file_b, "rb")) == NULL){	/* First run - HayStack file nonexistent */
		hsfile_ok = 0;
	}

	else{ /* file already exists */
		/* is it a valid one ? */
		fread(&bn, sizeof(bin_nums), 1, h_file_d);
		if(ferror(h_file_d) != 0){
			perror("fread");
			exit(FILE_ERROR);
		}

		if(magic_number != bn.mag_num){	/* nah, corrupted?? */
			hsfile_ok = 0;
			fclose(h_file_d);
		}

		else{
			counter=bn.next_id;
			hsfile_ok = 1;
		}
	}

	if(hsfile_ok){
		/**************************************************
		**********   	    BOOTSTRAPPING	        ***********
		***************************************************
		****** 1) Load index file in memory ***************
		****** 2) Create hash for access in HayStack ******
		****** 3) HayStack File Compaction ****************
		**************************************************/
		if((offset_file_d = fopen(AUX_FILE, "rb")) == NULL){
			perror("fopen");
			exit(FILE_ERROR);
		}
		if((upd_h_file_d = fopen(UPDATED_HSFILE, "w+b")) == NULL){
			perror("fopen");
			exit(FILE_ERROR);
		}
		fwrite(&bn, sizeof(bin_nums), 1, upd_h_file_d);
		int nhayneedles = 0;
		while(!feof(offset_file_d)){	/* for each ACTIVE needle in index file */
			fread(&needle_index, sizeof(NeedleIndex), 1, offset_file_d);
			if(!feof(offset_file_d)){
				nhayneedles++;
				printf("Needle ID: %d\n", needle_index.Id);
				/* alloc memory */
				size = needle_index.end_offset - needle_index.start_offset + 1;
				needle = malloc(size);
				/* seek startbyte in HS file */
				fseek(h_file_d, needle_index.start_offset, SEEK_SET);
				/* read needle */
				fread(needle, size, 1, h_file_d);
				/* update starting byte in new HS file */
				needle_index.start_offset = ftell(upd_h_file_d);
				/* copy needle to new HS file */
				fwrite(needle, size, 1, upd_h_file_d);
				/* update ending byte in new HS file */
				needle_index.end_offset = ftell(upd_h_file_d) - 1;
				free(needle);

				/* add needle_index in needle_index list */
				if(list_push_front(&needle_index_list, &needle_index, sizeof(NeedleIndex)) == OUT_OF_MEMORY){
					fprintf(stderr, "Memory allocation problem!\n");
					exit(MEM_ERROR);
				}
				hash_index = getHashIndex(needle_index.Id, &hashTable);
				new_needle_index = list_get_first_node(&needle_index_list);
				/* map it in hash table */
				add_in_hash(&hashTable, new_needle_index, hash_index);
			}
		}
		time(&now);
		current = localtime(&now);
		t = asctime(current);
		t[strlen(t)-1] = '\0';
		printf("[%s] %d files-needles found in the haystack files\n", t, nhayneedles);
		fclose(upd_h_file_d);
		fclose(h_file_d);
		fclose(offset_file_d);
		if (remove(file_b) == -1){
  			perror("remove");
			exit(FILE_ERROR);
		}
		if((rename(UPDATED_HSFILE, file_b)) != 0){
			perror("rename");
			exit(FILE_ERROR);
		}
	}
	else{
		printf("[%s] 0 files-needles found in the haystack files\n", t);
		if((h_file_d = fopen(file_b, "w+b")) == NULL){
			perror("fopen");
			exit(FILE_ERROR);
		}
		bn.mag_num=magic_number;
		bn.next_id=0;
		//fseek(h_file_d,sizeof(struct bin_nums), SEEK_SET);
		fwrite(&bn, sizeof(bin_nums), 1, h_file_d);
		fclose(h_file_d);

	}
	time(&now);
	current = localtime(&now);
	t = asctime(current);
	t[strlen(t)-1] = '\0';
	printf("[%s] Server is ready for new clients\n", t);
	if (( sock = socket ( AF_INET , SOCK_STREAM , 0) ) < 0)
	perror_exit ( " socket " ) ;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &true, sizeof (true)) == -1) {
    perror("Setsockopt");
    exit(1);
}
	server.sin_family = AF_INET ;
	server.sin_addr.s_addr=htonl(INADDR_ANY) ;
	server.sin_port = htons ( port ) ;
	if ( bind ( sock ,(struct sockaddr *) &server, sizeof(server) ) < 0)
	perror_exit ( "bind" ) ;
	if ( listen ( sock , 5) < 0) perror_exit ( " listen " ) ;
	printf ( "Listening for connection s to port %d .\n" , port ) ;
	while (1)
	{
		signal ( SIGINT , sigchld_handler ) ;
		sin_size = sizeof (client);
		pthread_t *thread = (pthread_t *)malloc( sizeof(pthread_t) );
		if (( newsock = accept ( sock , (struct sockaddr *) &client , &sin_size ) ) <= -1 )
			perror_exit ( "accept" ) ;
		printf ( "Accepted connection .\n" ) ;
		pthread_create(thread, NULL , thread_server, ( void *) &newsock);
		free(thread);
	}

return 0;
}


void * thread_server ( void * a) {
printf ( "The multiple  thread % ld .\n" ,pthread_self());
int newsock = *(int *) a;
FILE *h_file_d;
char buf [1],length[1000000],down_pin[1000000],down_pin2[1000000],delete_pin[1000000],delete_pin2[1000000],*arxeio;
int i=0,ii=0,len=0,post=0,bool=0,lathos=0,servererror=0,down=0,id_down=0,non_found_down=0,bad_down=0,iii=0,delete=0,id_delete=0,bad_delete=0;
char *IP_ptr, *t;
IP_ptr = (char *)inet_ntoa(client.sin_addr) ;

pthread_mutex_lock(&number_threads_lock);
number_threads++;
pthread_mutex_unlock(&number_threads_lock);


while (  bool == 0 )
{
	read ( newsock , buf , 1);
	putchar(buf[0]);
	if(buf[0] == 'P' && post==0){
	read ( newsock , buf , 1);
	putchar(buf[0]);
	if(buf[0] == 'O'){
	read ( newsock , buf , 1);
	putchar(buf[0]);
	if(buf[0] == 'S'){
	read ( newsock , buf , 1);
	putchar(buf[0]);
	if(buf[0] == 'T'){
	read ( newsock , buf , 1);
	putchar(buf[0]);
	post=1;
	}}}}

	////////////////////////

	else if(buf[0] == 'G'  && down==0){
	read ( newsock , buf , 1);
	putchar(buf[0]);
	if(buf[0] == 'E'){
	read ( newsock , buf , 1);
	putchar(buf[0]);
	if(buf[0] == 'T'){
	read ( newsock , buf , 1);
	putchar(buf[0]);
	if(buf[0] == ' '){
	read ( newsock , buf , 1);
	putchar(buf[0]);
	if(buf[0] == '/'){
	read ( newsock , buf , 1);
	putchar(buf[0]);
	if(buf[0] == '?'){
	read ( newsock , buf , 1);
	putchar(buf[0]);
	if(buf[0] == 'i'){
	read ( newsock , buf , 1);
	putchar(buf[0]);
	if(buf[0] == 'd'){
	read ( newsock , buf , 1);
	putchar(buf[0]);
	if(buf[0] == '='){
	read ( newsock , buf , 1);
	putchar(buf[0]);
	while(buf[0]!= ' '){
	down_pin[ii]=buf[0];
	read ( newsock , buf , 1);
	putchar(buf[0]);
	ii++;
	}
	putchar(buf[0]);
	id_down=atoi(down_pin);
	if(id_down<=0){
	bad_down=1;
	}
	read ( newsock , down_pin2 , 100000);
	printf("%s",down_pin2);
	down=1;
	bool=1;
	}}}}}}}}}


	if(buf[0] == 'd'){
	read ( newsock , buf , 1);
	putchar(buf[0]);
	if(buf[0] == '_'){
	read ( newsock , buf , 1);
	putchar(buf[0]);
	if(buf[0] == 'i'){
	read ( newsock , buf , 1);
	putchar(buf[0]);
	if(buf[0] == 'd'){
	read ( newsock , buf , 1);
	putchar(buf[0]);
	if(buf[0] == '='){
	read ( newsock , buf , 1);
	putchar(buf[0]);
	while(buf[0]!= ' '){
	delete_pin[iii]=buf[0];
	read ( newsock , buf , 1);
	putchar(buf[0]);
	iii++;
	}
	putchar(buf[0]);
	id_delete=atoi(delete_pin);
	if(id_delete<=0){
	bad_delete=1;
	}
	delete=1;
	bool=1;
	if(read ( newsock , delete_pin2 , 1000000)< 0)
	bad_delete=1;
	printf("%s",delete_pin2);
	}}}}}


	if ((buf[0] == 'h') && (post==1)){
	read ( newsock , buf , 1);
	putchar(buf[0]);
	if(buf[0] == ':'){
	read ( newsock , buf , 1);
	putchar(buf[0]);
	if(buf[0] == ' '){
	read ( newsock , buf , 1);
	while(buf[0]!= '\r'){
	putchar(buf[0]);
	length[i]=buf[0];
	read ( newsock , buf , 1);
	i++;}
	len=atoi(length);
	if(len==0)
	lathos=1;
	}}}


	else if((buf[0] == '\n') && (post==1)){
	read ( newsock , buf , 1);
	putchar(buf[0]);
	if(buf[0] == '\r'){
	read ( newsock , buf , 1);
	putchar(buf[0]);
	if(buf[0] == '\n'){
	putchar(buf[0]);
	arxeio=malloc(len*sizeof(char));
	//////////////////
	int pa=0,souma=0,pa1=0,phre=0;char souma_p[128];
	if (len >= 129){
	while(souma <= len-1){
	phre=read ( newsock , souma_p , 128);
	if(phre < 0)
	{
		servererror=1;
		break;
	}
	for(pa=0;pa<phre;pa++){
	arxeio[pa1]=souma_p[pa];
	pa1++;
	}
	souma=souma+phre;
	}}
	else{
	phre=read ( newsock , arxeio , len);
	if(phre < 0)
	servererror=1;
	}
	bool=1;
}}}
}


if(post == 1 ){
time(&now);
current = localtime(&now);
t = asctime(current);
t[strlen(t)-1] = '\0';
printf("[%s] Client [%s] requested a file upload with %d content length\n",t,IP_ptr,len);
pthread_mutex_lock(&counter_lock) ;
counter++;
pthread_mutex_unlock(&counter_lock) ;
Needle *needle=NULL;
NeedleIndex	needle_index;
list_t	new_needle_index;
int hash_index;
char badpost[10000];
int strbadpost=sprintf (badpost,"HTTP/1.0 400 Bad request\r\nServer: haystack_server v1.0\r\nConnection: close\r\n\r\n");
char lathospost[10000];
int strlathospost=sprintf (lathospost, "HTTP/1.0 500 Internal Server Error\r\nServer: haystack_server v1.0\r\nConnection: close \r\n\r\n");
pthread_mutex_lock(&counter_lock) ;
if(!lathos && servererror==0){
if((h_file_d = fopen(file_b, "r+b")) != NULL ){
	fseek(h_file_d,0,SEEK_END);
	needle_index.Id = counter;
	needle_index.status = 1;
	needle_index.start_offset = ftell(h_file_d);
	needle = malloc(sizeof(Needle) - 1 + len + 1);
	needle->Id = counter;
	needle->Status = 1;
	needle->Size = len;
	int meta=0;
	for(meta=0;meta<needle->Size;meta++)
	{
	needle->Data[meta]=	arxeio[meta];
	}
	fwrite(needle, sizeof(Needle) - 1 + len + 1, 1, h_file_d);
	free(needle);
	needle_index.end_offset = ftell(h_file_d) - 1;
	list_push_front(&needle_index_list, &needle_index, sizeof(NeedleIndex));
	hash_index = getHashIndex(needle_index.Id, &hashTable);
	new_needle_index = list_get_first_node(&needle_index_list);
	add_in_hash(&hashTable, new_needle_index, hash_index);
	fclose(h_file_d);
	}
	else
	servererror=1;
	}
pthread_mutex_unlock(&counter_lock) ;



if(post == 1 && servererror == 1){
time(&now);
current = localtime(&now);
t = asctime(current);
t[strlen(t)-1] = '\0';
printf("[%s] Client [%s] file upload failed due to some internal server error .\n",t,IP_ptr);
write ( newsock , lathospost , strlathospost);
}
else if(lathos==1){
printf("[%s] Client[%s] request errror .\n",t,IP_ptr);
write ( newsock , badpost , strbadpost);
}
else
{
	time(&now);
	current = localtime(&now);
	t = asctime(current);
	t[strlen(t)-1] = '\0';
	printf("[%s] Client [%s] file upload finished successfully .\n",t,IP_ptr);
	pthread_mutex_lock(&counter_lock) ;
	char okpost[10000],okpost2[10000];
	int pa;
	int strokpost=sprintf (okpost, "File successfully uploaded with id : %d",counter);
	int strokpost2=sprintf (okpost2, "HTTP/1.0 200 OK\r\nServer: haystack_server v1.0\r\nConnection: close\r\nContent-Type: text\r\nContent-Length: %d\r\n\r\nFile successfully uploaded with id : %d",strokpost,counter);
	for(pa=0;pa<strokpost2;pa++){
	write ( newsock ,&(okpost2[pa]),1);}
	pthread_mutex_unlock(&counter_lock) ;
}
}
///////////////////////////////////////////////////////////

else if(down == 1){
time(&now);
current = localtime(&now);
t = asctime(current);
t[strlen(t)-1] = '\0';
printf("[%s] Client [%s] requested downloading file with id : %d \n",t,IP_ptr,id_down);

list_t		*collision_list,
			tmp, ptr;
NeedleIndex	needle_index;
Needle *needle=NULL;
int	found = 0,servererror=0;
char not_found_down[10000];
int strnot_found_down=sprintf (not_found_down,"HTTP/1.0 404 Not Found\r\nServer: haystack_server v1.0\r\nConnection: close\r\n\r\n");
char lathosdown[10000];
int strlathosdown=sprintf (lathosdown, "HTTP/1.0 400 Bad request\r\nServer: haystack_server v1.0\r\nConnection: close\r\n\r\n");
char servererrormsg[10000];
int strservererror=sprintf (servererrormsg, "HTTP/1.0 500 Internal Server Error\r\nServer: haystack_server v1.0\r\nConnection: close\r\n\r\n");
pthread_mutex_lock(&counter_lock) ;
if(((h_file_d = fopen(file_b, "rb")) != NULL) && (id_down >= 1)){
	collision_list = get_from_hash_value(&hashTable, getHashIndex(id_down, &hashTable));
	if(*collision_list != NULL){
		tmp = *collision_list;
		do{
			list_node_get_object(tmp, &ptr, sizeof(list_t));
			list_node_get_object(ptr, &needle_index, sizeof(NeedleIndex));
			if(needle_index.Id == id_down){
				found = 1;
				break;
			}
			else{
				tmp = list_nextof(tmp);
			}
		}while(needle_index.Id != id_down && tmp != NULL);
	}

	if(found){
		needle = malloc(needle_index.end_offset - needle_index.start_offset + 1);
		fseek(h_file_d, needle_index.start_offset, SEEK_SET);
		if((fread(needle, needle_index.end_offset - needle_index.start_offset + 1, 1, h_file_d) != 1))
		{
			servererror=1;
		}
		char okdown[10000];char okdown2[10000];
		int strokdown=sprintf(okdown, "HTTP/1.0 200 OK\r\nConnection: close\r\nContent-Type: image/jpeg\r\nContent-Length: ");
		strokdown=strokdown+needle->Size;
		int strokdown2=sprintf(okdown2, "HTTP/1.0 200 OK\r\nConnection: close\r\nContent-Type: image/jpeg\r\nContent-Length: %d\r\n\r\n",needle->Size);

		write ( newsock , &okdown2 , strokdown2);

		int pa=0,souma=0,pa1=0,phre=128,phre1=128;char souma_p[128];
		if((needle->Size) >= 129)
		{
		while(souma <= needle->Size-1){
		for(pa=0;pa<phre1;pa++){
			souma_p[pa]=needle->Data[pa1];
			pa1++;
		}
		phre1=write ( newsock , souma_p , phre);
		souma=souma+phre1;
		}
		}
		else{
		write ( newsock , needle->Data , needle->Size);
		}
		time(&now);
		current = localtime(&now);
		t = asctime(current);
		t[strlen(t)-1] = '\0';
		printf("[%s] Client [%s] successfully finished downloading file with id : %d . Total bytes tranfered : %d .\n",t,IP_ptr,id_down,needle->Size);
		free(needle);
	}
	else
		non_found_down=1;
	}
	fclose(h_file_d);
pthread_mutex_unlock(&counter_lock) ;

if(down == 1 && non_found_down == 1 ){
printf("[%s] Client[%s] File with id : %d was not found .\n",t,IP_ptr,id_down);
write ( newsock ,not_found_down,strnot_found_down);
}

else if(down == 1 && bad_down == 1){
printf("[%s] Client[%s] request errror .\n",t,IP_ptr);
write ( newsock , lathosdown , strlathosdown);
}

else if(down == 1 &&  servererror == 1){
printf("[%s] Client[%s] File with id : %d could not be downloaded due to some internal server error .\n",t,IP_ptr,id_down);
write ( newsock , servererrormsg , strservererror);
}

}

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////

else if(delete == 1){
time(&now);
current = localtime(&now);
t = asctime(current);
t[strlen(t)-1] = '\0';
printf("[%s] Client [%s] requested deletion of file with id : %d .\n",t,IP_ptr,id_delete);

list_t                *collision_list,
                      tmp, ptr, prev;
NeedleIndex        needle_index, *needle_index_ptr=NULL;
Needle *needle=NULL;
char okdelete[10000],okdelete2[10000];
int        found = 0, servererror = 0, index;
int strokdelete=sprintf (okdelete, "File with id : %d was succesfully deleted",id_delete);
int strokdelete2=sprintf (okdelete2, "HTTP/1.0 200 OK\r\nServer: haystack_server v1.0\r\nConnection: close\r\nContent-Type: text\r\nContent-Length: %d\r\n\r\nFile with id : %d was succesfully deleted",strokdelete,id_delete);
char not_found_delete[10000];
int strnot_found_delete=sprintf (not_found_delete,"HTTP/1.0 404 Not Found\r\nServer: haystack_server v1.0\r\nConnection: close\r\n\r\n");
char lathosdelete[10000];
int strlathosdelete=sprintf (lathosdelete, "HTTP/1.0 400 Bad request\r\nServer: haystack_server v1.0\r\nConnection: close\r\n\r\n");
char server_error_delete[10000];
int str_server_error_delete=sprintf (server_error_delete, "HTTP/1.0 500 Internal Server Error\r\nServer: haystack_server v1.0\r\nConnection: close\r\n\r\n");
pthread_mutex_lock(&counter_lock) ;
        if((h_file_d = fopen(file_b, "r+b")) == NULL ){
                servererror = 1;
        }
        if(servererror != 1 && bad_delete==0){
                found = 0;
                index = getHashIndex(id_delete, &hashTable);
                collision_list = get_from_hash_value(&hashTable, index);
                if(*collision_list != NULL){
                        tmp = *collision_list;
                        prev = NULL;
                        do{
                                list_node_get_object(tmp, &ptr, sizeof(list_t));
                                list_node_get_object(ptr, &needle_index, sizeof(NeedleIndex));
                                if(needle_index.Id == id_delete){
                                        found = 1;
                                        break;
                                }
                                else{
                                        prev = tmp;
                                        tmp = list_nextof(tmp);
                                }
                        }while(needle_index.Id != id_delete && tmp != NULL);
                }

                if(found){
                        needle_index_ptr = (NeedleIndex *)(ptr->data);
                        needle_index_ptr->status = 0;
                        needle = malloc(needle_index.end_offset - needle_index.start_offset + 1);
                        fseek(h_file_d, needle_index.start_offset, SEEK_SET);
                        fread(needle, needle_index.end_offset - needle_index.start_offset + 1, 1, h_file_d);
                        needle->Status = 0;
                        fseek(h_file_d, needle_index.start_offset, SEEK_SET);
                        fwrite(needle, needle_index.end_offset - needle_index.start_offset + 1, 1,h_file_d);
                        //printf("ID: %d - Status: %d - Size: %d - Data: %s\n", needle->Id,needle->Status, needle->Size, needle->Data);
                        free(needle);
                        remove_from_hash(&hashTable, index, prev);
                }
                fclose(h_file_d);
        }

pthread_mutex_unlock(&counter_lock) ;


if(delete == 1 && bad_delete == 1){
printf("[%s] Client[%s] request errror .\n",t,IP_ptr);
write ( newsock , lathosdelete , strlathosdelete);
}

else if (delete == 1 && servererror == 1){
printf("[%s] Client [%s] File with id : %d could not be deleted due to some internal server error .\n",t,IP_ptr,id_delete);
write ( newsock , server_error_delete , str_server_error_delete);
}

else if(delete == 1 && found == 0){
printf("[%s] Client [%s] File with id : %d was not found .\n",t,IP_ptr,id_delete);
write ( newsock ,not_found_delete,strnot_found_delete);
}

else{
time(&now);
current = localtime(&now);
t = asctime(current);
t[strlen(t)-1] = '\0';
printf("[%s] Client [%s] File with id : %d was successfully deleted .\n",t,IP_ptr,id_delete);
write ( newsock , okdelete2 , strokdelete2);
}
}


///////////////////////////////////////////////////////

printf ( "Closing connection.\n " ) ;
pthread_mutex_lock(&number_threads_lock);
number_threads--;
pthread_mutex_unlock(&number_threads_lock);
pthread_mutex_lock(&mtx);
pthread_cond_signal(&cvar);
pthread_mutex_unlock(&mtx);
close ( newsock );
pthread_exit(NULL);
return 0;
}



void sigchld_handler( int sig )
{
	int i=0;
	FILE *offset_file_d,*h_file_d;
	NeedleIndex	needle_index;
	bin_nums bn;
	if(sig == SIGINT){
pthread_mutex_lock(&number_threads_lock);
if(number_threads >=1 ){
printf("The number of active Threads: %d\n",number_threads);
pthread_mutex_lock(&mtx);
for(i=0;i<number_threads;i++)
pthread_cond_wait(&cvar,&mtx);
pthread_mutex_unlock(&mtx);
}
pthread_mutex_unlock(&number_threads_lock);
if ( (pthread_cond_destroy(&cvar)) >=1 ){
perror( "pthread_cond_destroy") ; exit (1) ;}
}
pthread_mutex_lock(&counter_lock) ;
time(&now);
current = localtime(&now);
char *t = asctime(current);
t[strlen(t)-1] = '\0';
printf("[%s] Server is shutting down. Server will not accept any other clients\n", t);
printf("[%s] Waiting for running clients to be served...\n", t);
if((offset_file_d = fopen(AUX_FILE, "w+b")) == NULL){
		perror("fopen");
		exit(FILE_ERROR);
	}

	/* copy ACTIVE needle indexes from needle_index_list to HD */
	while(!list_empty(needle_index_list)){
		list_pop_front(&needle_index_list, &needle_index, sizeof(NeedleIndex));
		if(needle_index.status == 1){
			fwrite(&needle_index, sizeof(NeedleIndex), 1, offset_file_d);
		}
	}

	printf("[%s] Cleaning up...\n", t);

	fclose(offset_file_d);
	pthread_mutex_unlock(&counter_lock) ;
	pthread_mutex_lock(&counter_lock) ;
	if((h_file_d = fopen(file_b, "r+b")) == NULL){
		perror("fopen");
		exit(FILE_ERROR);
	}
	bn.mag_num=magic_number;
	bn.next_id=counter;
	fwrite(&bn,sizeof(bin_nums),1,h_file_d);
	fclose(h_file_d);
	pthread_mutex_unlock(&counter_lock) ;

	list_destroy(&needle_index_list);
	hash_destroy(&hashTable);
	free(file_b);

	printf("[%s] Bye!\n", t);

pthread_exit(NULL);
}

void perror_exit( char * message ) {
	perror ( message ) ;
	exit ( EXIT_FAILURE ) ;
}
