#include "thread.h"

static pthread_cond_t ok_to_send = PTHREAD_COND_INITIALIZER;
static pthread_cond_t ok_to_receive = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t ok_to_send_mut =  PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t ok_to_receive_mut = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t access_send_list = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t access_receive_list = PTHREAD_MUTEX_INITIALIZER;


void  shutdown_manager(void * args){

  struct shutdown *stop = args;
  if(args == NULL){
    return;
  }

  pthread_cancel(*(stop->keyboard));
  pthread_cancel(*(stop->printer));
  pthread_cancel(*(stop->receiver));
  pthread_cancel(*(stop->sender)); 

  pthread_mutex_destroy(&ok_to_send_mut);
  pthread_mutex_destroy(&ok_to_receive_mut);
  pthread_mutex_destroy(&access_send_list);
  pthread_mutex_destroy(&access_receive_list);

  pthread_cond_destroy(&ok_to_send);
  pthread_cond_destroy(&ok_to_receive);

  close(stop->sock_fd);



}

void *sender_input(void * args){
  while(1){
    struct UDP_params *sendParams = args;
    char messageTx[MSG_MAX_LEN];
    fgets(messageTx, MSG_MAX_LEN, stdin);

    pthread_mutex_lock(&access_send_list);
    {
      List_add(sendParams->pList, &messageTx);
    }
    pthread_mutex_unlock(&access_send_list);

    if(sendParams->pList->pCurrentNode->pItem == NULL){
      return NULL;
    }
    pthread_mutex_lock(&ok_to_send_mut);
    { 
        int c, d;
        for (c = 1; c <= 3276; c++)
            for (d = 1; d <= 3276; d++)
            {}  
        pthread_cond_signal(&ok_to_send);
    }
    pthread_mutex_unlock(&ok_to_send_mut);

    // printf("\nreached here after join: %c\n", messageTx[0]);


  }  


} 

void *UDP_send(void * args){
  while(1){
    pthread_mutex_lock(&ok_to_send_mut);
    {
        pthread_cond_wait(&ok_to_send,&ok_to_send_mut);
    }
    pthread_mutex_unlock(&ok_to_send_mut);
    
    struct UDP_params *sendParams = args;
    char *msg_to_send = List_curr(sendParams->pList);
    ssize_t bytes;
    bytes = sendto( sendParams->sock_fd, msg_to_send, strlen(msg_to_send), 0, (struct sockaddr *) sendParams->peer, sizeof(struct sockaddr_in));

    if ((strlen(msg_to_send) == 2) && (msg_to_send[0] == '!'))
    {
      printf("\nEnding connection please reconnect to continue talking\n");
      shutdown_manager(sendParams->shutdown_args);
    }

    if (bytes < 0) {
      printf("Error - sendto error: %s\n", strerror(errno));
    }

    pthread_mutex_lock(&access_send_list);
    {
      List_remove(sendParams->pList);
    }  
    pthread_mutex_unlock(&access_send_list); 
  }  
}

void* UDP_receive(void * args){
  while(1){
    struct UDP_params *receiveParams = args;
    char messageRx[MSG_MAX_LEN];

    ssize_t bytesRx = recvfrom(receiveParams->sock_fd, messageRx, 1024,0, NULL, NULL);
    if (bytesRx < 0) {
      printf("Error - recvfrom error: %s\n", strerror(errno));
    }
    
    int terminateIdx = (bytesRx < MSG_MAX_LEN) ? bytesRx : MSG_MAX_LEN - 1;
    messageRx[terminateIdx] = 0;

    pthread_mutex_lock(&access_receive_list);
    {
      List_add(receiveParams-> pList, &messageRx);
    }
    pthread_mutex_unlock(&access_receive_list);


    pthread_mutex_lock(&ok_to_receive_mut);
    {
      int c, d;    
      for (c = 1; c <= 3276; c++)
        for (d = 1; d <= 3276; d++)
        {}  
      pthread_cond_signal(&ok_to_receive);
    }
    pthread_mutex_unlock(&ok_to_receive_mut);

    if ((strlen(messageRx) == 2) && (messageRx[0] == '!'))
    {
      // printf("\nthis is the length: %ld\n", strlen(messageRx));
      // printf("\nthis is the character: %c\n", messageRx[0]);
      printf("\nPeer ended the connection please reconnect to continue talking!\n");
      shutdown_manager(receiveParams->shutdown_args);


    }
  }  
}
//ssh -p 24 -C wrl1@csil-cpu8.csil.sfu.ca
void *printer_output(void * pList){
  while(1){
    pthread_mutex_lock(&ok_to_receive_mut);
    {
      pthread_cond_wait(&ok_to_receive,&ok_to_receive_mut);
    }
    pthread_mutex_unlock(&ok_to_receive_mut);  
    struct List_s * receive_list = pList;
    if (receive_list== NULL)
    {
      printf("linked list allocation unsuccesful");
      pthread_exit();
    }
    
    char* UDP_receive = receive_list->pCurrentNode->pItem;


    if (receive_list->pCurrentNode-> pItem )
    {
        printf("Peer says: %s\n", (char*)UDP_receive );
    }
    pthread_mutex_lock(&access_receive_list);
    {
    List_remove (receive_list);
    }
    pthread_mutex_unlock(&access_receive_list);

    if ((strlen(UDP_receive) == 2) && (UDP_receive[0] == '!'))
    {
      pthread_exit(NULL);
    }
  }
}

