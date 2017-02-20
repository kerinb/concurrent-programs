#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/stat.h>

#include <jpeglib.h>


int main (int argc, char *argv[]) {
	int rc, i, j;

	char *syslog_prefix = (char*) malloc(1024);
	sprintf(syslog_prefix, "%s", argv[0]);
	openlog(syslog_prefix, LOG_PERROR | LOG_PID, LOG_USER);

	if (argc != 2) {
		fprintf(stderr, "USAGE: %s filename.jpg\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	// Variables for the source jpg
	struct stat file_info;
	unsigned long jpg_size;
	unsigned char *jpg_buffer;

	// Variables for the decompressor itself
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

	// Variables for the output buffer, and how long each row is
	unsigned long bmp_size;
	unsigned char *bmp_buffer;
	int row_stride, width, height, pixel_size;

    // load data into program and store in buffer 
	rc = stat(argv[1], &file_info); // jpeg_stdio_src
	if (rc) {
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
	
	width = cinfo.output_width;
	height = cinfo.output_height;
	pixel_size = cinfo.output_components;

	syslog(LOG_INFO, "Proc: Image is %d by %d with %d components", 
			width, height, pixel_size);

	bmp_size = width * height * pixel_size;
	bmp_buffer = (unsigned char*) malloc(bmp_size);

	row_stride = width * pixel_size;
    FILE* fa = fopen("file.txt", "w");
    

	syslog(LOG_INFO, "Proc: Start reading scanlines\n");
	unsigned char *buffer[cinfo.output_height][cinfo.output_height];
	
	int matrix[cinfo.output_height][cinfo.output_width];
	
	for (int x = 0; cinfo.output_scanline < cinfo.output_height; x++) 
	{
		unsigned char *buffer_array[1];
		buffer_array[0] = bmp_buffer + (cinfo.output_scanline) * row_stride;				           
	    //printf("buffer array: %i\n", buffer_array[0]);
		jpeg_read_scanlines(&cinfo, buffer_array, 1);
		/*for(int y = 0; cinfo.output_scanline < cinfo.output_height; y++)
		{
		    matrix[x][y] = (buffer_array[y*3] + buffer_array[y*3+1] + buffer_array[y*3 +2])/3;
		    fprintf(fa, "%i ", matrix[x][y]);
		}*/
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

	rc = sprintf(buf, "P6 %d %d 255\n", width, height);
	write(fd, buf, rc); // Write the PPM image header before data
	write(fd, bmp_buffer, bmp_size); // Write out all RGB pixel data

	close(fd);
	free(bmp_buffer);

	syslog(LOG_INFO, "End of decompression");
	return EXIT_SUCCESS;
}
