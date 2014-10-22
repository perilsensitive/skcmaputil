/*
  skcmaputil - Shovel Knight controller map utility
  Copyright (C) 2014 Ryan Jackson
  
  This program is free software: you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <error.h>
#include <string.h>
#include <ctype.h>

enum {
	ACTION_CLEAR,
	ACTION_APPEND,
	ACTION_REPLACE,
	ACTION_DUMP
};

struct file_info {
	uint64_t orig_data_offset;
	uint64_t new_data_offset;
	uint64_t orig_name_offset;
	uint64_t new_name_offset;
	uint64_t orig_size;
	uint64_t new_size;
	uint64_t metadata[3];
	char *name;
	uint8_t *new_data;
};

static struct file_info *files;
static uint64_t orig_data_offset_table_offset;
static uint64_t orig_name_offset_table_offset;
static uint64_t new_data_offset_table_offset;
static uint64_t new_name_offset_table_offset;
static uint32_t file_count;
uint64_t *data_offset_table;
uint64_t *name_offset_table;

static char **windows_mapping_list;
static char **linux_mapping_list;
static char **mac_mapping_list;
static int windows_mapping_count;
static int linux_mapping_count;
static int mac_mapping_count;

/*
  Read a string from a file into memory, stopping after '\0' or another chosen
  character is read, or EOF.
  
  fh:         FILE * to read from. Must be seekable (cannot be a FIFO).
  bufferptr:  address of the pointer that should store the string's address
  lengthptr:  address of the integer that should store the number of bytes
              read
  ch:         character on which to stop reading
  max_length: maximum number of characters to read from the file

  The last character read (including '\0' or ch) is included in the string
  returned.

  Returns 0 on success, -1 on error.
 */
static int read_string(FILE *fh, char **bufferptr, int *lengthptr, char ch,
		       int max_length)
{
	char *ptr;
	int length;
	int c;

	length = 0;
	while (1) {
		c = fgetc(fh);

		length++;

		if (!c || (c == EOF) || (c == ch))
			break;

		if (length == max_length)
			break;
	}

	if ((c == EOF) && (!feof(fh)))
		return -1;

	if (c == EOF)
		return 0;

	ptr = malloc(length + 1);
	if (!ptr)
		return -1;

	if (fseek(fh, -length, SEEK_CUR) < 0) {
		free(ptr);
		return -1;
	}

	if (fread(ptr, length, 1, fh) < 1) {
		free(ptr);
		return -1;
	}
	ptr[length] = '\0';
	    
	if (!bufferptr) {
		free(ptr);
	} else {
		*bufferptr = ptr;
	}

	if (lengthptr)
		*lengthptr = length;

	return 0;
}

/* Read a little-endian 64-bit word from a file */
static int read_64bit_word(FILE *fh, uint64_t *ptr)
{
	uint8_t buffer[8];
	uint64_t value;
	int rc;

	rc = fread(buffer, 8, 1, fh);

	if (rc != 1)
		return -1;

	value =	(uint64_t)buffer[0] |
		((uint64_t)buffer[1] << 8) |
		((uint64_t)buffer[2] << 16) |
		((uint64_t)buffer[3] << 24) |
		((uint64_t)buffer[4] << 32) |
		((uint64_t)buffer[5] << 40) |
		((uint64_t)buffer[6] << 48) |
		((uint64_t)buffer[7] << 56);

	if (ptr)
		*ptr = value;

	return 0;
}

/* Write a 64-bit word to a file in little-endian format */
static int write_64bit_word(FILE *dest, uint64_t value)
{
	uint8_t buffer[8];

	buffer[0] = value & 0xff;
	buffer[1] = (value >> 8) & 0xff;
	buffer[2] = (value >> 16) & 0xff;
	buffer[3] = (value >> 24) & 0xff;
	buffer[4] = (value >> 32) & 0xff;
	buffer[5] = (value >> 40) & 0xff;
	buffer[6] = (value >> 48) & 0xff;
	buffer[7] = (value >> 56) & 0xff;

	if (fwrite(buffer, 8, 1, dest) != 1)
		return -1;

	return 0;
}

