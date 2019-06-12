#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <string.h>
#include <regex.h>

#define text_segment_char '\x00'
#define num_segment_char '\x01'
#define byte_segment_char '\x02'
#define unknown_segment_char '\x03'
#define metadata_size 8
#define max_text_length 16
#define segments_size 64
#define max_segments_count 64

////Struct used to store starting information

struct textInfo { 
	char* name;
	int segmentOfSameType;
	int position;
	char* regex;
};

#define textSegPartsSize 9
const struct textInfo textSegParts[textSegPartsSize] = 
	{{"device_name", 0, 0, "^[a-zA-Z0-9_-]*$"},
	{"rom_revision", 0, 1, "^[a-zA-Z0-9_-]*$"},
	{"serial_number", 0, 2, "^[A-Z0-9]*$"},

	{"bd_addr_part0", 1, 0, "^[A-Z0-9:]*$"},
	{"bd_addr_part1", 1, 1, "^[A-Z0-9:]*$"},
	{"bd_pass_part0", 1, 2, "^[a-z0-9]*$"},

	{"bd_pass_part1", 2, 0, "^[a-z0-9]*$"},
	{"rom_checksum_part0", 2, 1, "^[a-z0-9]*$"},
	{"rom_checksum_part1", 2, 2, "^[a-z0-9]*$"}};

#define allowedNumbersCapacity 8
struct numInfo {
	char* name;
	int segmentOfSameType;
	int position;
	int allowedNums[allowedNumbersCapacity];
	int allowedNumsCount;
};

#define numSegPartsSize 3
const struct numInfo numSegParts[numSegPartsSize] = 
	{{"serial_baudrate", 0, 0, {1200, 2400, 4800, 9600, 19200, 115200}, 6},
	{"audio_bitrate", 0, 1, {32, 96, 128, 160, 192, 256, 320}, 7},
	{"sleep_period", 0, 2, {100, 500, 1000, 5000, 10000}, 5}};

#define allowedCharsCapacity 4
struct byteInfo {
	char* name;
	int segmentOfSameType;
	int position;
	char allowedBytes[allowedCharsCapacity];
	int allowedBytesCount;
};

#define byteSegPartsSize 3
const struct byteInfo byteSegParts[byteSegPartsSize] = 
	{{"serial_parity", 0, 0, {'N', 'E', 'O'}, 3},
	{"serial_data_bit", 0, 1, {'5', '6', '7', '8'}, 4},
	{"serial_stop_bit", 0, 2, {'0', '1'}, 2}};

////Functions closely connected to the structures

const struct textInfo* findTextPart(const char* parameter) {
	for (int i = 0; i < textSegPartsSize; ++i) {
		if (!strcmp(textSegParts[i].name, parameter)) {
			return (const struct textInfo*) &textSegParts[i];
		}
	}
	return (struct textInfo*) NULL;
}

const struct numInfo* findNumPart(const char* parameter) {
    for (int i = 0; i < numSegPartsSize; ++i) {
        if (!strcmp(numSegParts[i].name, parameter)) {
            return &numSegParts[i];
        }   
    }   
    return (struct numInfo*) NULL;
}

const struct byteInfo* findBytePart(const char* parameter) {
    for (int i = 0; i < byteSegPartsSize; ++i) {
        if (!strcmp(byteSegParts[i].name, parameter)) {
            return &byteSegParts[i];
        }   
    }   
    return (struct byteInfo*) NULL;
}

char findType(const char* parameter) {
	for (int i = 0; i < textSegPartsSize; ++i) {
		if (!strcmp(textSegParts[i].name, parameter)) {
			return text_segment_char;
		}
	}
	for (int i = 0; i < numSegPartsSize; ++i) {
		if (!strcmp(numSegParts[i].name, parameter)) {
			return num_segment_char;
		}
	}
	for (int i = 0; i < byteSegPartsSize; ++i) {
		if (!strcmp(byteSegParts[i].name, parameter)) {
			return byte_segment_char;
		}
	}
	
	return unknown_segment_char; //unknown_segment_char is used as non-recognized type
}

////Other functions

int match(const char* string, const char* pattern) { //https://www.quora.com/How-do-I-use-regular-expressions-in-the-C-programming-language
    regex_t regex; 
    if (regcomp(&regex, pattern, REG_NOSUB) != 0) {
		return 0;
	}

    int status = regexec(&regex, string, 0, NULL, 0); 
    regfree(&regex);
    if (status != 0) return 0;
    return 1;
}

