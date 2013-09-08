#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <getopt.h>

#define EXIT_SYNTAX 2
#define EXIT_ERROR  1
#define EXIT_NORMAL 0

#ifdef DEBUG
#define DEBUG_PRINTF printf
#else
#define DEBUG_PRINTF(...)
#endif

typedef uint8_t SETEntry[32];
typedef enum { SET_LITTLE_ENDIAN, SET_BIG_ENDIAN } endianness_t;

void swapBytes(uint8_t *byte1, uint8_t *byte2)
{
	uint8_t temp = *byte1;
	*byte1 = *byte2;
	*byte2 = temp;
}

uint32_t readSETHeader(FILE *fp, endianness_t endianness)
{
	// Returns number of objects.
	uint32_t objects;
	fread(&objects, 4, 1, fp);
	fseek(fp, 32, SEEK_SET);

	if (endianness == SET_BIG_ENDIAN) {
		// Convert to little endian
		uint8_t *ptr = (uint8_t*)&objects;
		swapBytes(ptr, ptr + 3);
		swapBytes(ptr + 1, ptr + 2);
	}
	
	// Return 0 if EOF already.
	return feof(fp) ? 0 : objects;
}

void writeSETHeader(FILE *fp, uint32_t num_objects, endianness_t endianness)
{
	if (endianness == SET_BIG_ENDIAN) {
		// Convert to big endian
		uint8_t *ptr = (uint8_t*)&num_objects;
		swapBytes(ptr, ptr + 3);
		swapBytes(ptr + 1, ptr + 2);
	}

	fwrite(&num_objects, 4, 1, fp);

	int i; for (i=0; i<28; i++) {
		uint8_t zero = 0;
		fwrite(&zero, 1, 1, fp);
	}
}

int doesFileExist(const char *filename)
{
	FILE *fp = fopen(filename, "r");
	
	if (fp) {
		fclose(fp);
		return 1;
	} else {
		if (errno == ENOENT)
			return 0;
		else
			return 1;
	}
}

int main(int argc, char *argv[])
{
	int opt;
	endianness_t endianness = SET_BIG_ENDIAN;
	int prompt_to_overwrite = 1;

	while ((opt = getopt(argc, argv, "ef")) != -1) {
		switch (opt) {
			case 'e': endianness = SET_LITTLE_ENDIAN; break;
			case 'f': prompt_to_overwrite = 0; break;
		}
	}

	DEBUG_PRINTF("argc=%d, optind=%d\n", argc, optind);

	if (argc - optind < 3) {
		printf("Usage: %s [-ef] set1 set2 ... output\n", argv[0]);
		printf(" -e\tTreat SET files as little-endian\n");
		printf(" -f\tOverwrite files without prompting\n");
		return EXIT_SYNTAX;
	}

	int num_files = argc - 1 - optind;
	uint32_t *object_counts = malloc(4 * num_files);
	uint32_t total_objects = 0;
	SETEntry **data = malloc(sizeof(SETEntry*) * num_files);

	int i; for (i=0; i<num_files; i++) {
		// loop through the input filenames
		const char *filename = argv[i+optind];
		DEBUG_PRINTF("Input file:  %s\n", filename);
		FILE *fp = fopen(filename, "rb");

		if (!fp) {
			printf("Error reading file %s\n", filename);
			perror(argv[0]);
			return EXIT_ERROR;
		}

		uint32_t num_objects = readSETHeader(fp, endianness);
		object_counts[i] = num_objects;
		total_objects += num_objects;
		data[i] = malloc(sizeof(SETEntry) * num_objects);

		if (data[i] == NULL) {
			printf("Not enough memory! Are you sure those are all SET files?\n");
			printf("(Make sure you have the endianness correct, and use -e if necessary.)\n");
			return EXIT_ERROR;
		}

		fread(data[i], sizeof(SETEntry), num_objects, fp);

		if (feof(fp)) {
			printf("File %s is too short.\n", filename);
			printf("Are you sure it's a SET file?\n");
			return EXIT_ERROR;
		}

		fclose(fp);
	}

	const char *out_filename = argv[argc-1];
	DEBUG_PRINTF("Output file: %s\n", out_filename);

	if (prompt_to_overwrite) // Don't check whether the file exists if not necessary
	if (doesFileExist(out_filename)) {
		printf("Output file %s already exists!\n", out_filename);
		printf("Do you want to overwrite it? [no]: ");
		
		char ch = getchar();
		if (ch != 'y' && ch != 'Y') {
			printf("Aborting. No changes have been made.\n");
			return EXIT_ERROR;
		}
	}

	FILE *fp = fopen(out_filename, "wb");
	
	if (!fp) {
		printf("Error creating file %s\n", out_filename);
		perror(argv[0]);
		return EXIT_ERROR;
	}

	writeSETHeader(fp, total_objects, endianness);

	for (i=0; i<num_files; i++) {
		uint32_t num_objects = object_counts[i];
		fwrite(data[i], sizeof(SETEntry), num_objects, fp);
	}

	fclose(fp);
	printf("%d files have been successfully merged into %s.\n", num_files, out_filename);

	// Let the OS take care of freeing the memory.
	return EXIT_SUCCESS;
}