static int copy_data(FILE *dest, FILE *src, uint64_t size)
{
	uint8_t *buffer;
	size_t chunk;
	int rc;

	buffer = malloc(4096);
	if (!buffer)
		return -1;

	rc = 0;
	while (size) {
		chunk = 4096 < size ? 4096 : size;
		if (fread(buffer, chunk, 1, src) != 1) {
			rc = -1;
			break;
		}

		if (fwrite(buffer, chunk, 1, dest) != 1) {
			rc = -1;
			break;
		}

		size -= chunk;
	}

	free(buffer);

	return rc;
}

static int save_pak(FILE *dest, FILE *src)
{
	uint64_t value;
	int i;

	value = (uint64_t)file_count;
	value <<= 32;

	if (write_64bit_word(dest, value) < 0)
		return -1;

	if (write_64bit_word(dest, new_data_offset_table_offset) < 0)
		return -1;

	if (write_64bit_word(dest, new_name_offset_table_offset) < 0)
		return -1;

	if (fseek(dest, new_data_offset_table_offset, SEEK_SET) < 0) {
		return -1;
	}

	for (i = 0; i < file_count; i++) {
		if (write_64bit_word(dest, files[i].new_data_offset) < 0)
			return -1;
	}

	for (i = 0; i < file_count; i++) {
		if (fseek(dest, files[i].new_data_offset, SEEK_SET) < 0)
			return -1;

		if (write_64bit_word(dest, files[i].new_size) < 0)
			return -1;

		if (write_64bit_word(dest, files[i].metadata[0]) < 0)
			return -1;

		if (write_64bit_word(dest, files[i].metadata[1]) < 0)
			return -1;

		if (write_64bit_word(dest, files[i].metadata[2]) < 0)
			return -1;

		/* Write out file data */
		if (files[i].new_data) {
			if (fwrite(files[i].new_data, files[i].new_size,
				   1, dest) != 1) {
				return -1;
			}
		} else {
			if (fseek(src, files[i].orig_data_offset + 32,
				  SEEK_SET) < 0) {
				return -1;
			}

			if (copy_data(dest, src, files[i].orig_size) < 0)
				return -1;
		}
	}

	if (fseek(dest, new_name_offset_table_offset, SEEK_SET) < 0)
		return -1;

	for (i = 0; i < file_count; i++) {
		if (write_64bit_word(dest, files[i].new_name_offset) < 0)
			return -1;
	}

	for (i = 0; i < file_count; i++) {
		int length, padding;

		length = strlen(files[i].name) + 1;
		if (fseek(dest, files[i].new_name_offset, SEEK_SET) < 0)
			return -1;

		if (fwrite(files[i].name, length, 1, dest) != 1)
			return -1;

		/* Add padding so that the next name is aligned on a
		   64-bit boundary.
		*/
		padding = length % 8;
		if (padding)
			padding = 8 - padding;

		if (fseek(dest, padding, SEEK_CUR) < 0)
			return -1;
	}

	return 0;
}


