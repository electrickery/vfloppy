/*
 * vf.c version 2.2 part of the vfman 2.2 package
 *
 * This version delivered in 2013 by Fred Jan Kraan (fjkraan@xs4all.nl)
 *
 * vfman is placed under the GNU General Public License in March 2010.
 *
 *  This file is part of Vfman 2.2.
 *
 *  Vfman is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Vfman is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with vfman; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * vf.c - functions for vformat, vfread, vfwrite, vferase
 */

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include "vf.h"
#include "logger.h"
#include "imageCode.h"

extern int debug;

void parseArgs (int argc, char **argv, argData_t *pArgData) {
	int index, c;
	pArgData->imageName = NULL;
	pArgData->fileName = NULL;
	pArgData->debug = LOG_INFO;
	
	opterr = 0;
     
	while ((c = getopt (argc, argv, "d:")) != -1) {
		switch (c) {
		case 'd':
			pArgData->debug = atoi(optarg);
			break;
		case '?':
			if (optopt == 'd')
				msg(LOG_ERROR, "Option -%c requires an argument.\n", optopt);
			else if (isprint (optopt))
				msg(LOG_ERROR, "Unknown option `-%c'.\n", optopt);
			else
				msg(LOG_ERROR, "Unknown option character `\\x%x'.\n", optopt);
			return;
		default:
			abort ();
		}
	}
	
	c = 0;
	for (index = optind; index < argc; index++) {
		msg(LOG_DEBUG, " parseArgs; Arg name %d: %s\n", index, argv[index]);
		if (c == 0)
		{
			pArgData->imageName = argv[index];
		}
		if (c == 1)
		{
			pArgData->fileName = argv[index];
		}
		c++;
	}	
        msg(LOG_DEBUG, " parseArgs; image name: %s\n", pArgData->imageName);
        msg(LOG_DEBUG, " parseArgs; file name: %s\n", pArgData->fileName);
        msg(LOG_DEBUG, " parseArgs; debug: %d\n", pArgData->debug);
}

void reformatFileName(unsigned char *pFileName, const unsigned char *pExtend) {
    int i, extendIndex, fileNameIndex;
    
    for (i = 0; i < (FILENAMEBASESIZE + 1 + FILEEXTSIZE); i++) pFileName[i] = ' ';
    pFileName[12] = 0;
    for (i = 0; i < FILENAMEBASESIZE; i++) {
        extendIndex = i + FILENAMEBASE;
        fileNameIndex = i;
        if (pExtend[extendIndex] == ' ' || pExtend[extendIndex] == 0) {
            fileNameIndex--; // compensate for early exit
            break;
        }
        pFileName[fileNameIndex] = pExtend[extendIndex];
    }
    fileNameIndex++;
    pFileName[fileNameIndex++] = '.';
    for (i = 0; i < FILEEXTSIZE; i++) {
        extendIndex = i + FILEEXTBASE;
        if (pExtend[extendIndex] == ' ' || pExtend[extendIndex] == 0) break;
        pFileName[fileNameIndex + i] = pExtend[extendIndex];
    }
}

void printDirectory(FILE *imageFile) {
	int i, fileSize;
	long int extendLocation;
	unsigned char extend[EXTEND];
	unsigned char fileName[13];
	fileName[8] = '.';
	fileName[12] = 0;
        unsigned char *pFileName = &fileName[0];
        unsigned char *pExtend = &extend[0];
        
        msg(LOG_INFO, "usr filename    flags ext  size\n");
        msg(LOG_INFO, "--  ------------  ---  -  -----\n");
	for (i = 0; i < DIRECTORYEXTENDS; i++) {
		extendLocation = getDirExtendLocation(i);
		msg(LOG_DEBUG," printDirectory; reading at %lX\n", extendLocation);
		
		readDisk((char *)extend, extendLocation, EXTEND, imageFile);
		if (extend[0] != FORMATPATTERN) {
/*
			memcpy(&fileName[0], &extend[1], 8);
			memcpy(&fileName[9], &extend[9], 3);
*/
                        reformatFileName(pFileName, pExtend);
			fileName[READONLYBIT]   &= 0x7F;
			fileName[SYSTEMFILEBIT] &= 0x7F;
			fileName[ARCHIVEBIT]    &= 0x7F;
			fileSize = extend[EXTENDRECORDS] * RECORD + (extend[EXTENDNO] & 1) * (1<<14);
			msg(LOG_ERROR, "%d:  %s  %c%c%c  %d  %5d\n", 
			       extend[0], 
			       fileName, 
			       setCharOnHibit(extend[READONLYBIT],   'r'), 
			       setCharOnHibit(extend[SYSTEMFILEBIT], 's'), 
			       setCharOnHibit(extend[ARCHIVEBIT],    'a'), 
			       extend[EXTENDNO] / 2, 
			       fileSize );
		}
	}
}

