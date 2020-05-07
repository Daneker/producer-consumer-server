#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <pthread.h>
#include <semaphore.h>
#include <prodcon.h>


ITEM 		**buffer;
int 		buf_idx 		= 0;
int 		thread_idx 		= 0;
int 		clients_idx 	= 0;
int 		consumer_idx 	= 0;
int 		producer_idx 	= 0;
int 		served_producer_idx = 0;
int 		served_consumer_idx = 0;
int 		time_rejected_clients_idx = 0;
int 		rejected_clients_idx = 0;
int 		rejected_producer_idx = 0;
int 		rejected_consumer_idx = 0;
int 		buf_size 		= BUFSIZE;

pthread_mutex_t mutex1, mutex2;
sem_t full, empty;

struct arg_struct {
    int  *arg1;
    char *arg2;
};

void close_sock( int ssock, int ctype ) 
{
	pthread_mutex_lock( &mutex1 );
	switch (ctype) 
	{
		case 0:
			served_consumer_idx++;
			consumer_idx--;
			clients_idx--;
			break;
		case 1:
			served_producer_idx++;
			producer_idx--;
			clients_idx--;
			break;
		case 2:
			rejected_clients_idx++;
			break;
		case 3:
			rejected_producer_idx++;
			break;
		case 4:
			rejected_consumer_idx++;
			break;
		case 5:
			producer_idx--;
			clients_idx--;
			break;		
	}
	pthread_mutex_unlock( &mutex1 );
	(void) close(ssock);
}


void* consume( void *fd ) 
{
	int cc;
	int item_size;
	int item_prod_sd;
	int letter_idx = 0;
	ITEM *p;
	int 	ssock = (int)( fd );
	
	sem_wait( &full );
	pthread_mutex_lock( &mutex2 );	
	p = buffer[buf_idx-1];
	buffer[buf_idx-1] = NULL;
	buf_idx--;

	printf("buffer index(c): %d\n", buf_idx);
	fflush(stdout);
	pthread_mutex_unlock( &mutex2 );
	sem_post( &empty );

	item_size = p->size;
	item_prod_sd = p->psd;

	fprintf( stderr, "item_size: %d\n", item_size );

	int reversed_size = htonl(p->size);
	write( ssock, &reversed_size, 4 );

	write( item_prod_sd, GO, 4 );

	int curr_item_size = (item_size - BUFSIZE > 0) ? BUFSIZE : item_size;
	char *item_letters = malloc( curr_item_size * sizeof(char*) );

	while( (cc = read( item_prod_sd, item_letters, curr_item_size )) > 0 ) 
    {
		write( ssock, item_letters, cc );
        letter_idx += cc;
		curr_item_size = (curr_item_size < item_size-letter_idx) ? curr_item_size : item_size-letter_idx;
		if( letter_idx >= p->size ) break;   
    }

	free( item_letters );
	// fprintf(stderr, "---------------------------------------------------\n");
    
    if( letter_idx < p->size )
    {
		fprintf(stderr, "ERROR: letter_idx: %d\n", letter_idx);
        close_sock( item_prod_sd, 5 );
    }
	else 
	{
		write( item_prod_sd, DONE, 6 );
		free( p );

		close_sock( item_prod_sd, PROD_TYPE );
		close_sock( ssock, CON_TYPE );
	}

	pthread_exit( 0 );
}

void* produce( void *fd ) 
{
	int cc = -1;
	int item_size;
	int 	ssock = (int)( fd );
	
	write(ssock, GO, 4);
    if( (cc = read( ssock, &item_size, 4 )) <= 0 ) 
        printf( "The server has gone.\n" );

    else item_size = htonl(item_size);	

    ITEM *p = malloc( sizeof(ITEM) );
    p->size = item_size;
    p->psd = ssock;	

	sem_wait(&empty);
	pthread_mutex_lock( &mutex2 );
	buffer[buf_idx] = p;
	
	printf("buffer index(p): %d\n", buf_idx);
	fflush(stdout);

	buf_idx++;
	pthread_mutex_unlock( &mutex2 );
	sem_post(&full);
        	
	pthread_exit(0);
}