static int load_pak_metadata(FILE *src)
{
	uint64_t value;
	int i;

	files = NULL;

	if (read_64bit_word(src, &value) < 0)
		return -1;

	file_count = value >> 32;
	if (read_64bit_word(src, &orig_data_offset_table_offset) < 0)
		return -1;

	new_data_offset_table_offset = orig_data_offset_table_offset;

	if (read_64bit_word(src, &orig_name_offset_table_offset) < 0)
		return -1;

	new_name_offset_table_offset = orig_name_offset_table_offset;

	files = malloc(file_count * sizeof(*files));
	if (!files)
		return -1;

	memset(files, 0, file_count * sizeof(*files));

	if (fseek(src, orig_data_offset_table_offset, SEEK_SET) < 0)
		return -1;

	for (i = 0; i < file_count; i++) {
		if (read_64bit_word(src, &value) < 0)
			return -1;

		files[i].orig_data_offset = value;
		files[i].new_data_offset = value;
	}

	if (fseek(src, orig_name_offset_table_offset, SEEK_SET) < 0)
		return -1;

	for (i = 0; i < file_count; i++) {
		if (read_64bit_word(src, &value) < 0)
			return -1;

		files[i].orig_name_offset = value;
		files[i].new_name_offset = value;
	}

	for (i = 0; i < file_count; i++) {
		uint64_t offset = files[i].orig_data_offset;
		if (fseek(src, offset, SEEK_SET) < 0)
			return -1;

		if (read_64bit_word(src, &files[i].orig_size) < 0)
			return -1;

		files[i].new_size = files[i].orig_size;

		if (read_64bit_word(src, &files[i].metadata[0]) < 0)
			return -1;

		if (read_64bit_word(src, &files[i].metadata[1]) < 0)
			return -1;

		if (read_64bit_word(src, &files[i].metadata[2]) < 0)
			return -1;
	}

	for (i = 0; i < file_count; i++) {
		uint64_t offset = files[i].orig_name_offset;
		if (fseek(src, offset, SEEK_SET) < 0)
			return -1;

		if (read_string(src, &files[i].name, NULL, 0, 0) < 0)
			return -1;
	}
 
	return 0;
}

static void cleanup(void)
{
	int i;

	for (i = 0; i < file_count; i++) {
		if (files[i].name)
			free(files[i].name);

		if (files[i].new_data)
			free(files[i].new_data);
	}

	for (i = 0; i < linux_mapping_count; i++) {
		if (linux_mapping_list[i])
			free(linux_mapping_list[i]);
	}

	for (i = 0; i < windows_mapping_count; i++) {
		if (windows_mapping_list[i])
			free(windows_mapping_list[i]);
	}

	for (i = 0; i < mac_mapping_count; i++) {
		if (mac_mapping_list[i])
			free(mac_mapping_list[i]);
	}

	if (windows_mapping_list)
		free(windows_mapping_list);

	if (linux_mapping_list)
		free(linux_mapping_list);

	if (mac_mapping_list)
		free(mac_mapping_list);

	free(files);
}

static int add_list_entry(char ***listptr, int *countptr, char *buffer)
{
	int new_count;
	char **tmp;
	int i;

	for (i = 0; i < *countptr; i++) {
		int guidlen_a, guidlen_b;
		char *tmp;

		tmp = strchr(buffer, ',');
		if (!tmp)
			continue;

		guidlen_a = tmp - buffer;

		tmp = strchr((*listptr)[i], ',');
		if (!tmp)
			continue;

		guidlen_b = tmp - (*listptr)[i];

		if (guidlen_a != guidlen_b)
			continue;

		if (strncasecmp(buffer, (*listptr)[i], guidlen_a) == 0)
			break;
	}

	/* Entry already present in list. Replace it with the new binding */
	if (i < *countptr) {
		free((*listptr)[i]);
		(*listptr)[i] = buffer;
		return 0;
	}

	new_count = *countptr + 1;
	tmp = *listptr;
	tmp = realloc(tmp, new_count * sizeof(*listptr));
	if (!tmp)
		return -1;

	tmp[new_count - 1] = buffer;
	*listptr = tmp;
	*countptr = new_count;

	return 0;
}