char setCharOnHibit(unsigned char testChar, char replaceChar) {
	return (testChar > 0x7F) ? replaceChar : ' ';
}

void convertFileName(fileData_t *pFileData) {
	// This is copied from the old code, but in need of serious refactoring. 
	// To be combined with the support for files from other than the local directory
	
	/* conversie fileName dirFileName
	   - to uppercase
	   - add spaces to basename to 8 chars
	   - remove dot
	   - add spaces to extension to 3 chars
	 */
	int i = 0;
	int j = 0;
	int c;
	for (c = 0; c < 11; c++)
		pFileData->cpmFileName[c] = ' ';
	pFileData->cpmFileName[11] = '\0';

	/* walk though the array to fill and check the
	 * input array:
	 * - end of string: 	exit
	 * - a dot:		write the rest at the extension location
	 * Most problems are not checked here, but will hopefully fail
	 * nicely at filename comparison.
	 */
	for (i = 0; i < 12; i++) {
		if (pFileData->fileName[i] == '\0')	// stop on end of string
			break;
		if (pFileData->fileName[i] == '.') {	// skip to filename extension
			j = 8;		// start filename extension
		} else {
			if (j < 11) {	// don't copy the end of string
				pFileData->cpmFileName[j] = toupper(pFileData->fileName[i]);
				j++;
			}	
		}
	} 

	if (debug > 1) printf(" convertFileName; filename: \'%s\'\n", pFileData->cpmFileName); 
}

void findFileExtends(FILE *imageFile, fileData_t *pFileData) {
/** findFileExtends - searches for a filename in the image and writes the used
  *            extend numbers to the struct extends[] and sets the found flag.
  */
	int i;
	long int extendLocation;
	unsigned char extend[EXTEND];
	unsigned char foundFilename[11];
	foundFilename[11] = '\0';
	for (i = 0; i < DIRECTORYEXTENDS; i++) { // iterate through directory entries
		extendLocation = getDirExtendLocation(i);
		msg(LOG_DEBUG, " findFileExtends; reading at %lX\n", extendLocation);
		
		readDisk((char *)extend, extendLocation, EXTEND, imageFile);	
		
		if (extend[0] != FORMATPATTERN) {
			memcpy(&foundFilename, &extend[FILENAMEBASE], 11);
			stripHighbit(foundFilename);
			if (strcmp((const char *)foundFilename, (const char *)pFileData->cpmFileName) == 0) {
				pFileData->extends[extend[EXTENDNO]] = i;
				msg(LOG_DEBUG, " findFileExtends; found %s at %X extend number %X\n", foundFilename, i, extend[i]);
				pFileData->found = 1;
			}
		}
 	}
		
	msg(LOG_DEBUG, " findFileExtends; found extends ");
	if (pFileData->extends[i] != -1) msg(LOG_DEBUG, "%X: %X, ", i, pFileData->extends[i]);
	msg(LOG_DEBUG, "\n");
}

void stripHighbit(unsigned char *fileName) {
        fileName[READONLYBIT]   = 0x7f & fileName[READONLYBIT];	/* clear Read only bit */
        fileName[SYSTEMFILEBIT] = 0x7f & fileName[SYSTEMFILEBIT];	/* clear System file bit */
        fileName[ARCHIVEBIT]    = 0x7f & fileName[ARCHIVEBIT];	/* clear Archive bit */ 
}