int hostname_to_ip(const char *hostname , char *ip, const char* arg3)
{
	int sockfd;  
	struct addrinfo hint, *peerinfo, *pr;
	struct sockaddr_in *cntanr;
	int ipchk;

	memset(&hint, 0, sizeof hint);
	hint.ai_family = AF_UNSPEC; 
	hint.ai_socktype = SOCK_DGRAM;

	if ( (ipchk = getaddrinfo( hostname , arg3 , &hint , &peerinfo)) != 0) 
	{
		fprintf(stderr, "errorcheck: %s\n", gai_strerror(ipchk)); //finds the error from the input
		return 1;
	}

	for(pr = peerinfo; pr != NULL; pr = pr->ai_next) 
	{
		cntanr = (struct sockaddr_in *) pr->ai_addr;
		strcpy(ip , inet_ntoa( cntanr->sin_addr ) );
	}
	
	freeaddrinfo(peerinfo); 
	return 0;
}

int main(int argc, char const *argv[])
{
  int sock_fd;
  struct List_s *send_list = List_create();
  struct List_s *receive_list = List_create();
  int keyboard_join = 0;
  int receiver_join = 0;
  if (send_list==NULL || receive_list == NULL)
  {
    printf("list.h implementation is incorrect\n");
    return -1;
  }

  if(argc < 4){
    printf("incorrect number of arguments!\n");
    return -1;
  }

  //local and remote ports 
	unsigned long local_port;
	unsigned long remote_port;
  if (argv[1] == NULL|| argv[3] ==NULL)
  {
    printf("Remote Ports are null\n");
    return -1;
  }

  local_port = strtoul(argv[1], NULL, 0);
  remote_port = strtoul(argv[3], NULL, 0);
  if (local_port < 1 || local_port > 65535) {
    printf("Error - invalid local port '%s'\n", argv[1]);
    return -1;
  }
  if (remote_port < 1 || remote_port > 65535) {
    printf("Error - invalid remote port '%s'\n", argv[3]);
    return -1;
  }


  pthread_t keyboard_thread, sender_thread, receiver_thread, printer_thread;
  int keyboard,sender,receiver,printer;

  struct sockaddr_in local_addr;
  struct sockaddr_in peer_addr;

  local_addr.sin_family = AF_UNSPEC;
  local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  local_addr.sin_port = htons(local_port);

  sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

  if (bind(sock_fd, (struct sockaddr *)(&local_addr),
           sizeof(local_addr)) < 0) {
    printf("Error -- failed to bind socket: %s\n", strerror(errno));
    return -1;
  }
  const char *hostname = argv[2];
  if (hostname == NULL){
    printf("Please provide a hostname\n" );
    return -1;

  }
	char ip[30];
	
	hostname_to_ip(hostname , ip, argv[3]); // return the ip based on the hostname input 
  peer_addr.sin_family = AF_UNSPEC;
  peer_addr.sin_port = htons(remote_port);

  if (inet_aton(ip, &peer_addr.sin_addr) == 0) {
    printf("Error for %s -- invalid remote address for  '%s'\n", argv[2] ,ip);
    return -1;
  }

  struct shutdown shutdownParams;
  shutdownParams.keyboard = &keyboard_thread;
  shutdownParams.printer = &printer_thread;
  shutdownParams.receiver = &receiver_thread;
  shutdownParams.sender = &sender_thread;
  shutdownParams.sock_fd = sock_fd;


  struct UDP_params sendParams;
  sendParams.sock_fd = sock_fd;
  sendParams.peer = &peer_addr;
  sendParams.pList = send_list;
  sendParams.shutdown_args  = &shutdownParams;


  struct UDP_params receiveParams;
  receiveParams.sock_fd = sock_fd;
  receiveParams.peer = &peer_addr;
  receiveParams.pList = receive_list;
  receiveParams.shutdown_args = &shutdownParams;
  


  keyboard  = pthread_create(&keyboard_thread, NULL, sender_input, (void *)&sendParams);
  sender = pthread_create(&sender_thread, NULL, UDP_send, (void *)&sendParams);
  receiver = pthread_create(&receiver_thread, NULL, UDP_receive, (void *)&receiveParams);
  printer = pthread_create(&printer_thread, NULL, printer_output, (void *)receive_list);

  if((keyboard != 0) || (sender != 0) || (receiver != 0) || (printer != 0)){
    printf("\npthread create failed\n");
    return -1;
  }


  keyboard_join = pthread_join(keyboard_thread, NULL);
  receiver_join = pthread_join(receiver, NULL);

  if((keyboard_join != 0)|| (receiver_join != 0)){
    
    printf("\nThe pthread join failed\n");
    return -1;
  }

  return 0;
}