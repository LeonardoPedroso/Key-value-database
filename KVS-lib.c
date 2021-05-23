#include "KVS-lib.h" // include header
#include "KVS-lib-com.h"
#include "KVS-lib-MACROS.h"

// ---------- Global variables ----------
extern int clientSock; // client socket
extern struct sockaddr_un server_sock_addr; // server socket address

int establish_connection (char * group_id, char * secret){
    // Create client socket    
    clientSock = socket(AF_UNIX, SOCK_STREAM,0);
    // Catch errors creating socket
    if (clientSock == -1){
        perror("Error creating socket");
        return ESTBL_CONN_ERROR_CREATING_SOCK;
    }
    // Setup server connection
    server_sock_addr.sun_family = AF_UNIX;
    strcpy(server_sock_addr.sun_path, KVS_LOCAL_SERVER_ADDR);
    if(connect(clientSock, (struct sockaddr *) &server_sock_addr, sizeof(struct sockaddr_un)) == -1){
        perror("Error connecting to server");
        return ESTBL_CONN_ERROR_CONNECTION_SERVER;
    }
    // Query KVS Local server for authentication
    //(int msgId, char * str1, char * str2, uint64_t len2, char * str3, uint64_t * len3)
    switch (queryKVSLocalServer(getpid(),group_id, secret, strlen(secret)+1, NULL,NULL)){
        case STATUS_OK:
            return ESTBL_CONN_SUCCESS;
        case STATUS_ACCSS_DENIED:
            return ESTBL_CONN_ACCSS_DENIED;
        case QUERY_COM_ERROR:
            return ESTBL_CONN_ERROR_COM_SERVER;
        default:
            return ESTBL_CONN_ERROR_COM_SERVER;
    }
}

int close_connection(){
    // Send disconnection request
    int status = queryKVSLocalServer(MSG_ID_CLOSE_CONN, NULL, NULL,0, NULL,NULL);
    // Even if communication with the server is not possible connection is closed
    // The only way an error arises is if the server is either disconnected or 
    // unable to answer the query
    
    clientSock = DISCONNECTED_SOCKET;
    switch(status){
        case STATUS_OK:
            return CLOSE_CONN_SUCCESS; 
        case QUERY_ERROR_DISCONNECTED_SOCK:
            return CLOSE_CONN_ERROR_DISCONNECTED_SOCK;
        default:
            return CLOSE_CONN_ERROR_COM_SERVER;
    }
}