int isInAllowedNumbers(const char* parameter, const int number) { //Is number in allowed values of parameter;
	int allowedNumCount = findNumPart(parameter)->allowedNumsCount;
	const int* allowedNum = findNumPart(parameter)->allowedNums;
	
	for (int i = 0; i < allowedNumCount; ++i) {
		if (number == allowedNum[i]) {
			return 1;
		}
	}
	return 0;
}

int isInAllowedBytes(const char* parameter, const char byte) { //Is byte in allowed values of parameter;
	int allowedBytesCount = findBytePart(parameter)->allowedBytesCount;
	const char* allowedBytes = findBytePart(parameter)->allowedBytes;
	
	for (int i = 0; i < allowedBytesCount; ++i) {
		if (byte == allowedBytes[i]) {
			return 1;
		}
	}
	return 0;
}

void findTypeSegmentAndPosition(const char* parameter, char* type, int* segment, int* position) {
	*type = findType(parameter);
	if (*type == unknown_segment_char) {
		errx(1, "Unknown parameter '%s'", parameter);
	}
		
	switch (*type) {
		case text_segment_char:
			*segment = findTextPart(parameter)->segmentOfSameType;
			*position = findTextPart(parameter)->position;
			break;
		case num_segment_char:
			*segment = findNumPart(parameter)->segmentOfSameType;
			*position = findNumPart(parameter)->position;
			break;
		case byte_segment_char:
			*segment = findBytePart(parameter)->segmentOfSameType;
       	    *position = findBytePart(parameter)->position;
			break;
	}	
}

int getElementsSize(char type) {
	switch(type) {
		case text_segment_char:
			return max_text_length;
		case num_segment_char:
			return sizeof(int);
		case byte_segment_char:
			return sizeof(char);
	}
	return -1;
}

int findRightSegment(char* fileName, char* parameter, int file, char type, int segment) { //Returns the number of segment where 'paremeter' is located in the file
	int segmentsCount = 0;
	int sameType = 0;
	char typeCheck;

	int fileSize = lseek(file, 0, SEEK_END);
	lseek(file, 0, SEEK_SET);

	if (read(file, &typeCheck, sizeof(typeCheck)) == -1) {
		close(file);
		errx(1, "Error reading from file '%s'", fileName);
	}

	while (type != typeCheck || segment != sameType) { //Finding the right segment to edit
			
		if (type == typeCheck) {
			++sameType;
		}
		++segmentsCount;

		if (lseek(file, segments_size - 1, SEEK_CUR) > fileSize) {
			close(file);
			errx(1, "Cannot find parameter '%s' in the file '%s'", parameter, fileName);
		}

		if (read(file, &typeCheck, sizeof(typeCheck)) == -1) {
			close(file);
           	errx(1, "Error reading from file '%s'", fileName);
		}
	}

	return segmentsCount;
}