/* Load a file of SDL GameController mappings.  The format of this
   file is the same as what SDL_GameControllerAddMappingsFromFile()
   will accept.
*/
static int load_mapping_database(const char *filename)
{
	FILE *db_fh;
	char *buffer;
	char ***listptr;
	int *countptr;
	int length;

	db_fh = fopen(filename, "rb");
	if (!db_fh)
		return -1;

	while (!feof(db_fh) && !ferror(db_fh)) {
		char *platform;
		char *ptr;

		buffer = NULL;
		if (read_string(db_fh, &buffer, &length, '\n', 0) < 0) {
			if (buffer)
				free(buffer);
			break;
		} else if (!buffer) {
			break;
		}

		/* Skip comments */
		if (buffer[0] == '#') {
			free(buffer);
			continue;
		}

		/* Skip blank lines */
		ptr = buffer;
		while (*ptr && isspace(*ptr))
			ptr++;

		if (!*ptr) {
			free(buffer);
			continue;
		}

		if (length && (buffer[length - 1] == '\n'))
			buffer[length - 1] = '\0';
		if ((length >= 2) && (buffer[length - 2] == '\r'))
			buffer[length - 2] = '\0';

		/* Skip records with no platform field */
		platform = strstr(buffer, "platform:");
		if (!platform) {
			free(buffer);
			continue;
		}

		platform += strlen("platform:");
		while (*platform && isspace(*platform))
			platform++;

		if (strncasecmp(platform, "Mac OS X", 8) == 0) {
			listptr = &mac_mapping_list;
			countptr = &mac_mapping_count;
		} else if (strncasecmp(platform, "Windows", 7) == 0) {
			listptr = &windows_mapping_list;
			countptr = &windows_mapping_count;
		} else if (strncasecmp(platform, "Linux", 5) == 0) {
			listptr = &linux_mapping_list;
			countptr = &linux_mapping_count;
		} else {
			continue;
		}

		if (add_list_entry(listptr, countptr, buffer) < 0) {
			if (buffer)
				free(buffer);
			break;
		}
	}

	fclose(db_fh);

	return 0;
}

static int prepare_new_data(const char *name, char **list, int count)
{
	uint8_t *ptr;
	int length;
	int i, j;
	int64_t diff;
	int padding, orig_padding;

	for (i = 0; i < file_count; i++) {
		if (strcmp(files[i].name, name) == 0)
			break;
	}

	if (i == file_count)
		return 0;

	length = 0;
	for (j = 0; j < count; j++) {
		length += strlen(list[j]) + 1;
	}

	if (length)
		files[i].new_data = malloc(length);
	else
		files[i].new_data = NULL;

	files[i].new_size = length;

	ptr = files[i].new_data;
	for (j = 0; j < count; j++) {
		int len = strlen(list[j]);
		memcpy(ptr, list[j], len);
		ptr[len] = '\n';
		ptr += len + 1;
	}

	/* Add padding so that the next file is aligned on a 64-bit
	   boundary.
	*/
	padding = files[i].new_size % 8;
	if (padding)
		padding = 8 - padding;

	/*  Figure out how much padding the old version of the file
	    needed.
	*/
	orig_padding = files[i].orig_size % 8;
	if (orig_padding)
		orig_padding = 8 - orig_padding;

	/* Figure out the difference we need to apply to the offsets
	   of everything stored after this file to account for the
	   change in size.
	*/
	diff = files[i].new_size + padding;
	diff -= files[i].orig_size + orig_padding;

	if (orig_data_offset_table_offset > files[i].orig_data_offset)
		new_data_offset_table_offset += diff;

	if (orig_name_offset_table_offset > files[i].orig_data_offset)
		new_name_offset_table_offset += diff;

	for (j = 0; j < file_count; j++) {
		if (files[j].orig_data_offset > files[i].orig_data_offset)
			files[j].new_data_offset += diff;

		if (files[j].orig_name_offset > files[i].orig_data_offset)
			files[j].new_name_offset += diff;
	}

	return 0;
}

