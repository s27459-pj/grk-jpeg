#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <jpeglib.h>
#include <argtable2.h>
#include <string.h>

const char * input_file;
const char * output_file;
const char * filter;
const char * axis;
const char * direction;
double percent;
double times;


/* we will be using this uninitialized pointer later to store raw, uncompressd image */
JSAMPARRAY row_pointers = NULL;


/* dimensions of the image we want to write */
JDIMENSION width;
JDIMENSION height;
int num_components;
int quality = 75;
J_COLOR_SPACE color_space;

void negate(){
    for (int y = 0; y < height; y++) {
        JSAMPROW row = row_pointers[y];
        for (int x = 0; x < width; x++) {
            JSAMPROW pixel = &(row[x * 3]);
            for (int k = 0; k <= 2; k++) {
                pixel[k] = 255 - pixel[k];
            }
        }
    }
}

int clamp(int min, int max, int value) {
    if (value < min) {
        return min;
    }
    if (value > max) {
        return max;
    }
    return value;
}

void brightness() {
    for (int y = 0; y < height; y++) {
        JSAMPROW row = row_pointers[y];
        for (int x = 0; x < width; x++) {
            JSAMPROW pixel = &(row[x * 3]);
            for (int k = 0; k <= 2; k++) {
                int new_value = pixel[k] + (percent / 100) * pixel[k];
                pixel[k] = clamp(0, 255, new_value);
            }
        }
    }
}

void contrast() {
    for (int y = 0; y < height; y++) {
        JSAMPROW row = row_pointers[y];
        for (int x = 0; x < width; x++) {
            JSAMPROW pixel = &(row[x*3]);
            for (int k = 0; k <= 2; k++) {
                int new_value = times * (pixel[k] - 127) + 127;
                pixel[k] = clamp(0, 255, new_value);
            }
        }
    }
}

void swap(JSAMPROW a, JSAMPROW b) {
    JSAMPLE temp;
    for (int k = 0; k <= 2; k++) {
        temp = a[k];
        a[k] = b[k];
        b[k] = temp;
    }
}

void flip() {
    if(strcmp(axis, "y") == 0) {
        for (int y = 0; y < height; y++) {
            JSAMPROW row = row_pointers[y];
            for (int x = 0; x < width / 2; x++) {
                JSAMPROW pixel_start = &(row[x * 3]);
                int pixel_end_index = width - 1 - x ;
                JSAMPROW pixel_end = &(row[pixel_end_index * 3]);
                swap(pixel_start, pixel_end);
            }
        }
    }
    else if(strcmp(axis, "x") == 0) {
        for (int y = 0; y < height / 2; y++) {
            JSAMPROW row_start = row_pointers[y];
            JSAMPROW row_end = row_pointers[height - 1 - y];
            for (int x = 0; x < width; x++) {
                JSAMPROW pixel_start = &(row_start[x * 3]);
                JSAMPROW pixel_end = &(row_end[x * 3]);
                swap(pixel_start, pixel_end);
            }
        }
    }
}

void rotate() {
    if(strcmp(direction, "right") == 0) {
        for (int y = 0; y < height; y++) {
            JSAMPROW row = row_pointers[y];
            for (int x = 0; x < width / 2; x++) {
                JSAMPROW pixel_start = &(row[x*3]);
                JSAMPROW pixel_end = &(row[(width - 1 - x) * 3]);

                // TODO)) Rotate right

                // Swap all channels
                JSAMPLE temp;
                for (int k = 0; k <= 2; k++) {
                    temp = pixel_start[k];
                    pixel_start[k] = pixel_end[k];
                    pixel_end[k] = temp;
                }
            }
        }
    }
    // TODO)) Rotate left
}

void process_file() {
    if (color_space != JCS_RGB) {
        printf("Unsupported color space! Please use RGB.");
        return;
    };

    if (strcmp(filter, "negate") == 0) {
        return negate();
    }
    if (strcmp(filter, "brightness") == 0) {
        return brightness();
    }
    if (strcmp(filter, "contrast") == 0) {
        return contrast();
    }
    if (strcmp(filter, "flip") == 0) {
        return flip();
    }
    if (strcmp(filter, "rotate") == 0) {
        return rotate();
    }

    printf("Unknown filter: %s\n", filter);
}




void abort_(const char * s, ...)
{
	va_list args;
	va_start(args, s);
	vfprintf(stderr, s, args);
	fprintf(stderr, "\n");
	va_end(args);
	abort();
}


/**
 * read_jpeg_file Reads from a jpeg file on disk specified by filename and saves into the
 * raw_image buffer in an uncompressed format.
 *
 * \returns positive integer if successful, -1 otherwise
 * \param *filename char string specifying the file name to read from
 *
 */