void changeValue(char* argv[], int changeBits) { 
	int file = open(argv[1], O_RDWR);

	if (file == -1) {
		errx(1, "File '%s' couldn't open", argv[1]);
	}
 	
	char type;
	int segment, position;

	findTypeSegmentAndPosition(argv[3], &type, &segment, &position); 

	int segmentsCount = findRightSegment(argv[1], argv[3], file, type, segment);
			
	int size = getElementsSize(type);
	
	char emptyArray[size];
	memset(emptyArray, 0, size);		//Clearing position before saving new one;

	lseek(file, (segmentsCount * segments_size) + (metadata_size + position * size), SEEK_SET);	
	if (write(file, emptyArray, size) == -1) {
		close(file);
		errx(1, "Error writing to file '%s'", argv[1]);
	}
		
	if (type == text_segment_char) { //Editing text segment, if it is the right one
		if (strlen(argv[4]) > (max_text_length - 1)) {
			close(file);
			errx(1, "Word '%s' is longer than %i symbols", argv[4], (max_text_length - 1));
		}
	
		if (!match(argv[4], (findTextPart(argv[3]))->regex)) {
			close(file);
			errx(1, "Unknown symbols in '%s'", argv[4]);
		}	

		lseek(file, (segmentsCount * segments_size) + (metadata_size + position * size), SEEK_SET);
	    write(file, argv[4], strlen(argv[4]) + 1); //+ 1 is for '\0' 
	} 
	else if (type == num_segment_char) { //Editing num segment, if it is the right one
		int number = atoi(argv[4]);
		if (number == 0 && argv[4][0] != '0') {
			close(file);
			errx(1, "Argument '%s' must be an integer", argv[4]);
		}

		if (!isInAllowedNumbers(argv[3], number)) {
			close(file);
			errx(1, "Argument '%s' is not in allowed values", argv[4]);
		}

		lseek(file, (segmentsCount * segments_size) + (metadata_size + position * size), SEEK_SET);
	    write(file, &number, sizeof(number));
	}
	 else if (type == byte_segment_char) { //Editing byte segment, if it is the right one
		if (strlen(argv[4]) > 1) {
			close(file);
			errx(1, "Last argument must be one character");
		}

		char byte = argv[4][0];
		if (!isInAllowedBytes(argv[3], byte)) {
			close(file);
			errx(1, "Value '%s' is not in allowed values", argv[4]);
		}
	
		if (argv[4][0] >= '0' && argv[4][0] <= '9') {
			byte -= '0';
		} 
		
		lseek(file, (segmentsCount * segments_size) + (metadata_size + position * size), SEEK_SET);
	    write(file, &byte, sizeof(byte));
	}

	if (changeBits) {
		lseek(file, (segmentsCount * segments_size) + 1, SEEK_SET);

		char byteToChange;
		if (read(file, &byteToChange, sizeof(byteToChange)) == -1) {
			close(file);
			errx(1, "Error reading from file '%s'", argv[1]);
		}
 
		char bitToChange = (1 << (7 - position));
		byteToChange |= bitToChange;

		lseek(file, (segmentsCount * segments_size) + 1, SEEK_SET);
		write(file, &byteToChange, sizeof(byteToChange));
	}
	
	close(file);
}

void createFile(int argc, char* argv[]) {
	int segArgs = (argc - 3);
	if (segArgs % 2 != 0) {
		errx(1, "Arguments after '-c' should be even number");
	}

	int segmentsCount = segArgs/2;

	if (segmentsCount > max_segments_count) {
		errx(1, "Max amount of segments is %i", max_segments_count);
	}

	char segments[segmentsCount];
	int pos;

	for (int i = 3; i < argc; ++i) {
		if (i % 2 == 1) {
			pos = atoi(argv[i]);
			if (pos <= 0 && argv[i][0] != '0') {
				errx(1, "Argument '%s' must be a positive integer", argv[i]);
			}

			if (pos >= segmentsCount) {
				errx(1, "Segment index is bigger than the number of segments");
			}
		} else {
			if (strlen(argv[i]) > 1 || (strcmp("t", argv[i]) && strcmp("n", argv[i]) && strcmp("b", argv[i]))) {
				errx(1, "Segment must be one of these: t, n, b");
			}
			segments[pos] = argv[i][0];
		} 
	}

	int file = open(argv[1], O_CREAT | O_TRUNC | O_WRONLY, 0666);
	if (file == -1) {
		errx(1, "Couldn't create file '%s'", argv[1]);
	}

	for (int i = 0; i < segmentsCount; ++i) {
		char emptyArray[segments_size];
		memset(emptyArray, 0, segments_size);
	
		if (segments[i] == 't') {
			emptyArray[0] = text_segment_char;
		} else if (segments[i] == 'n') {
			emptyArray[0] = num_segment_char;
		} else {
			emptyArray[0] = byte_segment_char;
		} 
		if (write(file, &emptyArray, sizeof(emptyArray)) == -1) {
			close(file);
			errx(1, "Couldn't create file '%s'", argv[1]);
		}
	}
	
	close(file);
}

void printValueInRightFormat(const char* string, int file, char type, int segmentsCount, int position) {
	int size = getElementsSize(type);
	
	lseek(file, (segmentsCount * segments_size) + (metadata_size + position * size), SEEK_SET);
			
	if (type == text_segment_char) {
		char text[size];
		read(file, text, size);
		printf("%s: %s\n", string, text);
	}
	else if (type == num_segment_char) {
		int number;
		read(file, &number, size);
		printf("%s: %i\n", string, number); 
	}
	else if (type == byte_segment_char) {
		char byte;
		read(file, &byte, size);
				
		printf("%s: ", string);
		if (byte >= 0 && byte <= 9) { //Comparing bytes
			printf("%c\n", byte + '0');
		} else {
			printf("%c\n", byte);
		}
	}
}

