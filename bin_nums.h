#ifndef __BIN_NUMS__
#define __BIN_NUMS__

/*Binary numbers structure of the Media Cloud Project*/
typedef struct {
	int mag_num, // magic number
	    next_id; // next identifier
} bin_nums;

const int magic_number = 666;

int counter=0;
int number_threads=0;
char * file_b;

typedef struct Needle{

	int	Id,
		Status,
		Size;

	char 	Data[1];	/* a notorious technique that does work */
				/* see memory allocation trick below */

}Needle;

typedef struct NeedleIndex{
	int		Id,		/* Needle unique ID */
			status;

	long int	start_offset,	/* starts at in HayStack file */
			end_offset;	/* ends at in HayStack file */
}NeedleIndex;


#endif
