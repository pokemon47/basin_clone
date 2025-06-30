////////////////////////////////////////////////////////////////////////
// COMP1521 23T2 --- Assignment 2: `basin', a simple file synchroniser
// <https://cgi.cse.unsw.edu.au/~cs1521/23T2/assignments/ass2/index.html>
//
// Written by Asish Balasundaram (z5430777) on INSERT-DATE-HERE.
// INSERT-DESCRIPTION-OF-PROGAM-HERE
//
// 2023-07-16   v1.1    Team COMP1521 <cs1521 at cse.unsw.edu.au>


#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>

#include "basin.h"

#define NUM_UPDATES_SIZE  3

static void tabiGenerator(FILE *inputFile, FILE *targetFile, char pathName[]);
static void littleEndianWriter(uint64_t value, int numBytes, FILE *targetFile);

static void tbbiGenerator(FILE *inputFile, FILE *targetFile, int numRecords);
static void hashesMatchCheck(FILE *inputFile, uint64_t numInputBlocks, char pathName[], int bitArray[]);
static uint64_t littleEndianReader(FILE *inputFile, int numBytesToRead);
static void copyWriter(FILE *inputFile, FILE *targetFile, uint64_t numBytes, char stringCopied[]);

static void tcbiGenerator(FILE *inputFile, FILE *targetFile, int numRecords);
static void fileModeWriter(FILE *targetFile, char pathName[]);
static void tcbiSection2Writer(FILE *targetFile, int numUpdates, int blockIndexingArray[], char pathName[]);

void tcbiUser(FILE *inputFile);

/// @brief Create a TABI file from an array of filenames.
/// @param out_filename A path to where the new TABI file should be created.
/// @param in_filenames An array of strings containing, in order, the files
//                      that should be placed in the new TABI file.
/// @param num_in_filenames The length of the `in_filenames` array. In
///                         subset 5, when this is zero, you should include
///                         everything in the current directory.
void stage_1(char *out_filename, char *in_filenames[], size_t num_in_filenames) {
    // TODO: implement this.
    FILE *targetFile = fopen(out_filename, "w");
    if (targetFile == NULL) {
        perror(out_filename);
        exit(1);
    }

    // putting in the header
    char header[] = TYPE_A_MAGIC;
    for (int i = 0; i < MAGIC_SIZE; i++) {
        fputc(header[i], targetFile);
    }

    // number of records
    fputc(num_in_filenames, targetFile);

    // generating the tabi stuff for each file
    for (int i = 0; i < num_in_filenames; i++) {
        FILE *inputFile = fopen(in_filenames[i], "r");
        if (inputFile == NULL) {
            perror(in_filenames[i]);
            exit(1);
        }   
        tabiGenerator(inputFile, targetFile, in_filenames[i]);
        fclose(inputFile);
    }
    fclose(targetFile);
}

/// @brief Create a TBBI file from a TABI file.
/// @param out_filename A path to where the new TBBI file should be created.
/// @param in_filename A path to where the existing TABI file is located.
void stage_2(char *out_filename, char *in_filename) {
    // TODO: implement this.
    FILE *inputFile = fopen(in_filename, "r");
    if (inputFile == NULL) {
        perror(in_filename);
        exit(1);
        // doubt - cant return 1?
    }
    FILE *targetFile = fopen(out_filename, "w");
    if (targetFile == NULL) {
        perror(out_filename);
        exit(1);
        // doubt - cant return 1?
    }

    // checking the header of the input file
    char headerCheck[] = TYPE_A_MAGIC;
    for (int i = 0; i < MAGIC_SIZE; i++) {
        char letter;
        if ((letter = fgetc(inputFile)) == EOF) {
            perror("Header is ended earlier than expected");
            exit(1);
        } else if (letter != headerCheck[i]) {
            perror("The header of input file is not TABI");
            exit(1);
        }
    }
    char header[] = TYPE_B_MAGIC;
    for (int i = 0; i < MAGIC_SIZE; i++) {
        fputc(header[i], targetFile);
    }
    int numRecords;
    if ((numRecords = fgetc(inputFile)) == EOF) {
        perror("The input file is too short, number of records");
        exit(1);
    } else {
        fputc(numRecords, targetFile);
    }
    tbbiGenerator(inputFile, targetFile, numRecords);
    fclose(inputFile);
    fclose(targetFile);
}

