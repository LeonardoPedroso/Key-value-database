#include "KVSLocalServer.h"
#include "KVSLocalServer-com.h"
#include "KVS-lib-MACROS.h"


// ---------- Global variables ----------
CLIENT * clients = NULL; // Pointer to the first element of the linked list of clients 
GROUP * groups = NULL; // Pointer to the first element of the linked list of groups 

int main(){
    
    // ---------- Setup server variables ----------
    int server_sock; // fd of rcv socket
    struct sockaddr_un server_sock_addr; // struct addr of sever socket
    server_sock_addr.sun_family = AF_UNIX; // set socket family type
    
    // ---------- Setup server socket ----------
    server_sock = socket(AF_UNIX, SOCK_STREAM, 0); // create server socket
    // Ignore socket disconnection signal from client, which is handled when read returns -1
    signal(SIGPIPE, SIG_IGN); 
    // Catch error creating reception socket
    if (server_sock == -1){
        perror("Error creating reception socket");
        exit(-1);
    }
    printf("Reception socket created\n");
    
    // ---------- Bind server socket ----------
    strcpy(server_sock_addr.sun_path, KVS_LOCAL_SERVER_ADDR); // set socket to known address
    // Catch error binding socket to address
    if( bind(server_sock, (struct sockaddr *) &server_sock_addr, sizeof(struct sockaddr_un)) == -1){
        perror("Error binding address to socket");
        exit(-1);
    }
    printf("Reception socket binded to address %s\n", server_sock_addr.sun_path);
    
    // ---------- Listen to incoming connections ----------
    // Catch error listening to connections
    if( listen(server_sock,KVS_LOCAL_SERVER_BACKLOG) == -1){
        perror("Error listening to incoming connections");
        exit(-1);
    }
    printf("Reception socket is listening to connections.\n");

    // ---------- Wait and handle connections on thread ----------
    pthread_t serverThread;
    pthread_create(&serverThread, NULL, &KVSLocalServerThread, &server_sock);


    // [ADD SERVER CONSOLE BELOW]
    while(1){}


    exit(0);

}

// ---------- KVS Server thread functions ----------

void * KVSLocalServerThread(void * server_sock){
    // Variable to hold new socket when a new client connects
    int clientSocket;
    // Wait for connections and handle them 
    while(1){
        // Wait for a connection (the server socket is passed as an argument to KVSLocalServerThread)
        clientSocket = accept(*((int *) server_sock), NULL, NULL);
        // Catch error waiting for a connection
        if (clientSocket == -1){
            // [TO DO] DISTINGUISH BETWEEN ERRORS OR JUST SERVER CLOSE
            printf("Stopped main server thread.\n");
            break;
        }
        // Add client and handle it in a new thread
        // Catch errors handling new client
        if( handleClient(clientSocket) <0 ){
            printf("Error handling new client.\n");
            break;
        }
        printf("Accepted new connection.\n");
    }
    closeClients(); // Close connections to the clients, join repective threads, and free memory
    pthread_exit(NULL); // Close KVSServerThread
}

void * KVSLocalServerClientThread(void * client){
    // Allocate buffers
    char buffer1[MAX_STR_LENGTH];
    int buffer1Len;
    char buffer2[MAX_STR_LENGTH];
    int buffer2Len;
    int msgId = 0;

    // ---------- Authenticate client ----------
    if(rcvQueryKVSLocalServer(((CLIENT *)client)->clientSocket, &msgId, &buffer1[0], &buffer2[0])
    == RCV_QUERY_COM_ERROR){
        //[HANDLE UNCONTROLLED DISCONNECTION]
        pthread_exit(NULL); // Close KVSServerThread
    }
    // PID 
    ((CLIENT *)client)->PID = msgId;    
    // [CHECK AUTHENTICATION AUTH SERVER]
    // switch(authenticated)
    // case OK:
        printf("%s\n",buffer1);
        printf("%s\n",buffer2);
        ((CLIENT *)client)->connectivityStatus = CONN_STATUS_CONNECTED;
        ansQueryKVSLocalServer(((CLIENT *)client)->clientSocket,STATUS_OK,NULL);

    // Loop receiving and hadling queries
    while(1){
        if(rcvQueryKVSLocalServer(((CLIENT *)client)->clientSocket, &msgId, &buffer1[0], &buffer2[0])
        == RCV_QUERY_COM_ERROR){
            //[HANDLE UNCONTROLLED DISCONNECTION]
            pthread_exit(NULL); // Close KVSServerThread
        }
        switch(msgId){
        case MSG_ID_ESTBL_CONN:

            break;
        case MSG_ID_PUT_VAL:
        
            break;
        case MSG_ID_GET_VAL:
        
            break;
        case MSG_ID_DEL_VAL:
        
            break;
        case MSG_ID_REG_CB:
        
            break;
        case MSG_ID_CLOSE_CONN:
        
            break;
        
        default:
            break;
        }

    }
    //[HANDLE UNCONTROLLED DISCONNECTION]
    pthread_exit(NULL); // Close KVSServerThread
}

// ---------- Server and client mangement prototypes ----------

int handleClient(int clientSocket){
    // ---------- Allocate memory to new client ----------
    CLIENT * newClient = (CLIENT *) malloc(sizeof(CLIENT));
    // Catch allocation error 
    if(newClient == NULL){
        perror("Error alocating memory to new client");
        return ERROR_CLIENT_ALLOCATION;
    }
    // Store socket for communication with this client
    newClient->clientSocket = clientSocket;
    // Define connectivity status of client
    newClient->connectivityStatus = CONN_STATUS_NOT_AUTH;
    // Define connection time
    if(clock_gettime(CLOCK_REALTIME, &(newClient->connTime)) == -1 ) {
        perror( "Clock gettime error" );
        // Time is not critical so exit is overkill
    }
    // Add client to client list
    clientAdd(newClient);
    // ---------- Handle client in new thread ----------
    pthread_create(&(newClient->clientThread), NULL, &KVSLocalServerClientThread, (void *) newClient);
    return SUCCESS_CLIENT_HANDLE;
}

void clientAdd(CLIENT * newClient){
    // [IN MUTEX client region]
    // ---------- Add new client block to the linked list ----------
    // Check if pointer to the linked list is NULL (i.e. the new client is the first client)
    CLIENT * searchPointer = clients;
    if(searchPointer == NULL){
        clients = newClient;
    }else{
        // Iterate through the clients until the last, whcih does not point to other CLIENT block
        while(searchPointer->prox != NULL){
            searchPointer = searchPointer->prox;
        }
        searchPointer->prox = newClient;
    }
    // [OUT MUTEX client region]
}


void closeClients(){
    // ---------- Remove client block to the linked list ----------
    // [CUIDADO NO FUTURO COM POSSIVEIS PROBLEMAS DE SINCRONIZAÇÃO]
    CLIENT * searchPointer = clients;
    CLIENT * searchPointerPrev;
    // Check if pointer to the linked list is NULL (i.e. there are no connected clients)
    if(searchPointer == NULL){
       return;
    }
    // Iterate through the clients closing, joining, and freeing memory
    while(searchPointer != NULL){
        searchPointerPrev = searchPointer;
        searchPointer = searchPointer->prox;
        // Close socket 
        close(searchPointerPrev->clientSocket);
        // Wait for client thread
        pthread_join(searchPointerPrev->clientThread, NULL);
        // Free memory allocated for client
        free(searchPointerPrev);
    }
}