void findFileBlocks(FILE *imageFile, fileData_t *pFileData) {
	int i;
	for (i = 0; i < DIRECTORYEXTENDS; i++) {
		if (pFileData->extends[i] != -1) {
			msg(LOG_DEBUG, " findFileBlocks; copying from extend %X\n", pFileData->extends[i]);
			copyBlocks(imageFile, pFileData, pFileData->extends[i]);
		}
	}
}

int checkFileExistance(char *fileName) {
	FILE *file = fopen(fileName, "rb");
	if (file == NULL) {
		msg(LOG_DEBUG, " checkFileExistance; file %s does not exist\n", fileName);
		return 0;
	}
	fclose(file);
	msg(LOG_DEBUG, " checkFileExistance; file %s does exist\n", fileName);
	return 1;
}

void copyBlocks(FILE *imageFile, fileData_t *pFileData, int extendNumber) {
	int i, j, block;
	long int extendLocation;
	unsigned char extend[EXTEND];
	
	extendLocation = getDirExtendLocation(extendNumber);
	msg(LOG_DEBUG, "  copyBlocks; reading at %lX, ", extendLocation);
	
	readDisk((char *)extend, extendLocation, EXTEND, imageFile);	
	
	for (j = 0; j < EXTBLOCKS; j++) {
		block = extend[EXTBLOCKBASE + j];
		if (block != 0) storeBlockNumber(pFileData, block);
	}
	// set for every extend in correct order, so last extend will remain
	pFileData->recordsInLastExtend = extend[EXTENDRECORDS]; 
	
	if (getLogLevel() >= LOG_DEBUG) {
		msg(LOG_DEBUG, "  copyBlocks; found blocks: ");
		for (i = 0; i < BLOCKS_ON_DISK; i++) {
			if (pFileData->fileBlocks[i] == 0) break;
			msg(LOG_DEBUG, "%X, ", pFileData->fileBlocks[i]);
		}
		msg(LOG_DEBUG, "\n");
		msg(LOG_DEBUG, "  copyBlocks; records in last extend: %X, in last block: %X\n", 
		       pFileData->recordsInLastExtend, 
		       (pFileData->recordsInLastExtend & 0x0F) == 0 ? 0x10 : pFileData->recordsInLastExtend & 0x0F);
	}
}

void storeBlockNumber(fileData_t *pFileData, int block) {
	int i = 0;
	while (pFileData->fileBlocks[i] != 0) i++;	// Find first empty location
	pFileData->fileBlocks[i] = block;
	msg(LOG_DEBUG, "   storeBlockNumber; copy block: %X to fileBlocks[%X]\n", block, i);
}

void lastBlockCopy(FILE *imageFile, fileData_t *pFileData, int block) {
	long int recordAddress;
	int i;
	unsigned char record[RECORD];
	int recordsInLastBlock = 
		(pFileData->recordsInLastExtend & 0x0F) == 0 ? 0x10 : pFileData->recordsInLastExtend & 0x0F;
	msg(LOG_DEBUG, "  lastBlockCopy; %X records to copy\n", recordsInLastBlock);
	for (i = 0; i < recordsInLastBlock; i++) {
		recordAddress = getRecordLocation(pFileData->fileBlocks[block], i);
		msg(LOG_DEBUG, "  lastBlockCopy; record %X at %lX\n", i, recordAddress);
		
		readDisk((char *)record, recordAddress, RECORD, imageFile);	

		writeDisk((char *)record, 0, RECORD, pFileData->file);
	}	
}

void otherBlockCopy(FILE *imageFile, fileData_t *pFileData, int block) {
	long int sectorAddress;
	int i;
	unsigned char blockSector[SECTOR];
	for (i = 0; i < (BLOCK / SECTOR); i++) {
		sectorAddress = getBlockSectorLocation(pFileData->fileBlocks[block], i);
		msg(LOG_DEBUG, "  otherBlockCopy; sector %X at %lX\n", i, sectorAddress);
		
		readDisk((char *)blockSector, sectorAddress, SECTOR, imageFile);	
		
		writeDisk((char *)blockSector, 0, SECTOR, pFileData->file);
	}
}

