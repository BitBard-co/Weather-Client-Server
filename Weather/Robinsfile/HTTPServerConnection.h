#ifndef __HTTPServerConnection_h_
#define __HTTPServerConnection_h_

#include "smw.h"
#include "TCPClient.h"

#define HTTPServer_MaxLineLength 1024
#define HTTPServer_BufferSize 128


typedef enum
{
    HTTPServerConnection_State_Init,
    HTTPServerConnection_State_ReadFirstLine,
    HTTPServerConnection_State_ReadHeaders,
    HTTPServerConnection_State_InvalidRequest

} HTTPServerConnection_State;


typedef int (*HTTPServerConnection_OnRequest)(void* _Context);

typedef struct
{
    TCPClient tcpClient;

    void* context;
    HTTPServerConnection_OnRequest onRequest;

    char* method;
    char* url;
    char* version;
    uint8_t* body;

    HTTPServerConnection_State state;

    uint8_t lineBuffer[HTTPServer_MaxLineLength];
    int lineLength;

    smw_task* task;

} HTTPServerConnection;


int HTTPServerConnection_Initiate(HTTPServerConnection* _Connection, int _FD);
int HTTPServerConnection_InitiatePtr(int _FD, HTTPServerConnection** _ConnectionPtr);

void HTTPServerConnection_SetCallback(HTTPServerConnection* _Connection, void* _Context, HTTPServerConnection_OnRequest _OnRequest);

void HTTPServerConnection_Dispose(HTTPServerConnection* _Connection);
void HTTPServerConnection_DisposePtr(HTTPServerConnection** _ConnectionPtr);

#endif //__HTTPServerConnection_h_
