#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <pthread.h>
#include <netinet/in.h>
#include <prodcon.h>
#include <time.h>
#include <string.h>

pthread_mutex_t mutex;

struct arg_struct {
    char *arg1;
    char *arg2;
};

void *get_status( void *arguments )
{
	int		    cc;
	int			i;
	int 		csock;
	char     *host = "localhost";

    struct arg_struct *args = (struct arg_struct *)arguments;

    char *service = args->arg1;
    char *status = args->arg2;

	if ( (csock = connectsock( host, service, "tcp" )) == 0 )
	{
		fprintf( stderr, "Cannot connect to server.\n" );
		exit( -1 );
	}
	else
	{
		write( csock, status, 10 );	
		int status_num;
		pthread_mutex_lock( &mutex );
		if ( (cc = read( csock, &status_num, BUFSIZE )) <= 0 )
		{
			printf( "The server has gone.\n" );
			close( csock );
		} 
		else 
		{
			fprintf(stderr, "server answer: %d\n", status_num);
			close( csock );
		}
		pthread_mutex_unlock( &mutex );
	}
	
	close( csock );
	pthread_exit( 0 );
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
    char        *cstatus;
	char     	*host = "localhost";
	pthread_mutex_init( &mutex, 0 );
	
	switch( argc )
	{
        case    2:
			service = argv[1];
			cstatus = "";
			break;
		case    3:
			service = argv[1];
			cstatus = argv[2];
			break;
		default:
			fprintf( stderr, "the wrong number of parameters\n" );
			exit(-1);
	}

	pthread_t thread;
    struct arg_struct args;
    args.arg1 = service;

	if( strlen(cstatus) > 0 ) strcat(cstatus, "\r\n");
    args.arg2 = cstatus;

	char *lstatus;
	char command[3];
	int status_code;

	char *menu = " 1.CURRCLI\n 2.CURRPROD\n 3.CURRCONS\n 4.TOTPROD\n 5.TOTCONS\n 6.REJMAX\n 7.REJSLOW\n 8.REJPROD\n 9.REJCONS\n 10.quit\n";
	if( strlen(cstatus) <= 0 )
	{
		while( status_code != 10 ) {
			usleep( 200000 );
			printf( "\n%s\n", menu );
			printf( "enter status number: " );
			fgets( command, 3, stdin );
			int status_code = atoi(command);
			// printf("status _code: %d\n", status_code);
			switch (status_code)
			{
				case 1:
					lstatus = CURRCLI;
					break;
				case 2:
					lstatus = CURRPROD;
					break;
				case 3:
					lstatus = CURRCONS;
					break;
				case 4:
					lstatus = TOTPROD;
					break;
				case 5:
					lstatus = TOTCONS;
					break;
				case 6:
					lstatus = REJMAX;
					break;
				case 7:
					lstatus = REJSLOW;
					break;
				case 8:
					lstatus = REJPROD;
					break;
				case 9:
					lstatus = REJCONS;
					break;
			}

			if( status_code == 10 ) break;

			args.arg2 = lstatus;

			int status = pthread_create( &thread, NULL, get_status, (void *) &args );
			if( status != 0 )
			{
				printf( "Error\n" );
				exit( -1 );
			}
		}
	} else {
		int status = pthread_create( &thread, NULL, get_status, (void *) &args );
		if( status != 0 )
		{
			printf( "Error\n" );
			exit( -1 );
		}
	}    

    (void) pthread_join(thread, NULL);

	pthread_exit( 0 );
	exit(-1);
}