void fileDataInit(fileData_t *pFileData) {
	pFileData->cpmFileName[11] = '\0';
	pFileData->found = 0;
	int i;
	for (i = 0; i < BLOCKS_ON_DISK; i++) pFileData->fileBlocks[i] = 0;
	for (i = 0; i < DIRECTORYEXTENDS; i++) pFileData->extends[i] = -1;	
}

int getFreeBlockCount(FILE *imageFile) {
	int i, j, blockCount = 0;
	long int extendLocation;
	unsigned char extend[EXTEND];
	for (i = 0; i < DIRECTORYEXTENDS; i++) {
		extendLocation = getDirExtendLocation(i);
		msg(LOG_DEBUG, "  getFreeBlockCount; reading at %X: %lX,", i, extendLocation);
	
		readDisk((char *)extend, extendLocation, EXTEND, imageFile);	
		msg(LOG_DEBUG, " extend[STATUS]: %X\n", extend[STATUS] );
		if (extend[STATUS] != FORMATPATTERN) {
			for (j = 0; j < EXTBLOCKS; j++) {
				if (extend[EXTBLOCKBASE + j] != 0) blockCount++;
			}
			msg(LOG_DEBUG, "  getFreeBlockCount; used blockCount: %d\n", blockCount);
		}
	}
	msg(LOG_DEBUG, "  getFreeBlockCount; found %d used blocks\n", blockCount);
	return BLOCKS_ON_DISK - blockCount;
}

int getSizeInRecords(int size) {
	return (size + RECORD - 1) / RECORD;
}

int getImageFileBlockCount(fileData_t *pFileData) {
	int i, blockCount = 0;
	for (i = 0; i < BLOCKS_ON_DISK; i++) {
		if (pFileData->fileBlocks[i] != 0) blockCount++;	
	}
	msg(LOG_DEBUG, "  getImageFileBlockCount; checked %X blocks, %X counted\n", i, blockCount);
	return blockCount;
}

int getImageFileExtendCount(fileData_t *pFileData) {
	int i, extendCount = 0;
	for (i = 0; i < DIRECTORYEXTENDS; i++) {
		if (pFileData->extends[i] != -1) extendCount++;
	}
	msg(LOG_DEBUG, "  getImageFileExtendCount; checked %X extends, %X counted\n", i, extendCount);
	return extendCount;
}

unsigned int getFileSize(fileData_t *pFileData) {
	int result, lSize = 0;
	FILE *file = fopen((char *)pFileData->fileName, "rb");
	if (file == NULL) {
		msg(LOG_ERROR, " failed to open %s\n", pFileData->fileName);
		exit(1);
	} else {
		result = fseek (file , 0 , SEEK_END);
		lSize = ftell (file);
		msg(LOG_DEBUG, "  getFileSize; returned: %d. File is %d bytes (0x%X)\n", result, lSize, lSize);
		fclose(file);
	}
	return lSize;
}

int getSizeInBlocks(int size) {
	return (size + (BLOCK - 1)) / BLOCK;
}

int getSizeInExtends(int size) {
	return (size + (EXTBLOCKS * BLOCK) - 1) / (EXTBLOCKS * BLOCK);
}

int getFreeDirectorySpace(FILE *imageFile) {
	int i, freeExtendCount = 0;
	long int extendLocation;
	unsigned char extend[EXTEND];
	for (i = 0; i < DIRECTORYEXTENDS; i++) {
		extendLocation = getDirExtendLocation(i);
		msg(LOG_DEBUG, "  getFreeDirectorySpace; reading at %X: %lX\n", i, extendLocation);
	
		readDisk((char *)extend, extendLocation, EXTEND, imageFile);	
		
		if (extend[STATUS] == FORMATPATTERN) freeExtendCount++;

	}
	msg(LOG_DEBUG, " getFreeDirectorySpace; %X extends free\n", 
		freeExtendCount);
	return freeExtendCount;
}

