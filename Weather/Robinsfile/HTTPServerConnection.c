#include "HTTPServerConnection.h"
#include <stdlib.h>
#include <stdio.h>

//-----------------Internal Functions-----------------

void HTTPServerConnection_TaskWork(void* _Context, uint64_t _MonTime);

//----------------------------------------------------

int HTTPServerConnection_Initiate(HTTPServerConnection* _Connection, int _FD)
{
    TCPClient_Initiate(&_Connection->tcpClient, _FD);

    _Connection->context = NULL;
    _Connection->onRequest = NULL;

    _Connection->method = NULL;
    _Connection->url = NULL;
    _Connection->version = NULL;
    _Connection->body = NULL;
    
    _Connection->state = HTTPServerConnection_State_Init;

    _Connection->lineLength = 0;

    _Connection->task = smw_createTask(_Connection, HTTPServerConnection_TaskWork);

    printf("HTTPServerConnection initiated on FD %d\n", _FD);

    return 0;
}

int HTTPServerConnection_InitiatePtr(int _FD, HTTPServerConnection** _ConnectionPtr)
{
    if(_ConnectionPtr == NULL)
        return -1;

    HTTPServerConnection* _Connection = (HTTPServerConnection*)malloc(sizeof(HTTPServerConnection));
    if(_Connection == NULL)
        return -2;

    int result = HTTPServerConnection_Initiate(_Connection, _FD);
    if(result != 0)
    {
        free(_Connection);
        return result;
    }

    *(_ConnectionPtr) = _Connection;

    return 0;
}

void HTTPServerConnection_SetCallback(HTTPServerConnection* _Connection, void* _Context, HTTPServerConnection_OnRequest _OnRequest)
{
    _Connection->context = _Context;
    _Connection->onRequest = _OnRequest;
}

void HTTPServerConnection_TaskWork(void* _Context, uint64_t _MonTime)
{
    HTTPServerConnection* _Connection = (HTTPServerConnection*)_Context;
    
    /*
        GET /index.html HTTP/1.1\r\n
        Host: example.com\r\n

    */

    switch(_Connection->state)
    {
        case HTTPServerConnection_State_Init:
        {
            _Connection->lineLength = 0;
            _Connection->state = HTTPServerConnection_State_ReadFirstLine;
        } break;

        case HTTPServerConnection_State_ReadFirstLine:
        {
            // Continuously read until we see \r\n
            for (;;) {
                uint8_t buffer[HTTPServer_BufferSize];
                int bytesRead = TCPClient_Read(&_Connection->tcpClient, buffer, sizeof(buffer));
                if (bytesRead <= 0)
                    break; // no more data this tick

                for (int i = 0; i < bytesRead; i++) {
                    uint8_t ch = buffer[i];
                    if (ch == '\n') {
                        if (_Connection->lineLength > 0 && _Connection->lineBuffer[_Connection->lineLength - 1] == '\r') {
                            _Connection->lineBuffer[_Connection->lineLength - 1] = 0;
                            printf("HTTPServerConnection(%d): First line: %s\n", _Connection->tcpClient.fd, _Connection->lineBuffer);

                            char method[16] = {0};
                            char url[1024] = {0};
                            char version[16] = {0};
                            if (sscanf((const char*)_Connection->lineBuffer, "%15s %1023s %15s", method, url, version) == 3) {
                                size_t mlen = strlen(method);
                                size_t ulen = strlen(url);
                                size_t vlen = strlen(version);
                                _Connection->method = (char*)malloc(mlen + 1);
                                _Connection->url = (char*)malloc(ulen + 1);
                                _Connection->version = (char*)malloc(vlen + 1);
                                if (_Connection->method && _Connection->url && _Connection->version) {
                                    memcpy(_Connection->method, method, mlen + 1);
                                    memcpy(_Connection->url, url, ulen + 1);
                                    memcpy(_Connection->version, version, vlen + 1);
                                    if (_Connection->onRequest) {
                                        _Connection->onRequest(_Connection->context);
                                    }
                                    _Connection->state = HTTPServerConnection_State_ReadHeaders;
                                } else {
                                    _Connection->state = HTTPServerConnection_State_InvalidRequest;
                                }
                            } else {
                                _Connection->state = HTTPServerConnection_State_InvalidRequest;
                            }
                        } else {
                            _Connection->state = HTTPServerConnection_State_InvalidRequest;
                        }
                        return; // done processing first line
                    } else {
                        if (_Connection->lineLength < HTTPServer_MaxLineLength - 1) {
                            _Connection->lineBuffer[_Connection->lineLength++] = ch;
                        } else {
                            _Connection->state = HTTPServerConnection_State_InvalidRequest;
                            return;
                        }
                    }
                }
            }
        } break;

        case HTTPServerConnection_State_ReadHeaders:
        {
            // For simplicity, we ignore headers and let the handler respond.
            // Close connection after handler writes response.
            // No-op here.

        } break;

        case HTTPServerConnection_State_InvalidRequest:
        {
            const char* resp = "HTTP/1.1 400 Bad Request\r\nConnection: close\r\nContent-Length: 0\r\n\r\n";
            TCPClient_Write(&_Connection->tcpClient, (const uint8_t*)resp, (int)strlen(resp));
            TCPClient_Dispose(&_Connection->tcpClient);

        } break;

    }

}

void HTTPServerConnection_Dispose(HTTPServerConnection* _Connection)
{
    TCPClient_Dispose(&_Connection->tcpClient);
    smw_destroyTask(_Connection->task);
}

void HTTPServerConnection_DisposePtr(HTTPServerConnection** _ConnectionPtr)
{
    if(_ConnectionPtr == NULL || *(_ConnectionPtr) == NULL)
        return;

    HTTPServerConnection_Dispose(*(_ConnectionPtr));
    free(*(_ConnectionPtr));
    *(_ConnectionPtr) = NULL;
}