void printValue(char* fileName, char* parameter, int shouldCheckBit) {
	int file = open(fileName, O_RDONLY);

	if (file == -1) {
		errx(1, "File '%s' does not exitst!", fileName);
	}
 	
	char type;
	int segment, position;

	findTypeSegmentAndPosition(parameter, &type, &segment, &position); 

	int segmentsCount = findRightSegment(fileName, parameter, file, type, segment);

	char bitToCheck = 0;
	if (shouldCheckBit) {
		lseek(file, (segmentsCount * segments_size) + 1, SEEK_SET);
		char byteToCheck;
		read(file, &byteToCheck, sizeof(byteToCheck));
	
		bitToCheck = (1 << (7 - position));
		bitToCheck &= byteToCheck;
		bitToCheck <<= position;
	}

	if (!shouldCheckBit || bitToCheck == '\x80') { //Check if the bit is enabled // The byte '\x80' is used to confirm a bit is 1
		printValueInRightFormat(parameter, file, type, segmentsCount, position);	
	} else {
		printf("The function '%s' is not enabled\n", parameter);
	}

	close(file);
}

void printAllValues(char* argv[], int shouldCheckBits) {
	int file = open(argv[1], O_RDONLY);

		if (file == -1) {
			errx(1, "File %s does not exitst!", argv[1]);
		}

		int fileSize = lseek(file, 0, SEEK_END);
		lseek(file, 0, SEEK_SET);

		char typeCheck;
		
		int segmentsCount = 0;
		int textSegments = 0, numSegments = 0, byteSegments = 0;

		do {
			if (read(file, &typeCheck, 1) == -1) {
				close(file);
				errx(1, "Couldn't read from file '%s'", argv[1]);
			}
				
			int size = getElementsSize(typeCheck);

			int shouldPrint[3] = {1, 1, 1};
			if (shouldCheckBits) {
				lseek(file, (segmentsCount * segments_size) + 1, SEEK_SET);
				char byteToCheck;
				if (read(file, &byteToCheck, sizeof(byteToCheck)) == -1) {
					close(file);
					errx(1, "Couldn't check if parameter is enabled");
				}
				
				char bitToCheck;
				for (int i = 0; i < 3; ++i) {
			
					bitToCheck = (1 << (7 - i));
					bitToCheck &= byteToCheck;
					bitToCheck <<= i;

					if (!(bitToCheck == '\x80')) {
						shouldPrint[i] = 0;
					}
				}
			}
			
			if (typeCheck == text_segment_char) {
				if (textSegPartsSize / 3 == textSegments) {
					++segmentsCount;
					continue;		
				}

				char text[size];
				for (int i = 0; i < 3; ++i) {
					if (shouldPrint[i]) {
						lseek(file, (segmentsCount * segments_size) + (metadata_size + i * size), SEEK_SET);
						if (read(file, &text, size) == -1) {
							close(file);
							errx(1, "Couldn't read from file '%s'", argv[1]);
						}
						printf("%s: %s\n", textSegParts[3 * textSegments + i].name, text);
					}
				}
				++textSegments;	
			}
			if (typeCheck == num_segment_char) {
				if (numSegPartsSize / 3 == numSegments) {
					++segmentsCount;
					continue;		
				}		
	
				int number;		
				for (int i = 0; i < 3; ++i) {
					if (shouldPrint[i]) {
						lseek(file, (segmentsCount * segments_size) + (metadata_size + i * size), SEEK_SET);
						if (read(file, &number, size) == -1) {
							close(file);
							errx(1, "Couldn't read from file '%s'", argv[1]);
						}
            	        printf("%s: %i\n", numSegParts[3 * numSegments + i].name, number);
					}
				}
				++numSegments;
			}
			if (typeCheck == byte_segment_char) {
				if (byteSegPartsSize / 3 == byteSegments) {
					++segmentsCount;
					continue;		
				}
	
				char byte;		
				for (int i = 0; i < 3; ++i) {
					if (shouldPrint[i]) {
						lseek(file, (segmentsCount * segments_size) + (metadata_size + i * size), SEEK_SET);
						if (read(file, &byte, size) == -1) {
							close(file);
							errx(1, "Couldn't read from file '%s'", argv[1]);
						}
				
						printf("%s: ", byteSegParts[3 * byteSegments + i].name);
						if (byte >= 0 && byte <= 9) { //Comparing bytes
							printf("%c\n", byte + '0');
						} else {
							printf("%c\n", byte);
						}
					}
				}
				++byteSegments;
			}				
			++segmentsCount;
		} while(lseek(file, segmentsCount * segments_size, SEEK_SET) < fileSize);
}