/// @brief Create a TCBI file from a TBBI file.
/// @param out_filename A path to where the new TCBI file should be created.
/// @param in_filename A path to where the existing TBBI file is located.
void stage_3(char *out_filename, char *in_filename) {
    // TODO: implement this.
    FILE *inputFile = fopen(in_filename, "r");
    if (inputFile == NULL) {
        perror(in_filename);
        exit(1);
        // doubt - cant return 1?
    }
    FILE *targetFile = fopen(out_filename, "w");
    if (targetFile == NULL) {
        perror(out_filename);
        exit(1);
        // doubt - cant return 1?
    }

    char headerCheck[] = TYPE_B_MAGIC;
    for (int i = 0; i < MAGIC_SIZE; i++) {
        char letter;
        if ((letter = fgetc(inputFile)) == EOF) {
            perror("Header is ended earlier than expected");
            exit(1);
        } else if (letter != headerCheck[i]) {
            perror("The header of input file is not TABI");
            exit(1);
        }
    }
    char header[] = TYPE_C_MAGIC;
    for (int i = 0; i < MAGIC_SIZE; i++) {
        fputc(header[i], targetFile);
    }
    int numRecords;
    if ((numRecords = fgetc(inputFile)) == EOF) {
        perror("The input file is too short, number of records");
        exit(1);
    } else {
        fputc(numRecords, targetFile);
    }    
    tcbiGenerator(inputFile, targetFile, numRecords);
    fclose(inputFile);
    fclose(targetFile);      
}

/// @brief Apply a TCBI file to the filesystem.
/// @param in_filename A path to where the existing TCBI file is located.
void stage_4(char *in_filename) {
    // TODO: implement this.
    FILE *inputFile = fopen(in_filename, "r");
    if (inputFile == NULL) {
        perror(in_filename);
        exit(1);
    }

    // checking the header of the input file
    char headerCheck[] = TYPE_C_MAGIC;
    for (int i = 0; i < MAGIC_SIZE; i++) {
        char letter;
        if ((letter = fgetc(inputFile)) == EOF) {
            perror("Header is ended earlier than expected");
            exit(1);
        } else if (letter != headerCheck[i]) {
            perror("The header of input file is not TCBI");
            exit(1);
        }
    }

    int numRecords;
    if ((numRecords = fgetc(inputFile)) == EOF) {
        perror("The input file is too short, number of records");
        exit(1);
    }
    for (int i = 0; i < numRecords; i++) {
        tcbiUser(inputFile);
    }
    if (fgetc(inputFile) != EOF) {
        perror("The input file is bigger than specified");
        exit(1);
    }
    fclose(inputFile);
}

/////////////////////////////////////////////////////////////////////////////////
///////////////////////////// HELPER FUNCTIONS //////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