void eraseFile(FILE *imageFile, fileData_t *pFileData) {
	int i;
	long int extendLocation;
	unsigned char extend[EXTEND];
	
	for (i = 0; i < DIRECTORYEXTENDS; i++) {
		if (pFileData->extends[i] != -1) {
			extendLocation = getDirExtendLocation(pFileData->extends[i]);
			msg(LOG_DEBUG, " eraseFile; erasing extend %X: %X at %lX\n", i, pFileData->extends[i], extendLocation);

			readDisk((char *)extend, extendLocation, EXTEND, imageFile);
				
			extend[0] = FORMATPATTERN;
				
			writeDisk((char *)extend, extendLocation, EXTEND, imageFile);			
		}
	}
	msg(LOG_DEBUG, " eraseFile; end\n");
}

void findUnusedBlocksExtends(FILE *imageFile, fileData_t *pFileData) {
	int i,  j;
	long int extendLocation;
	unsigned char extend[EXTEND];
	// start with making all blocks available
	for (i = 1; i < BLOCKS_ON_DISK; i++) pFileData->fileBlocks[i] = 0;
	for (i = 0; i < DIRECTORYEXTENDS; i++) {
		extendLocation = getDirExtendLocation(i);
		readDisk((char *)extend, extendLocation, EXTEND, imageFile);
		msg(LOG_DEBUG, " findUnusedBlocksExtends; checking extend %X: at %lX (STATUS=%X)\n", 
			       i,  extendLocation, extend[STATUS]);
		if (extend[STATUS] != FORMATPATTERN) {
			msg(LOG_DEBUG, " findUnusedBlocksExtends; found used extend at %X: ", i);
			for (j = 0; j < EXTBLOCKS; j++) {
				if (extend[EXTBLOCKBASE + j] != 0) {
					pFileData->fileBlocks[extend[EXTBLOCKBASE + j]] = extend[EXTBLOCKBASE + j];
					msg(LOG_DEBUG, " %X:,", extend[EXTBLOCKBASE + j]);
				}
			}
			pFileData->extends[i] = i;
			msg(LOG_DEBUG, "\n");
		} else {
			msg(LOG_DEBUG, " findUnusedBlocksExtends; free extend:%X\n", i);
			pFileData->extends[i] = -1;
		}
	}
	if (getLogLevel() >= LOG_DEBUG) {
		msg(LOG_DEBUG, " findUnusedBlocksExtends; blocks found to be free: ");
		for (i = 1; i < BLOCKS_ON_DISK; i++) {
			if (pFileData->fileBlocks[i] == 0)
				msg(LOG_DEBUG, " %X,", i);
		}
		msg(LOG_DEBUG, "\n");
		msg(LOG_DEBUG, " findUnusedBlocksExtends; extends found to be free: ");
		for (i = 1; i < DIRECTORYEXTENDS; i++) {
			if (pFileData->extends[i] == -1)
				msg(LOG_DEBUG, " %X,", i);
		}
		msg(LOG_DEBUG, "\n");
	}
}

int getNextFreeBlock(fileData_t *pFileData, int index) {
	int i = index + 1;
	while (pFileData->fileBlocks[i] != 0) i++;
	return i;
}

int getNextFreeExtend(fileData_t *pFileData, int index) {
	int i = index + 1;
	while (pFileData->extends[i] != -1) i++;
	return i;
}