void read_jpeg_file( const char *filename )
{
	/* these are standard libjpeg structures for reading(decompression) */
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	/* libjpeg data structure for storing one row, that is, scanline of an image */
	int y;

	FILE *infile = fopen( filename, "rb" );

	if ( !infile )
	{
		abort_("Error opening input jpeg file %s!\n", filename);
	}
	/* here we set up the standard libjpeg error handler */
	cinfo.err = jpeg_std_error( &jerr );
	jpeg_create_decompress( &cinfo );
	/* this makes the library read from infile */
	jpeg_stdio_src( &cinfo, infile );
	/* reading the image header which contains image information */
	jpeg_read_header( &cinfo, TRUE );


	/* Start decompression jpeg here */
	jpeg_start_decompress( &cinfo );
	width = cinfo.output_width;
	height = cinfo.output_height;
	num_components = cinfo.out_color_components;
	color_space = cinfo.out_color_space;


	/* allocate memory to hold the uncompressed image */
	size_t rowbytes = width * num_components;
	row_pointers = (JSAMPARRAY) malloc(sizeof(j_common_ptr) * height);
	for (y=0; y<height; y++){
		row_pointers[y] = (JSAMPROW) malloc(rowbytes);
	}


	/* read one scan line at a time */
	y=0;
	JSAMPARRAY tmp = row_pointers;
	while( cinfo.output_scanline < cinfo.image_height )
	{
		y = jpeg_read_scanlines( &cinfo, tmp, 1 );
		tmp +=y;
	}
	/* wrap up decompression, destroy objects, free pointers and close open files */
	jpeg_finish_decompress( &cinfo );
	jpeg_destroy_decompress( &cinfo );
// 	free( row_pointer[0] );
	fclose( infile );
	/* yup, we succeeded! */
}




/**
 * write_jpeg_file Writes the raw image data stored in the raw_image buffer
 * to a jpeg image with default compression and smoothing options in the file
 * specified by *filename.
 *
 * \returns positive integer if successful, -1 otherwise
 * \param *filename char string specifying the file name to save to
 *
 */
void write_jpeg_file( const char *filename )
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	int y;
	JSAMPARRAY tmp;


	/* this is a pointer to one row of image data */
	FILE *outfile = fopen( filename, "wb" );

	if ( !outfile )	{
		abort_("Error opening output jpeg file %s!\n", filename );
	}
	cinfo.err = jpeg_std_error( &jerr );
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, outfile);

	/* Setting the parameters of the output file here */
	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = num_components;
	cinfo.in_color_space = color_space;
    /* default compression parameters, we shouldn't be worried about these */
	jpeg_set_defaults( &cinfo );
	jpeg_set_quality (&cinfo, quality, TRUE);
	/* Now do the compression .. */
	jpeg_start_compress( &cinfo, TRUE );
	/* like reading a file, this time write one row at a time */
	tmp = row_pointers;
	while( cinfo.next_scanline < cinfo.image_height )
	{
		y = jpeg_write_scanlines( &cinfo, tmp, 1 );
		tmp +=y;
	}
	/* similar to read file, clean up after we're done compressing */
	jpeg_finish_compress( &cinfo );
	jpeg_destroy_compress( &cinfo );
	fclose( outfile );

        /* cleanup heap allocation */
	for (y=0; y<height; y++){
		free(row_pointers[y]);
	}
	free(row_pointers);
}




int main(int argc, char **argv){
  // Options
  struct arg_file *input_file_arg = arg_file1("i", "input-file", "<input>", "Input JPEG File");
  struct arg_file *output_file_arg = arg_file1("o", "out-file" , "<output>", "Output JPEG File");
  struct arg_str *filter_arg = arg_str1("f", "filter" , "<filter>", "Filter (negate, brightness, contrast, flip, rotate)");
  struct arg_str *axis_arg = arg_str0("a", "axis", "<axis>", "Axis (x, y)");
  struct arg_str *direction_arg = arg_str0("d", "direction", "<direction>", "Direction (left, right)");
  struct arg_dbl *percent_arg = arg_dbl0("p", "percent" , "<percent>", "Percent (for brightness");
  struct arg_dbl *times_arg = arg_dbl0("t", "times" , "<times>", "Multiplier (for contrast)");
  struct arg_lit *help = arg_lit0("h","help", "print this help and exit");
  struct arg_end *end = arg_end(10); // maksymalna liczba błędów 10

  int nerrors;

  void *argtable[] = {input_file_arg, output_file_arg, filter_arg, axis_arg, direction_arg, percent_arg, times_arg, help, end};

  if (arg_nullcheck(argtable) != 0) printf("error: insufficient memory\n");

  axis_arg->sval[0] = "y";
  direction_arg->sval[0] = "right";
  percent_arg->dval[0] = 0;
  times_arg->dval[0] = 1;

  nerrors = arg_parse(argc, argv, argtable);

  if (help->count > 0){
     printf("Usage: point");
     arg_print_syntax(stdout, argtable,"\n");
     arg_print_glossary(stdout, argtable,"  %-30s %s\n");
     arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
     return 0;
  }


  if (nerrors==0){
     input_file = input_file_arg->filename[0];
     output_file = output_file_arg->filename[0];
     filter = filter_arg->sval[0];
     axis = axis_arg->sval[0];
     direction = direction_arg->sval[0];
     percent = percent_arg->dval[0];
     times = times_arg->dval[0];
  }
  else{
     arg_print_errors(stderr, end, "point");
     arg_print_glossary(stderr, argtable, " %-25s %s\n");
     arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
     return 1;
  }


   read_jpeg_file(input_file);
   process_file();
   write_jpeg_file(output_file);
   arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
   return 0;
}
