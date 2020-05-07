#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <prodcon.h>

int good_num;
int bad_num;

int is_bad() 
{
	// srand(time(0));
	int randbool = rand() & 1;
	if(bad_num > 0 && randbool) {
		bad_num--;
		return randbool;
	} else if(good_num > 0 && !randbool) {
		good_num--;
		return randbool;
	} else if(good_num == 0) {
		bad_num--;
		return 1;
	} else if(bad_num == 0) {
		good_num--;
		return 0;
	}
	return randbool;
}

void *consume( void *servicesock )
{
	int 	cc;
	int		fd;
	int 	dd;
	int 	csock;
    int 	item_size;
	char    *host = "localhost";	
	char	*service = (char *)servicesock;
	int 	 index = 0;


	if ( ( csock = connectsock( host, service, "tcp" )) == 0 )
		fprintf( stderr, "Cannot connect to server in consumer.\n" );
	else
	{
		if(is_bad()) usleep( SLOW_CLIENT * 1000000 );
		write(csock, CONSUME,9);
		fflush( stdout );

		int id = abs(pthread_self());
		char filename[45];
		sprintf( filename, "%d.txt", id );
		fd = open( filename, O_CREAT | O_WRONLY, 0666 );

		if ( (cc =  read( csock, &item_size, 4 )) <= 0 )
		{
			printf( "CONSUMER: The server has gone.\n" );
			char reject[15] = REJECT;
			write( fd, reject, 15 );
			close(csock);
		}
		else
		{
			item_size = htonl( item_size );

			// fprintf(stderr, "item_size: %d\n", item_size);

			int curr_item_size = (item_size - BUFSIZE > 0) ? BUFSIZE : item_size;	
			
			char *arrived = malloc( curr_item_size * sizeof(char*) );
			dd = open( "/dev/null", O_WRONLY );
			while (( cc = read(csock, arrived, curr_item_size )) > 0 )
			{
				write( dd, arrived, cc );
				// fprintf( stderr, "arrived size: %ld\n", cc );
				// write( fd, arrived, cc );
				// lseek( fd, 0, SEEK_END );
				index += cc;
				curr_item_size = (curr_item_size < item_size-index) ? curr_item_size : item_size-index;
				if( index >= item_size ) break;
			}
			close( dd );
			free( arrived );
			fprintf(stderr, "item_size_final: %d\n", index);

			int num_byte_size = (int)((ceil(log10(index+1)))*sizeof(char));
			char received_size[num_byte_size];
			sprintf(received_size, "%d", index);

			fprintf(stderr, "index_size: %s\n", received_size);

			if( index >= item_size ) 
			{
				char success[15] = SUCCESS;
				write( fd, success, 15 );
				lseek( fd, 0, SEEK_END );
				write( fd, received_size, num_byte_size );
				fprintf( stderr, "consumer client: item was consumed.\n" );
			} else {
				char byte_error[14] = BYTE_ERROR;
				write( fd, byte_error, 14 );
				lseek( fd, 0, SEEK_END );
				write( fd, received_size, num_byte_size );
				fprintf( stderr, "consumer client: server closed unexpectedly.\n" );
			}
			
			close( fd );
			close( csock );
		}
		pthread_exit( 0 );
	}
}

double poissonRandomInterarrivalDelay( double r )
{
    return (log((double) 1.0 - 
			((double) rand())/((double) RAND_MAX)))/-r;
}


int
main( int argc, char *argv[] )
{
	int			cc;
	int			csock;
	int         con_num;
	int			i, j;
	int 		bad;
	double 		rate;
	char		buf[BUFSIZE];
	char		*service;		
	char		*host = "localhost";
	
	switch( argc )
	{
		case    5:
			service = argv[1];
			con_num = atoi(argv[2]);
			rate = atof(argv[3]);
			bad = atoi(argv[4]);
			break;
		default:
			fprintf( stderr, "the wrong number of parameters\n" );
			exit( -1 ) ;
	}

	if( con_num > 2000 ) {
		printf( "Error: maximum 2000 clients allowed!\n" );
		exit( -1 );
	}


	pthread_t threads[con_num];
	bad_num = con_num * bad / 100;
	good_num = con_num - bad_num;

	double delay = poissonRandomInterarrivalDelay( rate ) * 1000000;

	for( i = 0; i < con_num; i++ )
	{
		usleep( delay );
		int status = pthread_create( &threads[i], NULL, consume, (void *) service );
		if( status != 0 )
		{
			printf( "Error\n" );
			exit( -1 );
		}
	}

	for ( int j = 0; j < con_num; j++ )
		pthread_join( threads[j], NULL );

	pthread_exit( 0 );
}