void writeDirectory(FILE *imageFile, fileData_t *pFileData) {
	int i, blocksThisExtend, extCount, blkCount, fileInExtends, fileInBlocks, lastExtend = 0;
	int blockIndex = 0, extendIndex = -1;	// blocks should start at 1, extends at 0
	long int extendLocation;
	unsigned char extend[EXTEND];
	
	int remainingSize = pFileData->fileSize;
	fileInExtends = getSizeInExtends(pFileData->fileSize);
	fileInBlocks = getSizeInBlocks(pFileData->fileSize);
	msg(LOG_DEBUG, " writeDirectory; fileInBlocks: %X, remainingSize: %X\n", 
		fileInBlocks, remainingSize);
	for (extCount = 0; extCount < fileInExtends; extCount++) {
		memset(extend, 0x0, EXTEND);
		for (i = 0; i < 11; i++) extend[FILENAMEBASE + i] = pFileData->cpmFileName[i];
		
		if (extCount + 1 == fileInExtends) lastExtend = 1;
		extend[EXTENDNO] = extCount * 2; // use bits 1 and up
		extendIndex = getNextFreeExtend(pFileData, extendIndex);
		msg(LOG_DEBUG, " writeDirectory; extCount=%X, extIdx=%X:", extCount, extendIndex);
		
		extendLocation = getDirExtendLocation(extendIndex);
		
		if (getSizeInBlocks(remainingSize) > EXTBLOCKS) {
			blocksThisExtend = EXTBLOCKS;
		} else {
			blocksThisExtend = getSizeInBlocks(remainingSize);
		}
		msg(LOG_DEBUG, " bTE: %X,", blocksThisExtend);
		for (blkCount = 0; blkCount < blocksThisExtend; blkCount++) {
			blockIndex = getNextFreeBlock(pFileData, blockIndex);
			extend[EXTBLOCKBASE + blkCount] = blockIndex;
			msg(LOG_DEBUG, " blkCount=%X, blkIdx=%X ", blkCount, blockIndex);
//			if (blkCount >= EXTBLOCKS) break;
		}
		msg(LOG_DEBUG, "\n");
		if (lastExtend) {
			extend[EXTENDRECORDS] = getSizeInRecords(remainingSize) & 0x7F;
			if (extend[EXTENDRECORDS] == 0) extend[EXTENDRECORDS] = 0x80;
			
			if (getSizeInRecords(remainingSize) > 0x7F) extend[EXTENDNO] = extend[EXTENDNO] + 1;
			msg(LOG_DEBUG, " writeDirectory; (last) remainingSize = 0x%X, fileInRecords = %X, extend[EXTENDRECORDS] = %X, extend[EXTENDNO] = %X\n",
			       remainingSize, getSizeInRecords(remainingSize), extend[EXTENDRECORDS], extend[EXTENDNO]);
			remainingSize = 0;
		} else {
			extend[EXTENDRECORDS] = 0x80;			// any non-last extend is full.
			extend[EXTENDNO] = extend[EXTENDNO] + 1;	// full = 256 records
			msg(LOG_DEBUG, " writeDirectory; remainingSize = 0x%X, fileInRecords = %X, extend[EXTENDRECORDS] = %X, extend[EXTENDNO] = %X\n",
			       remainingSize, getSizeInRecords(remainingSize), extend[EXTENDRECORDS], extend[EXTENDNO]);
			remainingSize -= EXTBLOCKS * BLOCK;
		}

		msg(LOG_DEBUG, " writeDirectory; location: %lX, updated remaining size: 0x%X\n", extendLocation, remainingSize);
		writeDisk((char *)extend, extendLocation, EXTEND, imageFile);
	}
}

void writeFile(FILE *imageFile, fileData_t *pFileData) {
	int blkCount, blockSectorCount, blockIndex = 0;
	unsigned char buffer[SECTOR];
	unsigned long int sectorLocation, fileSectorLocation = 0;
	for (blkCount = 0; blkCount < getSizeInBlocks(pFileData->fileSize); blkCount++) {
		blockIndex = getNextFreeBlock(pFileData, blockIndex);
		for (blockSectorCount = 0; blockSectorCount < SECTORSPERBLOCK; blockSectorCount++) {
			sectorLocation = getBlockSectorLocation(blockIndex, blockSectorCount);
			msg(LOG_DEBUG, "blkCount: %X, blockIndex %X, secCount: %X, sectorLocation: %lX\n",
			       blkCount, blockIndex, blockSectorCount, sectorLocation);
			memset(buffer, FORMATPATTERN, SECTOR);
			readDisk((char *)buffer, fileSectorLocation, SECTOR, pFileData->file);
			fileSectorLocation += SECTOR;
			writeDisk((char *)buffer, sectorLocation, SECTOR, imageFile);
		}
	}
}

void initImageHeader(d88Header_t *pD88Header) {
	int i;
	for (i = 0; i < 9; i++)	pD88Header->rsrv[i] = 0;
	pD88Header->protect = 0;
	pD88Header->type = 0;
	pD88Header->size = TRACK * HEADS * TRACKS + SECTORBASE;
	for (i = 0; i < TRACKS * HEADS; i++)   pD88Header->trkptr[i] = i * TRACK + SECTORBASE;
	for (i = TRACKS * HEADS; i < 164; i++) pD88Header->trkptr[i] = 0;
}