void *get_status( void *arguments )
{
	struct arg_struct *args = (struct arg_struct *)arguments;

	printf("-------HERE------\n");
    int csock = args->arg1;
    char *status_buf = args->arg2;

	// printf("strcmp: %d\n", strcmp( status_buf, CURRCLI ));
	if( strcmp( status_buf, CURRCLI ) == 0 )
		write( csock, &clients_idx, 4 );
	if( strcmp( status_buf, CURRPROD ) == 0 )
		write( csock, &producer_idx, 4 );
	if( strcmp( status_buf, CURRCONS ) == 0 )
		write( csock, &consumer_idx, 4 );
	if( strcmp( status_buf, TOTPROD ) == 0 )
		write( csock, &served_producer_idx, 4 );
	if( strcmp( status_buf, TOTCONS ) == 0 )
		write( csock, &served_consumer_idx, 4 );
	if( strcmp( status_buf, REJMAX ) == 0 )
		write( csock, &rejected_clients_idx, 4 );
	if( strcmp( status_buf, REJSLOW ) == 0 )
		write( csock, &time_rejected_clients_idx, 4 );
	if( strcmp( status_buf, REJPROD ) == 0 )
		write( csock, &rejected_producer_idx, 4 );
	if( strcmp( status_buf, REJCONS ) == 0 )
		write( csock, &rejected_consumer_idx, 4 );

	pthread_mutex_lock( &mutex1 );
	clients_idx--;
	pthread_mutex_unlock( &mutex1 );
	(void) close(csock);
}

