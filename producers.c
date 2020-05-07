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

int bad_num;
int good_num;

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

char *makeItem(int size) 
{
	int i = 0;
	char *item_letters = malloc( size+1 * sizeof(char*) );
	for ( i = 0; i < size; i++ )
		item_letters[i] = 'X';
	item_letters[i] = '\0';
	return item_letters;
}

void *produce( void *servicesock )
{
    char		buf[BUFSIZE];
	int		    cc, wc;
	int			i;
	int 		csock;
	char		msg[8];

	char     *host = "localhost";
	char	 *service = (char *)servicesock;

	if(is_bad()) {
		printf("Bad client-------------------------\n");
		usleep( SLOW_CLIENT * 1000000 );
	} else {
		// printf("Good client\n");
	}

	if ( (csock = connectsock( host, service, "tcp" )) == 0 )
	{
		fprintf( stderr, "Cannot connect to server.\n" );
		exit( -1 );
	}

	else 
	{
		if((cc = write(csock, PRODUCE, 9)) < 0) {
			printf("here\n");
			printf( "%d\n", cc);
			// printf( "%s\n", buf);
		}
		if ( (cc = read( csock, buf, BUFSIZE )) <= 0 )
		{
			printf( "The server has gone.\n" );
			close( csock );
		} 
		else 
		{
			buf[cc] = '\0';
			if( strcmp( buf, GO ) == 0 )
			{
				srand(time(0)); 
				int item_size = random()%MAX_LETTERS;
				int reversedSize = htonl( item_size );
				write( csock, &reversedSize, 4 );
				int item_idx = 0;
				fprintf( stderr, "item_size: %d\n", item_size );

				cc = read( csock, buf, BUFSIZE );
				buf[cc] = '\0';
				if( strcmp( buf, GO ) == 0 )
				{
					int curr_item_size = (item_size - BUFSIZE > 0) ? BUFSIZE : item_size;			
					char *item_letters = makeItem( curr_item_size );
					while(item_idx < item_size) 
					{						
						wc = write( csock, item_letters, curr_item_size );
						item_idx += wc;
						curr_item_size = (curr_item_size < item_size-item_idx) ? curr_item_size : item_size-item_idx;
					}

					free( item_letters );
					// printf("item_idx: %d\n", item_idx);
					
					while( !(cc = 6 && strcmp( msg, DONE ) == 0) ) {
						cc = read( csock, msg, 6 );
					}
					msg[ 6 ] = '\0';
					if ( cc = 6 && strcmp( msg, DONE ) == 0 ) {
						fprintf( stderr, "Sent the item and ended.\n" );
						close( csock );
					} else {
						fprintf( stderr, "Undetermined error\n" );
					}	
				}			
			} 
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
	int         num;
	int 		i, j;
	int 		csock;
	int 		bad;
	double 		rate;
	char		*service;
	char     	*host = "localhost";
	
	switch( argc )
	{
		case    5:
			service = argv[1];
			num = atoi(argv[2]);
			rate = atof(argv[3]);
			bad = atoi(argv[4]);
			break;
		default:
			fprintf( stderr, "the wrong number of parameters\n" );
			exit(-1);
	}

	if( num > 2000 ) {
		printf( "Error: maximum 2000 clients allowed!\n" );
		exit( -1 );
	}

	bad_num = num * bad / 100;
	good_num = num - bad_num;
	pthread_t threads[num];
	double delay = poissonRandomInterarrivalDelay( rate ) * 1000000;

	for( i = 0; i < num; i++ )
	{
		usleep( delay );	
		int status = pthread_create( &threads[i], NULL, produce, (void *) service );
		if( status != 0 )
		{
			printf( "Error\n" );
			exit( -1 );
		}
	}

	for ( j = 0; j < num; j++ )
		pthread_join( threads[j], NULL );

	pthread_exit( 0 );
}