// This function writes the necessary information for each record
// to the tabi file based on the format mentioned in the assignment
// @param1 inputFile - a FILE pointer that points to the file to read from
// @param2 targetFile - a FILE pointer that points to the tabi file to write to
// @param3 pathName - a string array that contains the file name, this array is null-terminated
static void tabiGenerator(FILE *inputFile, FILE *targetFile, char pathName[]) {
    // pathfile length and name
    uint16_t pathNameLength = strlen(pathName);
    littleEndianWriter(pathNameLength, PATHNAME_LEN_SIZE, targetFile);
    for (int i = 0; i < pathNameLength; i++) {
        fputc(pathName[i], targetFile);
    }

    // number of blocks
    struct stat statBuff;
    stat(pathName, &statBuff);
    uint64_t numBlocks = number_of_blocks_in_file(statBuff.st_size);
    littleEndianWriter(numBlocks, NUM_BLOCKS_SIZE, targetFile);

    // the hashes
    for (int i = 0; i < numBlocks; i++) {
        // determining the number of bytes to read in
        int numBytesToRead = BLOCK_SIZE;
        if (i + 1 == numBlocks) {
            numBytesToRead = statBuff.st_size - (i * BLOCK_SIZE);
        }

        // reading in the number of bytes determined to an array
        char block[BLOCK_SIZE];
        for (int j = 0; j < numBytesToRead; j++) {
            block[j] = fgetc(inputFile);
        }

        // making the hash of the block
        uint64_t hash = hash_block(block, numBytesToRead);

        // writing the hash to the targetfile
        littleEndianWriter(hash, HASH_SIZE, targetFile);
    }

}

// This function writes the necessary information for each record
// to the tbbi file based on the format mentioned in the assignment
// @param1 inputFile - a FILE pointer that points to the tabi file to read from
// @param2 targetFile - a FILE pointer that points to the tbbi file to write to
// @param3 numRecords - an int that is the number of records to expect in the tabi file
static void tbbiGenerator(FILE *inputFile, FILE *targetFile, int numRecords) {
    for (int j = 0; j < numRecords; j++) {
        uint16_t pathNameLength = 0; 
        pathNameLength = littleEndianReader(inputFile, PATHNAME_LEN_SIZE);
        littleEndianWriter(pathNameLength, PATHNAME_LEN_SIZE, targetFile);
        char pathName[(pathNameLength + 1)];
        pathName[pathNameLength] = '\0';
        copyWriter(inputFile, targetFile, pathNameLength, pathName);

        uint64_t numBlocks = littleEndianReader(inputFile, NUM_BLOCKS_SIZE);
        littleEndianWriter(numBlocks, NUM_BLOCKS_SIZE, targetFile);

        uint64_t numMatchBytes = num_tbbi_match_bytes(numBlocks);
        if (numMatchBytes == 0) {
            continue;
        }
        int bitArray[MATCH_BYTE_BITS * numMatchBytes];
        for (int i = 0; i < MATCH_BYTE_BITS * numMatchBytes; i++) {
            bitArray[i] = 0;
        }
        hashesMatchCheck(inputFile, numBlocks, pathName, bitArray);

        //converting the array to bit value
        uint8_t matchByte = 0;
        for (int i = 0; i < MATCH_BYTE_BITS * numMatchBytes; i++) {
            uint8_t placeHolder = 1u;
            if (bitArray[i] == 1) {
                matchByte = matchByte | (placeHolder << ((MATCH_BYTE_BITS - (i % 8)) - 1));
            }
            if ((i + 1) % 8 == 0 && i != 0) {
                fputc(matchByte, targetFile);
                matchByte = 0;
            }
        }
    }
    if (fgetc(inputFile) != EOF) {
        perror("The input file is bigger than specified");
        exit(1);
    }
}

