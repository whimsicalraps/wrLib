#include "wrQStream.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "wrQueue.h"

typedef struct{
    wrStream_t* server;
    wrStream_t client;
    queue_t* q;
    wrStream_PACKET_t* ps;
} wrQStream_t;

wrQStream_t self;


//////////////////////////////////
// private declarations

static int client_busy( void );
static void server_error( int errorcode, char* msg );
static int client_request( wrStream_DIR_t direction
                         , int            location
                         , int            size_in_bytes
                         , uint8_t*       data );
static int server_response( wrStream_DIR_t direction
                          , int            location
                          , int            size_in_bytes
                          , uint8_t*       data );
static void server_request( void );


////////////////////////////////////
// constructor

wrStream_t* QStream_init( int max_length, wrStream_t* stream )
{
    self.q  = queue_init( max_length );
    self.ps = malloc( sizeof(wrStream_PACKET_t) * max_length );

    // capture & complete the server-side stream
    self.server = stream;

    // copy the server-stream into our new client stream
    memcpy( &self.client, self.server, sizeof( wrStream_t ) );

    // splice in the virtual-server qstream system

    // client busy & requests are sent to the queue system
    self.client.busy    = &client_busy;
    self.client.request = &client_request;

    // server response & error callbacks via the queue system (for removing requests)
    self.server->response = &server_response;
    self.server->error    = &server_error;

    return &self.client; // return the new stream to the client
}

void QStream_deinit( void )
{
    free( self.ps ); self.ps = NULL;
}

void QStream_try( void )
{
    server_request();
}

///////////////////////////////////////
// virtual stream interface
static int client_busy( void )
{
    return !queue_space( self.q );
}

static int client_request( wrStream_DIR_t direction
                         , int            location
                         , int            size_in_bytes
                         , uint8_t*       data
                         )
{
    int ix = queue_enqueue( self.q );
    if( ix == -1 ){ return 1; }

    // save the stream-request to the queue
    wrStream_PACKET_t* packet = &self.ps[ix];
    packet->direction     = direction;
    packet->location      = location;
    packet->size_in_bytes = size_in_bytes;
    packet->data          = data;

    server_request(); // in case the queue was empty (ie no ongoing request)

    return 0;
}

static int server_response( wrStream_DIR_t direction
                          , int            location
                          , int            size_in_bytes
                          , uint8_t*       data
                          )
{
    // the request succeeded, so remove it from the queue
    if( queue_dequeue( self.q ) == -1 ){
        printf("UH OH! shouldn't happen!\n");
        return 1;
    }

    // tell the client which request is complete
    (*self.client.response)( direction
                           , location
                           , size_in_bytes
                           , data
                           );

    server_request(); // initiate the next request in case the queue isn't empty

    return 0;
}

static void server_error( int errorcode, char* msg )
{
    printf("stream error %i: %s\n",errorcode,msg);
    printf("TODO qstream: _error. re-enqueue?\n");

    // FIXME if the server is no longer available need to call back to the client
        // *error function so it is aware the stream has dropped out

    server_request(); // initiate the next request in case the queue isn't empty
}


///////////////////////////////////
// helper functions

static void server_request( void )
{
    if( !(*self.server->busy)() ){

        // TODO prioritize reads over writes up to N writes
        // TODO attempt to combine read/write accesses
            // just look at proceeding page (no need for deep search)

        int ix = queue_front( self.q ); // nb: dequeue happens in response
        if( ix == -1 ){
            //printf("queue empty\n"); // TODO useful when testing with SD card!!
            return;
        }

        wrStream_PACKET_t* packet = &self.ps[ix];
        if( (*self.server->request)( packet->direction
                                   , packet->location
                                   , packet->size_in_bytes
                                   , packet->data
                                   ) ){
            printf("qstream request failed. TODO pop bad request?\n");
        }
    }
}
