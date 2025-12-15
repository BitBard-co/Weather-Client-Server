#include "HTTPClient.h"

void HTTPClient_Work(void* _Context, uint64_t _MonTime)
{
    HTTPClient* client = (HTTPClient*)_Context;
    if (client == NULL)
        return; // Safety check

    switch (client->state)
    {
        case HTTPClient_State_Init:
            // Initialize resources or connection parameters
            // Example: open socket, resolve hostname, etc.y
            client->state = HTTPClient_State_Connect;
            break;

        case HTTPClient_State_Connect:
            // Attempt to connect to the HTTP server
            // Example: if connection succeeds, move to transmit
            if (HTTPClient_TryConnect(client))
            {
                client->state = HTTPClient_State_Transmit;
            }
            else if (HTTPClient_HasTimedOut(client, _MonTime))
            {
                client->state = HTTPClient_State_Close;
            }
            break;

        case HTTPClient_State_Transmit:
            // Send the HTTP request
            if (HTTPClient_SendRequest(client))
            {
                client->state = HTTPClient_State_Receive;
            }
            else if (HTTPClient_HasError(client))
            {
                client->state = HTTPClient_State_Close;
            }
            break;

        case HTTPClient_State_Receive:
            // Wait for and process the response
            if (HTTPClient_ReceiveResponse(client))
            {
                client->state = HTTPClient_State_Close;
            }
            else if (HTTPClient_HasTimedOut(client, _MonTime))
            {
                client->state = HTTPClient_State_Close;
            }
            break;

        case HTTPClient_State_Close:
            // Clean up connection and resources
            HTTPClient_Close(client);
            client->state = HTTPClient_State_Init; // Ready for next use
            break;

        default:
            // Unknown state — reset or log error
            client->state = HTTPClient_State_Close;
            break;
    }
}


int HTTPClient_Initiate(HTTPClient* _Client)
{
	memset(_Client, 0, sizeof(HTTPClient));
	
	_Client->buffer = NULL;
	_Client->task = NULL;

	return 0;
}

int HTTPClient_GET(HTTPClient* _Client, const char* _URL, void (*callback)(HTTPClient* _CLient, const char* _Event))
{
	_Client->buffer = malloc(4096);
	if(_Client->buffer == NULL)
		return -1;


	snprintf(_Client->buffer, 4096, "GET %s HTTP/1.1\r\nHost: chas.se\r\nConnection: close\r\n\r\n", _URL);

	_Client->bufferPtr = _Client->buffer;

	_Client->task = smw_createTask(_Client, HTTPClient_Work);

}

void HTTPClient_Dispose(HTTPClient* _Client)
{
	if(_Client->task != NULL)
		smw_destroyTask(_Client->task);

	if(_Client->buffer != NULL)
		free(_Client->buffer);

}

