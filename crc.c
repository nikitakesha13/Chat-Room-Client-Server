#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "interface.h"
#include "additional.h"

int buffercapacity = MAX_DATA;


/*
 * TODO: IMPLEMENT BELOW THREE FUNCTIONS
 */
int connect_to(const char *host, const int port);
struct Reply process_command(const int sockfd, char* command);
void process_chatmode(const char* host, const int port);

int main(int argc, char** argv) 
{
	if (argc != 3) {
		fprintf(stderr,
				"usage: enter host address and port number\n");
		exit(1);
	}

    display_title();

    int sockfd = connect_to(argv[1], atoi(argv[2]));
	// printf("SOCKFD: %d\n", sockfd);

	while (1) {
		
		// int sockfd = connect_to(argv[1], atoi(argv[2]));

		char command[MAX_DATA];
        get_command(command, MAX_DATA);

		touppercase(command, strlen(command)-1);

		struct Reply reply = process_command(sockfd, command);
		display_reply(command, reply);
		
		touppercase(command, strlen(command) - 1);
		if (strncmp(command, "JOIN", 4) == 0) {
			printf("Now you are in the chatmode\n");
			process_chatmode(argv[1], reply.port);
		}

		memset(command, 0, MAX_DATA);
	
		// close(sockfd);
    }

	close(sockfd);

    return 0;
}

/*
 * Connect to the server using given host and port information
 *
 * @parameter host    host address given by command line argument
 * @parameter port    port given by command line argument
 * 
 * @return socket fildescriptor
 */
