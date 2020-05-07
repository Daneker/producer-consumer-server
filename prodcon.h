#ifndef PRODCON

#define PRODCON

#define QLEN            5
#define BUFSIZE         8192
#define MAX_CLIENTS     512
#define MAX_PROD    480
#define MAX_CON     480

// Some clients may wait (seconds) before introducing themseleves
#define SLOW_CLIENT     3

// Slow clients may be rejected if they do not respond in (seconds)
#define REJECT_TIME     2

// Results to be recorded in the txt files by consumers
#define BYTE_ERROR	"ERROR: bytes "
#define SUCCESS		"SUCCESS: bytes "
#define REJECT		"ERROR: REJECTED"

// Each item consists of random number of bytes between 1 and MAX_LETTERS.
#define MAX_LETTERS     1000000000


// This item struct will work for all versions
typedef struct item_t
{
	uint32_t	size;
	int		psd;
        char		*letters;
} ITEM;

#define PROD_TYPE 		1
#define CON_TYPE		0

#define GO	        "GO\r\n"
#define CONSUME		"CONSUME\r\n"
#define PRODUCE		"PRODUCE\r\n"
#define DONE		"DONE\r\n"


#define CURRCLI		"CURRCLI\r\n"
#define CURRPROD	"CURRPROD\r\n"
#define CURRCONS	"CURRCONS\r\n"
#define TOTPROD		"TOTPROD\r\n"
#define TOTCONS		"TOTCONS\r\n"
#define REJMAX		"REJMAX\r\n"
#define REJSLOW		"REJSLOW\r\n"
#define REJPROD		"REJPROD\r\n"
#define REJCONS		"REJCONS\r\n"


#endif