static int load_mappings(FILE *src, const char *name, const char *platform,
			 char ***list_ptr, int *count_ptr)
{
	uint64_t offset;
	uint64_t size;
	int i;

	for (i = 0; i < file_count; i++) {
		if (strcmp(files[i].name, name) == 0)
			break;
	}

	if (i == file_count)
		return 0;

	/* add 32 bytes to the offset to skip metadata */
	offset = files[i].orig_data_offset + 32;
	size = files[i].orig_size;

	*count_ptr = 0;

	if (fseek(src, offset, SEEK_SET) < 0)
		return -1;

	while (size > 0) {
		int length;
		char *buffer;
		char **tmp;
		int new_count;

		if (read_string(src, &buffer, &length, '\n', size) < 0)
			return -1;

		/* Strip line endings */
		if (buffer[length - 1] == '\n')
			buffer[length - 1] = '\0';
		if (buffer[length - 2] == '\r')
			buffer[length - 2] = '\0';

		/* Add platform field if it's missing.  This way the
		   mapping conforms to the standard format.
		*/
		if (!strstr(buffer, "platform:")) {
			int guid_length;
			int new_buffer_length;
			char *tmp_buf;

			new_buffer_length  = strlen(buffer);
			new_buffer_length += strlen(",platform:");
			new_buffer_length += strlen(platform);
			new_buffer_length++;

			tmp_buf = strchr(buffer, ',');
			if (!tmp_buf) {
				free(buffer);
				size -= length;
				continue;
			}

			guid_length = tmp_buf - buffer;

			tmp_buf = malloc(new_buffer_length);
			if (!tmp_buf) {
				free(buffer);
				size -= length;
				continue;
			}

			memcpy(tmp_buf, buffer, guid_length);
			snprintf(tmp_buf + guid_length,
				 new_buffer_length - guid_length,
				 ",platform:%s%s", platform,
				 buffer + guid_length);

			free(buffer);
			buffer = tmp_buf;
		}
		
		new_count = *count_ptr + 1;
		tmp = *list_ptr;
		tmp = realloc(tmp, new_count * sizeof(*tmp));
		if (!tmp) {
			free(buffer);
			return -1;
		}

		tmp[new_count - 1] = buffer;
		*list_ptr = tmp;
		*count_ptr = new_count;

		size -= length;
	}


	return 0;
}

void usage(void)
{
	fprintf(stderr, "skcmaputil - Shovel Knight controller map utility\n\n"
		"Usage: skcmaputil COMMAND [ARGS]\n\n"
		"COMMAND [ARGS] is one of the following:\n\n"
		"dump <pakfile>:                      display mapping lists in pakfile\n"
		"\n"
		"clear <src> <dest>:                  remove mapping lists from src and store\n"
		"                                     the modified PAK file in dest\n"
		"\n"
		"replace <src> <dest> <mapping_file>: replace mapping lists from src and\n"
		"                                     replace them with those in mapping_file,\n"
		"                                     storing the modified PAK file in dest\n"
		"\n"
		"append <src> <dest> <mapping_file>:  append mapping_file entries to the lists\n"
		"                                     in source_pakfile, storing the modified\n"
		"                                     PAK file in dest\n"
		"\n");
}