int connect_to(const char *host, const int port)
{
	// ------------------------------------------------------------
	// GUIDE :
	// In this function, you are suppose to connect to the server.
	// After connection is established, you are ready to send or
	// receive the message to/from the server.
	// 
	// Finally, you should return the socket fildescriptor
	// so that other functions such as "process_command" can use it
	// ------------------------------------------------------------

    // below is just dummy code for compilation, remove it
	int sockfd;
	char port_str[20];
	sprintf(port_str, "%d", port);
	// client side 
	struct addrinfo hints, *res;

	// first, load up address structs with getaddrinfo():
	memset(&hints, 0, sizeof (hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	int status;
	//getaddrinfo("www.example.com", "3490", &hints, &res);
	if ((status = getaddrinfo (host, port_str, &hints, &res)) != 0) { // looking up IP address from name
		// cerr << "getaddrinfo: " << gai_strerror(status) << endl;
		perror("client: getaddrinfo");
		exit (-1);
	}

	// make a socket:
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd < 0){
		perror ("cliet: Cannot create socket");	
		exit (-1);
	}

	// connect!
	if (connect(sockfd, res->ai_addr, res->ai_addrlen)<0){ // client is attempting the server connection
		perror ("client: Cannot Connect");
		exit (-1);
	}
	// printf("THE socket from client side: %d\n", sockfd);
	freeaddrinfo(res);
	// int sockfd = -1;
	return sockfd;
}

/* 
 * Send an input command to the server and return the result
 *
 * @parameter sockfd   socket file descriptor to commnunicate
 *                     with the server
 * @parameter command  command will be sent to the server
 *
 * @return    Reply    
 */
struct Reply process_command(const int sockfd, char* command)
{
	// ------------------------------------------------------------
	// GUIDE 1:
	// In this function, you are supposed to parse a given command
	// and create your own message in order to communicate with
	// the server. Surely, you can use the input command without
	// any changes if your server understand it. The given command
    // will be one of the followings:
	//
	// CREATE <name>
	// DELETE <name>
	// JOIN <name>
    // LIST
	//
	// -  "<name>" is a chatroom name that you want to create, delete,
	// or join.
	// 
	// - CREATE/DELETE/JOIN and "<name>" are separated by one space.
	// ------------------------------------------------------------

	// ------------------------------------------------------------
	// GUIDE 2:
	// After you create the message, you need to send it to the
	// server and receive a result from the server.
	// ------------------------------------------------------------

	// printf("%s\n", command);
	send(sockfd, command, strlen(command), 0);

	// ------------------------------------------------------------
	// GUIDE 3:
	// Then, you should create a variable of Reply structure
	// provided by the interface and initialize it according to
	// the result.
	//
	// For example, if a given command is "JOIN room1"
	// and the server successfully created the chatroom,
	// the server will reply a message including information about
	// success/failure, the number of members and port number.
	// By using this information, you should set the Reply variable.
	// the variable will be set as following:
	//
	// Reply reply;
	// reply.status = SUCCESS;
	// reply.num_member = number;
	// reply.port = port;
	// 
	// "number" and "port" variables are just an integer variable
	// and can be initialized using the message fomr the server.
	//
	// For another example, if a given command is "CREATE room1"
	// and the server failed to create the chatroom becuase it
	// already exists, the Reply varible will be set as following:
	//
	// Reply reply;
	// reply.status = FAILURE_ALREADY_EXISTS;
    // 
    // For the "LIST" command,
    // You are suppose to copy the list of chatroom to the list_room
    // variable. Each room name should be seperated by comma ','.
    // For example, if given command is "LIST", the Reply variable
    // will be set as following.
    //
    // Reply reply;
    // reply.status = SUCCESS;
    // strcpy(reply.list_room, list);
    // 
    // "list" is a string that contains a list of chat rooms such 
    // as "r1,r2,r3,"
	// ------------------------------------------------------------

	// REMOVE below code and write your own Reply.
	struct Reply reply;
	int bytes = recv(sockfd, &reply, sizeof(reply), 0);
	if (bytes < 0){
		perror("client: Server side terminated abnormally\n");
		return ;
	}
	else if (bytes == 0){
		perror("client: Could not read anything\n");
		return ;
	}
	return reply;
}

/* 
 * Get into the chat mode
 * 
 * @parameter host     host address
 * @parameter port     port
 */

// static int bytes = 0;

pthread_t send_msg;
pthread_t recv_msg;

void send_handle_loop(int sockfd){
	char msg[MAX_DATA];
	while(1){
		memset(msg, 0, MAX_DATA);
		const int size = MAX_DATA;
		get_message(msg, size);
		send(sockfd, msg, strlen(msg), 0);
		if (!strcmp(msg, "Q")){
			close(sockfd);
			pthread_cancel(recv_msg);
			pthread_cancel(pthread_self());
			pthread_exit(NULL);
			// return NULL;
		}
	}
}

void recv_handle_loop(int sockfd){
	char buffer[buffercapacity];
	while(1){
		memset(buffer, 0, sizeof(buffer));
		int bytes = recv(sockfd, buffer, buffercapacity, 0);
		if (bytes > 0){
			display_message(buffer);
			printf("\n");
		}
		if (!strcmp(buffer, "Warnning: the chatting room is going to be closed...")){
			close(sockfd);
			pthread_cancel(send_msg);
			pthread_cancel(pthread_self());
			pthread_exit(NULL);
			// return NULL;
		}
	}
}

void process_chatmode(const char* host, const int port)
{
	// ------------------------------------------------------------
	// GUIDE 1:
	// In order to join the chatroom, you are supposed to connect
	// to the server using host and port.
	// You may re-use the function "connect_to".
	// ------------------------------------------------------------
	int sockfd = connect_to(host, port);
	// printf("The socket is %d\n", sockfd);
	// printf("The port number is: %d\n", port);

	pthread_create(&send_msg, NULL, send_handle_loop, sockfd);
	pthread_create(&recv_msg, NULL, recv_handle_loop, sockfd);
	pthread_join(send_msg, NULL); 
	pthread_join(recv_msg, NULL); 

	// ------------------------------------------------------------
	// GUIDE 2:
	// Once the client have been connected to the server, we need
	// to get a message from the user and send it to server.
	// At the same time, the client should wait for a message from
	// the server.
	// ------------------------------------------------------------
	

    // ------------------------------------------------------------
    // IMPORTANT NOTICE:
    // 1. To get a message from a user, you should use a function
    // "void get_message(char*, int);" in the interface.h file
    // 
    // 2. To print the messages from other members, you should use
    // the function "void display_message(char*)" in the interface.h
    //
    // 3. Once a user entered to one of chatrooms, there is no way
    //    to command mode where the user  enter other commands
    //    such as CREATE,DELETE,LIST.
    //    Don't have to worry about this situation, and you can 
    //    terminate the client program by pressing CTRL-C (SIGINT)
	// ------------------------------------------------------------
}