void changeBit(char* argv[]) {
	if (strcmp(argv[4], "1") && strcmp(argv[4], "0")) {
		errx(1, "Last argument should either '0' or '1'");
	}

	int file = open(argv[1], O_RDWR);
	
	if (file == -1) {
		errx(1, "File '%s' does not open!", argv[1]);
	}
 	
	char type;
	int segment, position;

	findTypeSegmentAndPosition(argv[3], &type, &segment, &position); 

	int segmentsCount = findRightSegment(argv[1], argv[3], file, type, segment);
	
	lseek(file, (segmentsCount * segments_size) + 1, SEEK_SET);
	
	char byteToChange;
	if (read(file, &byteToChange, sizeof(byteToChange)) == -1) {
		close(file);
		errx(1, "Couldn't read from file '%s'", argv[1]) ;
	}

	char byteToChangeWith;
	if (argv[4][0] == '0') {
		byteToChangeWith = (255 ^ (1 << (7 - position)));
		byteToChange &= byteToChangeWith;
	}
	else if (argv[4][0] == '1') {
		byteToChangeWith = (1 << (7 - position));
		byteToChange |= byteToChangeWith;
	}
	
	lseek(file, (segmentsCount * segments_size) + 1, SEEK_SET);
	if (write(file, &byteToChange, sizeof(byteToChange)) == -1) {
		close(file);
		errx(1, "Couldn't write to file '%s'", argv[1]);
	}
}

int main (int argc, char* argv[]) {
	if (argc == 5 && !strcmp("-s", argv[2])) {
		changeValue(argv, 1);
	}
	else if (argc == 5 && !strcmp("-S", argv[2])) {
		changeValue(argv, 0);
	}

	else if (argc >= 4 && !strcmp("-c", argv[2])) {
		createFile(argc, argv);
	}

	else if (argc == 4 && !strcmp("-g", argv[2])) {
		printValue(argv[1], argv[3], 1);
	}

	else if (argc == 4 && !strcmp("-G", argv[2])) {
		printValue(argv[1], argv[3], 0);
	}

	else if (argc == 3 && !strcmp("-l", argv[2])) {
		printAllValues(argv, 1);
	}

	else if (argc == 3 && !strcmp("-L", argv[2])) {
		printAllValues(argv, 0);
	}
	
	else if (argc > 3 && !strcmp("-l", argv[2])) {
		for (int i = 3; i < argc; ++i) {
			printValue(argv[1], argv[i], 1);	
		}
	}
	else if (argc > 3 && !strcmp("-L", argv[2])) {
		for (int i = 3; i < argc; ++i) {
			printValue(argv[1], argv[i], 0);
		}
	}
	
	else if (argc == 5 && !strcmp("-b", argv[2])) {
		changeBit(argv);	
	}

	else if (argc == 2 && !strcmp("-h", argv[1])) {
		printf("Homework3\n\n");
		printf("usage: main file argument       Change or use 'file' according to the specific 'argument'\n");
		printf("   or: main -h                  Print info about usage of the program\n\n");
		printf("Arguments:\n");
		printf("    -s <parameter> <new_vaule>  Changes the value of 'parameter' in 'file' to 'new_value' and changing 'parameter' status to active\n");
		printf("    -S <parameter> <new_value>  Changes the value of 'parameter' in 'file' to 'new_value' without changing 'parameter' status\n");
		printf("    -g <parameter>              Prints value in 'parameter', only if its status is active\n");
		printf("    -G <parameter>              Prints value in 'parameter', regardless of its status\n");
		printf("    -l                          Prints list of all active parameters in 'file'\n");
		printf("    -L                          Prints list of all parameters in 'file', regardless of their status\n");
		printf("    -l <parameter>..            Prints the values of all listed parameters, only if their status is active\n");
		printf("    -L <parameter>..            Prints the values of all listed parameters, regardless of their status\n");
		printf("    -b <parameter> <1 or 0>     Activate or deactivate 'parameter' status\n");
		printf("    -c <seg_pos seg_type>..     Creates 'file' with segments of type 'seg_type' in segment position'seg_pos'\n");
	}
	else {
		printf("Command is unknown or has missing arguments, use '-h' for help\n");
	}
}
