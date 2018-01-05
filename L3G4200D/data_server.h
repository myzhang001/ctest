/*---------------------------------------------------------------------
Based on: blog.csdn.net/dlutbrucezhang/article/details/8880131

---------------------------------------------------------------------*/
#ifndef __DATA_SERVER__
#define __DATA_SERVER__

#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILE_SERVER_PORT 5555
#define SERVER_LISTEN_BACKLOG 5


int svrsock_desc; //server socekt descriptor 
int cltsock_desc; //client socket descriptor
struct sockaddr_in server_addr;
int rec_len;
struct timeval tm_start,tm_end;
int time_use;


//---- addr. for client
struct sockaddr_in client_addr;



/*---------------------------------------------
Prepare data server and start listening
Return value:
	0   OK
	<0  FAILS
----------------------------------------------*/
int prepare_data_server(void)
{
   //----- set server address and port ------
   bzero(&server_addr,sizeof(server_addr));
   server_addr.sin_family = AF_INET; //Address Family IP version 4
   server_addr.sin_addr.s_addr = htons(INADDR_ANY); //host to net short
   server_addr.sin_port = htons(FILE_SERVER_PORT);

   //----- get a socket desc. with specified protocol type -----
   svrsock_desc = socket(AF_INET,SOCK_STREAM,0); //SOCK_STREAM -- connection oriented TCP protocol,  0--or IPPROTO_IP is IP protocol
   if(svrsock_desc < 0)
   {
	printf("create server socket failed!\n");
	return -1;
   }

   //-----bind socket desc. with IP address,   int bind(int sock, struct sockaddr *addr, int addrLen) -----
   if( bind(svrsock_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0 )
   {
	printf("Failed to bind server socket with port %d!\n",FILE_SERVER_PORT);
	return -1;
    }

   //-----start listening,  int listen(int sock, int backlog) -----
   if( listen(svrsock_desc,SERVER_LISTEN_BACKLOG) != 0)
   {
	printf("listen to port %d failed!\n",FILE_SERVER_PORT);
	return -1;
   }

   return 0;
}


/*---------------------------------------------
Accept a client and return the socket descriptor
Return value:
	>0   OK (socket descriptor)
	<0  FAILS
----------------------------------------------*/
int accept_data_client(void)
{
   int ret_socket;
   socklen_t client_addr_len = sizeof(client_addr);

   //---- accept a client with socket desc, with address
   //---- int accept( int sock, struct sockaddr *addr, int *addrLen ) ------
   ret_socket = accept(svrsock_desc, (struct sockaddr*)&client_addr, &client_addr_len);

   return ret_socket;
}

/*---------------------------------------------
Send data to client, assert cltsock_desc first!
Return value:
	>0  OK (lenth of sent data)
	<0  FAILS
----------------------------------------------*/
int send_client_data(uint8_t *data, int len)
{
   int ret;

   //----- send data to client  ------
   //----- int send( int sock, const void *msg, int len, unsigned int flag ) --------
   ret=send(cltsock_desc, data, len,0);

   return ret;
}


/*- --------------------------------------------
Close both server and client socket descriptors.
---- ------------------------------------------*/
void close_data_service(void)
{
	close(cltsock_desc);
	close(svrsock_desc);
}


#endif
