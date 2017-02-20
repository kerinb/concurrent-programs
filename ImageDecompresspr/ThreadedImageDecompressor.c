#include <pthread.h> 
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/stat.h>
#include <jpeglib.h>
#include <stdlib.h>
#include <math.h>

#define NUM_THREADS 4
#define BILLION 1E9

// Variables for the source jpg
struct stat file_info;
unsigned long jpg_size;
unsigned char *jpg_buffer;

// Variables for the decompressor itself
struct jpeg_decompress_struct cinfo;
struct jpeg_error_mgr jerr;

// Variables for the output buffer, and how long each row is
unsigned long bmp_size[NUM_THREADS];
unsigned char *bmp_buffer;
int row_stride;

typedef struct pixelData // iformation requried to integrate
{
        int threadNumber, width, height, pixelSize;
} pixelData;

void *imageManip(void *args) 
{
	for (int x = 0; cinfo.output_scanline < cinfo.output_height; x++) 
	{
	        
		unsigned char *buffer_array[1];
		buffer_array[0] = bmp_buffer + (cinfo.output_scanline) * row_stride;				           
		jpeg_read_scanlines(&cinfo, buffer_array, 1);
	
	}
}

int main (int argc, const char *argv[]) 
{
        //---------Start Defining relevant Variables --------------//
        struct timespec requestStart, requestEnd;
        int rc, i, j, t; 
	char *syslog_prefix = (char*) malloc(1024);
	sprintf(syslog_prefix, "%s", argv[0]);
	openlog(syslog_prefix, LOG_PERROR | LOG_PID, LOG_USER);

	pixelData pixelThread[NUM_THREADS]; // array of type Integral( data required to integrate a function)
	pthread_t threads[NUM_THREADS]; //array of threads
        //---------End Defining relevant Variables --------------//

        clock_gettime(CLOCK_REALTIME, &requestStart); // time 
		
	// load data into program and store in buffer 
	rc = stat(argv[1], &file_info); // jpeg_stdio_src
	if (rc) 
	{
		syslog(LOG_ERR, "FAILED to stat source jpg");
		exit(EXIT_FAILURE);
	}
	jpg_size = file_info.st_size;
	jpg_buffer = (unsigned char*) malloc(jpg_size + 100);

	int fd = open(argv[1], O_RDONLY);
	i = 0;
	while (i < jpg_size) {
		rc = read(fd, jpg_buffer + i, jpg_size - i);
		syslog(LOG_INFO, "Input: Read %d/%lu bytes", rc, jpg_size-i);
		i += rc;
	}
	close(fd);
 
        syslog(LOG_INFO, "Proc: Create Decompress struct");
	cinfo.err = jpeg_std_error(&jerr);	
	jpeg_create_decompress(&cinfo);


	syslog(LOG_INFO, "Proc: Set memory buffer as source");
	jpeg_mem_src(&cinfo, jpg_buffer, jpg_size);


	syslog(LOG_INFO, "Proc: Read the JPEG header");
	rc = jpeg_read_header(&cinfo, TRUE);

	if (rc != 1) {
		syslog(LOG_ERR, "File does not seem to be a normal JPEG");
		exit(EXIT_FAILURE);
	}
	
	syslog(LOG_INFO, "Proc: Initiate JPEG decompression");
	jpeg_start_decompress(&cinfo);
	
	for (t=0;t<NUM_THREADS;t++) // for each thread 
	{ 
		pixelThread[t].threadNumber=t; // set the thread ID 
		pixelThread[t].width = cinfo.output_width/NUM_THREADS; //set the width ( should be constant)
		pixelThread[t].height =  cinfo.output_height/NUM_THREADS; //set height 
		pixelThread[t].pixelSize = cinfo.output_components;
		
		syslog(LOG_INFO, "Proc: Image is %d by %d with %d components for thread %d", 
                pixelThread[t].width, pixelThread[t].height, pixelThread[t].pixelSize, t);
                
                bmp_size[t] = pixelThread[t].width * pixelThread[t].height * pixelThread[t].pixelSize;
        }
        
        bmp_buffer = (unsigned char*) malloc(bmp_size[0]*NUM_THREADS); 
	row_stride = pixelThread[0].width * pixelThread[0].pixelSize;
        FILE* fa = fopen("file.txt", "w");
	
	syslog(LOG_INFO, "Proc: Start reading scanlines\n");
	unsigned char *buffer[cinfo.output_height/NUM_THREADS][cinfo.output_height/NUM_THREADS];	
	
	// wait for threads to exit 
	for(t=0;t<NUM_THREADS;t++)  
	{
	        rc = pthread_create(&threads[t],NULL, imageManip,(void *)&pixelThread[t]); // 
		if (rc) 
		{ 
			printf("ERROR return code from pthread_create(): %d\n", rc); 
			return(1); 
		} 
		
		pthread_join( threads[t], NULL);
	}
	
	syslog(LOG_INFO, "\nProc: Done reading scanlines");

        fclose(fa);
	jpeg_finish_decompress(&cinfo);
	// Once you're really really done, destroy the object to free everything
	jpeg_destroy_decompress(&cinfo);
	// And free the input buffer
	free(jpg_buffer);
	
	// Write the decompressed bitmap out to a ppm file, just to make sure 
	// it worked. 
	fd = open("output.ppm", O_CREAT | O_WRONLY, 0666);
	char buf[1024];

	rc = sprintf(buf, "P6 %d %d 255\n", pixelThread[0].width*NUM_THREADS, pixelThread[0].height*NUM_THREADS);
	write(fd, buf, rc); // Write the PPM image header before data
	write(fd, bmp_buffer, *bmp_size); // Write out all RGB pixel data

	close(fd);
	free(bmp_buffer);

	syslog(LOG_INFO, "End of decompression");

        clock_gettime(CLOCK_REALTIME, &requestEnd);
		
	// Calculate time it took
        double accum = (requestEnd.tv_sec-requestStart.tv_sec)+(requestEnd.tv_nsec-requestStart.tv_nsec)/BILLION;
        printf( "%lf ns\n", accum );
		
	return EXIT_SUCCESS;
}