int main(int argc, char **argv)
{
	const char *src_pak_filename;
	const char *dest_pak_filename;
	const char *mapping_database_filename;
	const char *command;
	FILE *src_pak_fh, *dest_pak_fh;
	int status;
	int action;
	int i;

	if (argc < 3) {
		usage();
		return -1;
	}

	status = -1;
	command = argv[1];
	src_pak_filename = argv[2];
	dest_pak_filename = NULL;
	mapping_database_filename = NULL;
	src_pak_fh = NULL;
	dest_pak_fh = NULL;

	if (strcasecmp(command, "clear") == 0) {
		action = ACTION_CLEAR;
		if (argc < 4) {
			usage();
			return -1;
		}

		dest_pak_filename = argv[3];
	} else if (strcasecmp(command, "append") == 0) {
		action = ACTION_APPEND;
		if (argc < 5) {
			usage();
			return -1;
		}

		dest_pak_filename = argv[3];
		mapping_database_filename = argv[4];
	} else if (strcasecmp(command, "replace") == 0) {
		action = ACTION_REPLACE;
		if (argc < 5) {
			usage();
			return -1;
		}

		dest_pak_filename = argv[3];
		mapping_database_filename = argv[4];
	} else if (strcasecmp(command, "dump") == 0) {
		action = ACTION_DUMP;
	} else {
		usage();
		return -1;
	}

	src_pak_fh = fopen(src_pak_filename, "rb");
	if (!src_pak_fh) {
		perror("Failed to open source PAK file");
		goto done;
	}

	if (load_pak_metadata(src_pak_fh))
		goto done;


	if ((action == ACTION_DUMP) || (action == ACTION_APPEND)) {
		if (load_mappings(src_pak_fh, "control_bindings_lnx.cmap",
				  "Linux",
				  &linux_mapping_list,
				  &linux_mapping_count) < 0) {
			goto done;
		}

		if (load_mappings(src_pak_fh, "control_bindings_mac.cmap",
				  "Mac OS X",
				  &mac_mapping_list,
				  &mac_mapping_count) < 0) {
			goto done;
		}

		if (load_mappings(src_pak_fh, "control_bindings.cmap",
				  "Windows",
				  &windows_mapping_list,
				  &windows_mapping_count) < 0) {
			goto done;
		}
	} else {
		/* Shovel Knight will crash if the mapping file has a size
		   of 0, so make sure there's always at least one blank
		   line.
		*/
		linux_mapping_count = 0;
		windows_mapping_count = 0;
		mac_mapping_count = 0;

		linux_mapping_list = malloc(sizeof (*linux_mapping_list));
		windows_mapping_list = malloc(sizeof (*windows_mapping_list));
		mac_mapping_list = malloc(sizeof (*mac_mapping_list));

		if (!linux_mapping_list || !windows_mapping_list ||
		    !mac_mapping_list) {
			goto done;
		}

		linux_mapping_list[0] = strdup("\n");
		windows_mapping_list[0] = strdup("\n");
		mac_mapping_list[0] = strdup("\n");

		if (!linux_mapping_list[0] || !windows_mapping_list[0] ||
		    !mac_mapping_list[0]) {
			goto done;
		}

		linux_mapping_count = 1;
		windows_mapping_count = 1;
		mac_mapping_count = 1;
	}

	if ((action == ACTION_APPEND) || (action == ACTION_REPLACE)) {
		if (load_mapping_database(mapping_database_filename) < 0)
			goto done;
	}

	if (action != ACTION_DUMP) {
		if (prepare_new_data("control_bindings_lnx.cmap",
				     linux_mapping_list,
				     linux_mapping_count) < 0) {
			goto done;
		}

		if (prepare_new_data("control_bindings_mac.cmap",
				     mac_mapping_list,
				     mac_mapping_count) < 0) {
			goto done;
		}

		if (prepare_new_data("control_bindings.cmap",
				     windows_mapping_list,
				     windows_mapping_count) < 0) {
			goto done;
		}

		dest_pak_fh = fopen(dest_pak_filename, "wb");
		if (!dest_pak_fh) {
			perror("Failed to open dest PAK file");
			goto done;
		}

		if (save_pak(dest_pak_fh, src_pak_fh))
			goto done;
	} else {

		printf("# Windows\n");
		for (i = 0; i < windows_mapping_count; i++) {
			printf("%s\n", windows_mapping_list[i]);
		}
		printf("\n");

		printf("# Linux\n");
		for (i = 0; i < linux_mapping_count; i++) {
			printf("%s\n", linux_mapping_list[i]);
		}
		printf("\n");

		printf("# Mac OS X\n");
		for (i = 0; i < mac_mapping_count; i++) {
			printf("%s\n", mac_mapping_list[i]);
		}
	}

	status = 0;

done:
	if (src_pak_fh)
		fclose(src_pak_fh);

	if (dest_pak_fh)
		fclose(dest_pak_fh);

	cleanup();

	return status;
}