// This function makes a bitArray similar to the match bytes by comparing the 
// hashes in the tabi file and the hashes made from the file with 
// the name in pathName string array
// @param1 inputFile - a FILE pointer that points to the tabi file to read from
// @param2 numInputBlock - an int that is the number of block's hashes to compare
// @param3 pathName - a string array that contains the file name, this array is null-terminated
// @param4 bitArray - an int array to update which is used later for making the matchbytes
static void hashesMatchCheck(FILE *inputFile, uint64_t numInputBlocks, char pathName[], int bitArray[]) {
    FILE *receiverFile = fopen(pathName, "r");
    if (receiverFile == NULL) {
        // file does not exist in the bbb dir
        for (int i = 0; i < numInputBlocks * HASH_SIZE; i++) {
            fgetc(inputFile);
        }
        return;
    }    
    // determining the number of blocks in the receiver's file.
    struct stat statBuff;
    stat(pathName, &statBuff);
    uint64_t numReceiverBlocks = number_of_blocks_in_file(statBuff.st_size);

    // determining the number of blocks to compare.
    uint64_t numBlocksToCompare;
    if (numReceiverBlocks < numInputBlocks) {
        numBlocksToCompare = numReceiverBlocks;
    } else {
        numBlocksToCompare = numInputBlocks;
    }

    for (int i = 0; i < numBlocksToCompare; i++) {
        // determining the number of bytes to read in
        int numBytesToRead = BLOCK_SIZE;
        if (i + 1 == numBlocksToCompare && (statBuff.st_size - (i * BLOCK_SIZE) < BLOCK_SIZE)) {
            numBytesToRead = statBuff.st_size - (i * BLOCK_SIZE);
        }

        // reading in the number of bytes determined to an array
        char block[BLOCK_SIZE];
        for (int j = 0; j < numBytesToRead; j++) {
            block[j] = fgetc(receiverFile);
        }

        // making the hash of the block from receiver's file
        uint64_t receiverHash = hash_block(block, numBytesToRead);
        uint64_t senderHash = littleEndianReader(inputFile, HASH_SIZE);

        if (receiverHash == senderHash) {
            bitArray[i] = 1;
        }
    }
    if (numReceiverBlocks < numInputBlocks) {
        for (int i = 0; i < (numInputBlocks - numReceiverBlocks) * HASH_SIZE; i++) {
            fgetc(inputFile);
        }
    }
    fclose(receiverFile);
}

// This function writes the necessary information for each record
// to the tcbi file based on the format mentioned in the assignment
// @param1 inputFile - a FILE pointer that points to the tbbi file to read from
// @param2 targetFile - a FILE pointer that points to the tcbi file to write to
// @param3 numRecords - an int that is the number of records to expect in the tbbi file
static void tcbiGenerator(FILE *inputFile, FILE *targetFile, int numRecords) {
    for (int j = 0; j < numRecords; j++) {
        uint16_t pathNameLength = 0; 
        pathNameLength = littleEndianReader(inputFile, PATHNAME_LEN_SIZE);
        littleEndianWriter(pathNameLength, PATHNAME_LEN_SIZE, targetFile);
        char pathName[(pathNameLength + 1)];
        pathName[pathNameLength] = '\0';
        copyWriter(inputFile, targetFile, pathNameLength, pathName);
        
        // writing the permissions to the input file
        fileModeWriter(targetFile, pathName);
        
        struct stat statBuff;
        if (stat(pathName, &statBuff) != 0) {
            perror(pathName);
            exit(1);
        }

        uint32_t fileSize = statBuff.st_size;
        littleEndianWriter(fileSize, FILE_SIZE_SIZE, targetFile);

        uint64_t numBlocksTbbi = littleEndianReader(inputFile, NUM_BLOCKS_SIZE);
        if (number_of_blocks_in_file(fileSize) != numBlocksTbbi) {
            if (number_of_blocks_in_file(fileSize) > numBlocksTbbi) {
                perror("The number of blocks mentioned in TBBI file is lesser");
            } else {
                perror("The number of blocks mentioned in TBBI file is greater");
            }
            exit(1);
        }
        uint8_t matchByte = 0;
        uint64_t numUpdates = 0;
        int numBitsTotal = (1 + num_tbbi_match_bytes(numBlocksTbbi)) * MATCH_BYTE_BITS;
        int blockIndexingArray[numBitsTotal];
        for (int i = 0; i < numBitsTotal; i++) {
            blockIndexingArray[i] = 0;
        }
        for (int i = 0; i < numBitsTotal - MATCH_BYTE_BITS; i++) {
            if (i % MATCH_BYTE_BITS == 0) {
                matchByte = fgetc(inputFile);
            }
            if ((((matchByte >> ((MATCH_BYTE_BITS) - (i % MATCH_BYTE_BITS) - 1)) & 1u) == 0)
                && i < number_of_blocks_in_file(fileSize)) {
                blockIndexingArray[numUpdates] = i;
                numUpdates++;
            } else if ((((matchByte >> ((MATCH_BYTE_BITS) - (i % MATCH_BYTE_BITS) - 1)) & 1u) == 1)
                && i >= number_of_blocks_in_file(fileSize)) {
                perror("Incorrect padding");
                exit(1);
            }
        }

        littleEndianWriter(numUpdates, NUM_UPDATES_SIZE, targetFile);
        tcbiSection2Writer(targetFile, numUpdates, blockIndexingArray, pathName);
    }
    if (fgetc(inputFile) != EOF) {
        perror("The input file is bigger than specified");
        exit(1);
    }
}