/*
**	The server ... uses multiplexng to switch between clients
**	Each client gets one echo per turn, 
**	but can have as many echoes as it wants until it disconnects
*/
int
main( int argc, char *argv[] )
{
	char			buf[BUFSIZE];
	char			*service;
	struct sockaddr_in	fsin;
	int			msock;
	int			ssock;
	fd_set			rfds;
	fd_set			afds;
	int			alen;
	int			fd;
	int			nfds;
	int			rport = 0;
	int			cc;
    pthread_t	thr;

	struct tm *ctime;
	time_t current;

    sem_init( &full, 0, 0 );
	sem_init( &empty, 0, buf_size );

    pthread_mutex_init( &mutex1, 0 );
	pthread_mutex_init( &mutex2, 0 );
	
	// Same arguments as usual
	switch (argc) 
	{
		case	2:
			// No args? let the OS choose a port and tell the user
			rport = 1;
			buf_size = atoi(argv[1]);
			break;
		case	3:
			// User provides a port? then use it
			service = argv[1];
			buf_size = atoi(argv[2]);
			break;
		default:
			fprintf( stderr, "usage: server [port]\n" );
			exit(-1);
	}

	int *stime = malloc( 516 * sizeof(int) );
	for( int i = 0; i < 516; i++ ) {
		stime[i] = -1;
	}

	// Create the main socket as usual
	// This is the socket for accepting new clients
	msock = passivesock( service, "tcp", QLEN, &rport );
	if (rport)
	{
		//	Tell the user the selected port
		printf( "server: port %d\n", rport );	
		fflush( stdout );
	}

	// Now we begin the set up for using select
	
	// nfds is the largest monitored socket + 1
	// there is only one socket, msock, so nfds is msock +1
	// Set the max file descriptor being monitored
	nfds = msock+1;
	buffer = malloc( buf_size * sizeof( ITEM* ) );

	// the variable afds is the fd_set of sockets that we want monitored for
	// a read activity
	// We initialize it to empty
	FD_ZERO(&afds);
	
	// Then we put msock in the set
	FD_SET( msock, &afds );

	// Now start the loop
	for (;;)
	{
		if( buf_size <= 0 ) break;
		// Since select overwrites the fd_set that we send it, 
		// we copy afds into another variable, rfds
		// Reset the file descriptors you are interested in
		memcpy((char *)&rfds, (char *)&afds, sizeof(rfds));

		// Only waiting for sockets who are ready to read
		//  - this includes new clients arriving
		//  - this also includes the client closed the socket event
		// We pass null for the write event and exception event fd_sets
		// we pass null for the timer, because we don't want to wake early
		if (select(nfds, &rfds, (fd_set *)0, (fd_set *)0,
				(struct timeval *)0) < 0)
		{
			fprintf( stderr, "server select: %s\n", strerror(errno) );
			exit(-1);
		}

		// Since we've reached here it means one or more of our sockets has something
		// that is ready to read
		// So now we have to check all the sockets in the rfds set which select uses
		// to return a;; the sockets that are ready

		// If the main socket is ready - it means a new client has arrived
		// It must be checked separately, because the action is different
		if (FD_ISSET( msock, &rfds)) 
		{
			int	ssock;

			// we can call accept with no fear of blocking
			alen = sizeof(fsin);
			ssock = accept( msock, (struct sockaddr *)&fsin, &alen );
			if (ssock < 0)
			{
				fprintf( stderr, "accept: %s\n", strerror(errno) );
				exit(-1);
			}

			// printf("ssock: %d\n", ssock);

			current = time(0);
			ctime = localtime(&current);
			stime[ssock] = ctime->tm_sec;
			

			// If a new client arrives, we must add it to our afds set
			FD_SET( ssock, &afds );
			// and increase the maximum, if necessary
			if ( ssock+1 > nfds )
				nfds = ssock+1;
		}

		// printf("nfds: %d\n", nfds);
		// Now check all the regular sockets
		for ( fd = 0; fd < nfds; fd++ )
		{
			current = time(0);
			ctime = localtime(&current);

			// check every socket to see if it's in the ready set
			// But don't recheck the main socket
			int wait_time = ctime->tm_sec - stime[fd];
			if (fd != msock )
			{
				if( wait_time > REJECT_TIME && stime[fd] >= 0 )
				{
					time_rejected_clients_idx++;

					printf( "The client is REJECTED, time: %d\n", wait_time );
					(void) close(fd);
					
					// If the client has closed the connection, we need to
					// stop monitoring the socket (remove from afds)
					stime[fd] = -1;
					// lower the max socket number if needed
					if ( nfds == fd+1 )
						nfds--;

					// printf("-----HERE_2------\n");

					FD_CLR( fd, &afds );
					continue;
				}
				// printf("-----HERE_1------\n");
				if ( FD_ISSET(fd, &rfds) )
				{
					// you can read without blocking because data is there
					// the OS has confirmed this
					if ( (cc = read( fd, buf, BUFSIZE )) <= 0 )
					{
						printf( "The client has gone.\n" );
						(void) close(fd);
						
						// If the client has closed the connection, we need to
						// stop monitoring the socket (remove from afds)
						// lower the max socket number if needed
						if ( nfds == fd+1 )
							nfds--;
					}
					else
					{
						// Otherwise send the echo to the client
						// pthread_create( &thr, NULL, echo, (void *) fd );
						buf[cc] = '\0';
						if( clients_idx < MAX_CLIENTS )	
						{
							if ( strcmp( buf, PRODUCE ) == 0 ) 
							{
								if( producer_idx < MAX_PROD ) {
									pthread_mutex_lock( &mutex1 );
									clients_idx++;
									producer_idx++;
									pthread_mutex_unlock( &mutex1 );
									pthread_create( &thr, NULL, produce, (void *) fd );
								}  else {
									close_sock(fd, 3);
								}
							}
							else if ( strcmp( buf, CONSUME ) == 0 )
							{
								if( consumer_idx < MAX_CON ) {
									pthread_mutex_lock( &mutex1 );
									clients_idx++;
									consumer_idx++;
									pthread_mutex_unlock( &mutex1 );
									pthread_create( &thr, NULL, consume, (void *) fd );
								}  else {
									close_sock(fd, 4);
								}
							}	
							else
							{
								pthread_mutex_lock( &mutex1 );
								clients_idx++;
								pthread_mutex_unlock( &mutex1 );
								struct arg_struct args;
								args.arg1 = fd;
    							args.arg2 = buf;
								pthread_create( &thr, NULL, get_status, (void *) &args );
							}	
						} 
						else 
						{
							printf( "The clients have reached maximum.\n" );
							close_sock(fd, 2);
						}
						stime[fd] = -1;
					}   
					FD_CLR( fd, &afds );
					if ( nfds == fd+1 )
						nfds--;
				}
			}
		}
	}
}