// This file writes the permissions of a file mentioned by pathName string to the 
// file pointed to at by the targetFile FILE pointer
// @param1 targetFile - a FILE pointer that points to the tcbi file to write to
// @param4 pathName - a string array of the file name for which the permissions are written
static void fileModeWriter(FILE *targetFile, char pathName[]) {
    struct stat statBuff;
    if (stat(pathName, &statBuff) != 0) {
        perror(pathName);
        exit(1);
    }

    if (S_ISREG(statBuff.st_mode)) {
        fputc('-', targetFile);
    } else if (S_ISLNK(statBuff.st_mode)) {
        fputc('l', targetFile);
    } else {
        fputc('d', targetFile);
    }

    for (int i = 0; i < (MODE_SIZE - 1); i++) {
        if ((statBuff.st_mode & (S_IRUSR >> i)) == (S_IRUSR >> i)) {
            if (i % 3 == 0) {
                fputc('r', targetFile);
            } else if (i % 3 == 1) {
                fputc('w', targetFile);
            } else {
                fputc('x', targetFile);
            }
        } else {
            fputc('-', targetFile);
        }
    }
}

// This function writes the second section of the tcbi file for each record. It writes
// the block of data the receiver need, the number of updates for a record is equal to
// the number of non-padding 0 bits in the tbbi record
// @param1 targetFile - a FILE pointer that points to the tcbi file to write to
// @param2 numUpdates - an int that is the number of updates to expect
// @param3 blockIndexingArray - an int array that contains the index of blocks to be updated
// @param4 pathName - a string array that contains the file name to read the blocks of data required
static void tcbiSection2Writer(FILE *targetFile, int numUpdates, int blockIndexingArray[], char pathName[]) {
    FILE *senderInputFile = fopen(pathName, "r");
    if (senderInputFile == NULL) {
        perror(pathName);
        exit(1);
    }
    struct stat statBuff;
    if (stat(pathName, &statBuff) != 0) {
        perror(pathName);
        exit(1);
    }

    for (int i = 0; i < numUpdates; i++) {
        int blockIndex = blockIndexingArray[i];
        littleEndianWriter(blockIndex, BLOCK_INDEX_SIZE, targetFile);

        uint16_t updateLength = BLOCK_SIZE;
        if ((statBuff.st_size - ((blockIndex) * BLOCK_SIZE)) < BLOCK_SIZE) {
            updateLength = statBuff.st_size - ((blockIndex) * BLOCK_SIZE);
        }
        littleEndianWriter(updateLength, 2, targetFile);
        char block[BLOCK_SIZE] = {'\0'};
        if ((fseek(senderInputFile, ((blockIndex) * BLOCK_SIZE), SEEK_SET)) != 0) {
            perror("The file is shorter than expected");
            exit(1);
        }
        copyWriter(senderInputFile, targetFile, updateLength, block);
    }
    fclose(senderInputFile);
}

// this function reads a tcbi file pointed to at by the inputFile FILE pointer
// and updates the blocks mentioned for each file mentioned accordingly
// @param1 inputFile - a FILE pointer that points to the tcbi file to read from
void tcbiUser(FILE *inputFile) {
    uint16_t pathNameLength = littleEndianReader(inputFile, PATHNAME_LEN_SIZE);

    char pathName[pathNameLength + 1];
    pathName[pathNameLength] = '\0';
    for (int i = 0; i < pathNameLength; i++) {
        if ((pathName[i] = fgetc(inputFile)) == EOF) {
            perror("The TCBI file is shorter than expected, path name");
            exit(1);
        }
    }
    FILE *targetFile;
    if ((targetFile = fopen(pathName, "r+")) == NULL) {
        targetFile = fopen(pathName, "w");
    }
    
    // converting the permission string from input file to binary
    int permArray[MODE_SIZE];
    mode_t permByte = 0;
    for (int i = 0; i < MODE_SIZE; i++) {
        if ((permArray[i] = fgetc(inputFile)) == EOF) {
            perror("The TCBI file is shorter than expeted, permission string");
            exit(1);
        }
        if (permArray[i] != '-') {
            permByte = permByte | (1u << (MODE_SIZE - i - 1));
        }
    }
    if ((chmod(pathName, permByte)) != 0) {
        perror(pathName);
        exit(1);
    }
    // getting the sender's file size from the tcbi file
    uint32_t fileSize = littleEndianReader(inputFile, FILE_SIZE_SIZE);
    truncate(pathName, fileSize);
    uint64_t numUpdates = littleEndianReader(inputFile, NUM_UPDATES_SIZE);

    for (int i = 0; i < numUpdates; i++) {
        uint64_t blockIndex = littleEndianReader(inputFile, BLOCK_INDEX_SIZE);
        uint16_t updateLength = littleEndianReader(inputFile, UPDATE_LEN_SIZE);
        fseek(targetFile, (blockIndex * BLOCK_SIZE), SEEK_SET);
        char block[updateLength];
        copyWriter(inputFile, targetFile, updateLength, block);
    }  
    fclose(targetFile);
}

// this functuion is used read the value in little endian format from a file
// @param1 inputFile - a FILE pointer that points to the file to read from
// @param2 numBytesToRead - The number of bytes in which the value is stored in
static uint64_t littleEndianReader(FILE *inputFile, int numBytesToRead) {
    uint64_t value = 0;
    for (int i = 0; i < numBytesToRead; i++) {
        uint64_t temp = 0;
        if ((temp = fgetc(inputFile)) == EOF) {
            perror("The input file is too short, little endian reader");
            exit(1);
        }
        value = value | (temp << (i * 8));
    }
    return value;
}

// this function reads from the file pointed to at by the inputFile pointer and writes to the 
// the file pointed to at by the targetFile pointer. It does this for the number of bytes
// mentioned in the numBytes. It also stores the bytes copied over in an array seperately.
// @param1 inputFile - a FILE pointer that points to the tabi file to read from
// @param2 targeteFile - a FILE pointer that points to the tbbi file to write to
// @param3 numBytes - an int that is the number of bytes to copy over
// @param4 stringCopied - a char array which stores the value of each byte copied over
static void copyWriter(FILE *inputFile, FILE *targetFile, uint64_t numBytes, char stringCopied[]) {
    for (int i = 0; i < numBytes; i++) {
        int value = 0;
        if ((value = fgetc(inputFile)) == EOF) {
            perror("The input file is too short, copyWriter");
            exit(1);
        } else {
            fputc(value, targetFile);
            stringCopied[i] = value;
        }
    }
}

// This function takes a value and writes the value to the file pointed to at 
// by the target FILE pointer in little-endian over the number of bytes 
// mentioned in numBytes.
// param1 value - an int which is the value to be written
// param2 numBytes - an int that is the number of bytes to write over
// param3 targetFile - a FILE pointer that point to file to write to
static void littleEndianWriter(uint64_t value, int numBytes, FILE *targetFile) {
    for (int i = 0; i < numBytes; i++) {
        uint8_t temp = 0xff;
        temp = temp & (value >> (i * 8));
        fputc(temp, targetFile);
    }
}
