/*
 * ImageCode.c version 2.2 part of the vfman 2.2 package
 *
 * This version delivered in 2013 by Fred Jan Kraan (fjkraan@xs4all.nl)
 *
 * vfman is placed under the GNU General Public License in March 2010.
 *
 *  This file is part of Vflman 2.2.
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
 *  along with Vman; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 *imageCode.c - functions for vformat, vfread, vfwrite, vferase
 */
        
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "imageCode.h"
#include "logger.h"
#include "vf.h"

/*
 * 
 */
ImageIndex_t ii0[D88_MAX_SECTORS], *imageIndex = &ii0[0];

char *d88VfloppySignature = "D88 VFloppy";
char *d88TelediskSignature = "TELEDISK";

int createImageIndex(FILE *readFile) {
    D88Header_t hdr, *header = &hdr;
    char in[D88_HEADER_NAME_SIZE], *imageName = &in[0];
      
    D88SectorHeader_t sh, *sectorHeader = &sh;
        
    int location = 0, result, sectorCount = 0;
    int maxTrack = 0, maxSide = 0, maxSector = 0;
    
    readDisk((char *)header, location, sizeof(D88Header_t), readFile);
    memcpy(imageName, header, D88_HEADER_NAME_SIZE);
    msg(LOG_DEBUG, "  createImageIndex; image name: '%s'\n", imageName);
    msg(LOG_DEBUG, "  createImageIndex; image protect & type:  %02X %02X\n", 
            (unsigned char)header->protect, (unsigned char)header->type);
    msg(LOG_DEBUG, "  createImageIndex; image size:  %07X\n", 
            valueComposer(header->size.addrLLSB, header->size.addrLSB, 
            header->size.addrMSB, header->size.addrMMSB));
    
    location = D88_SECTOR_BASE_LOCATION;
    while(1) {
//        if (location == 0) break;
//        if (sectorCount > D88_MAX_TRACK_SIDES) break;
        msg(LOG_TRACE, "  createImageIndex; track/side start location at: %07X\n", location);
//        result = imageRead(fileDescriptor, (unsigned char *)sectorHeader, location, sizeof(D88SectorHeader_t));
        result = readDisk((char *)sectorHeader, location, sizeof(D88SectorHeader_t), readFile);
        if (result != 1) {
                msg(LOG_ERROR,"  createImageIndex; readDisk result not 1 but %d", result);
                break;
        } else {
                msg(LOG_DEBUG,"  createImageIndex; readDisk result OK");
        }
        imageIndex[sectorCount].track  = sectorHeader->track;
        imageIndex[sectorCount].side   = sectorHeader->side;
        imageIndex[sectorCount].sector = sectorHeader->sector;
        imageIndex[sectorCount].sectorLocation = location + sizeof(D88SectorHeader_t);
        imageIndex[sectorCount].flags  = sectorHeader->sectorSize;
        imageIndex[sectorCount].flags += (sectorHeader->density == 0) ? SD_DOUBLE_DENSITY : 0;
        imageIndex[sectorCount].flags += (sectorHeader->density == 1) ? SD_HIGH_DENSITY   : 0;
        msg(LOG_TRACE, "  createImageIndex; Track: %d, Side: %d, Sector: %d, flags: %02X, Location: %07X\n", 
                imageIndex[sectorCount].track, imageIndex[sectorCount].side, imageIndex[sectorCount].sector, 
                imageIndex[sectorCount].flags, imageIndex[sectorCount].sectorLocation);
        location += sectorHeader->sizeLSB + (sectorHeader->sizeMSB << 8) + sizeof(D88SectorHeader_t);
        sectorCount++;
        if (sectorHeader->track > maxTrack)   maxTrack = sectorHeader->track;
        if (sectorHeader->side > maxSide)     maxSide = sectorHeader->side;
        if (sectorHeader->sector > maxSector) maxSector = sectorHeader->sector;
    }
    msg(LOG_DEBUG,"  createImageIndex; indexed %d sectors in imageIndex %d\n", sectorCount);
    msg(LOG_DEBUG,"  createImageIndex; max track: %d, max side: %d, max sector: %d\n", 
            maxTrack, maxSide, maxSector);
    return 1;
}

unsigned long int getSectorLocation(int track, int record) {
    unsigned long location = 0;
    int i;
    int side   = 0;
    int diskSector, diskSectorHalf;
    
    // Mapping logical records (128 bytes, 0 to 63) to physical disk sectors 
    // (256 bytes, 1 to 16, 2 sides)
    if (record < D88_SECTORS_PER_SIDE * 2) {
        diskSector = (record / 2) + 1;
        side = 0;
     } else {
        diskSector = (record / 2) - D88_SECTORS_PER_SIDE + 1;
        side = 1;
    }
    diskSectorHalf = record % 2;        // 0 = first, 1 = second half

    msg(LOG_DEBUG, "   getSectorLocation; Track %d, Record: %d > Side: %d, Sector: %d, %s half",
            track, record, side, diskSector, (diskSectorHalf) ? "second" : "first");
    for (i = 0; i < D88_MAX_SECTORS; i++) {       
        if (imageIndex[i].track == track && imageIndex[i].side == side && imageIndex[i].sector == diskSector) {
            msg(LOG_DEBUG, "   getSectorLocation;  Location: %05X @ index: %d", imageIndex[i].sectorLocation, i);
            location = imageIndex[i].sectorLocation + (diskSectorHalf ? CPMLOGICALSECTORSIZE : 0);
            break;
        }
    }
    msg(LOG_DEBUG, "\n");
    if (location == 0) msg(LOG_ERROR, \
        "   getSectorLocation; record location not found. Track: %d, Record: %d, Sector: %d, Side: %d\n", 
            track, record, diskSector, side);
    return location;
}

long int getBlockSectorLocation(int block, int blockSector) {
    int mySide   = getSideFromBlock(block);
    int myTrack  = getTrackFromBlock(block);
    int myCylinderSector = (block * SECTORSPERBLOCK + blockSector) % (D88_SECTORS_PER_SIDE  * HEADS);
    int myRecord = myCylinderSector << 1;
    msg(LOG_DEBUG, "  getBlockSectorLocation; Blk: %d, BlkSector: %d > Trk: %d, Sd: %d, Sec: %d, Rec: %d\n",
            block, blockSector, myTrack, mySide, myCylinderSector + 1, myRecord);
    return getSectorLocation(myTrack, myRecord);
}


int getTrackFromBlock(int block) {
    return (block / BLOCKSPERCYLINDER) + BLOCKSTARTTRACK;
}

int getSideFromBlock(int block) {
    return (block + 1) % HEADS;
}

long int getDirExtendLocation (int count) {
//    int record = count / (SECTOR / EXTEND);
//    int extendOffset = (count % (SECTOR / EXTEND)) * EXTEND;
//    long int mySectorLocation2 = getSectorLocation(DIRTRACK, record);
//    long int myLocation2 = mySectorLocation2 + extendOffset;
    
    long int myLocation = (SECTORBASE + SECTORHEADER + (DIRTRACK * TRACK * HEADS)) + 	// base offset
		count * EXTEND +					// per extend offset
		(count / (SECTOR / EXTEND)) * SECTORHEADER;		// per sector offset
    
//    if (myLocation != myLocation2) {
//        msg(LOG_ERROR, "  getDirExtendLocation; count: %d, track: %d, record: %d, ext: %02X > %05X, %05X\n",
//            count, DIRTRACK, record, count, myLocation, myLocation2);
//    }
    return myLocation;
}

long int getRecordLocation(int block, int record) {
    int oddRecord = record % 2;
    long int myLocation2 = getBlockSectorLocation (block, record >> 1);
//    msg(LOG_ERROR, "  getRecordLocation; blk: %d, rec: %d ", block, record);
    if (oddRecord) {
        myLocation2 += RECORD;
    }
    
    long int myLocation =  BLOCKBASE + 
            block * (BLOCK / SECTOR) * (SECTORHEADER + SECTOR) +
            record * (RECORD) +
            SECTORHEADER + SECTORHEADER * (record / (SECTOR / RECORD));
    if (myLocation != myLocation2) {
        msg(LOG_ERROR, "  getRecordLocation; location mismatch  %05X, %05X\n", myLocation, myLocation2);
    }
    return myLocation;
}

int valueComposer(unsigned char llsb, unsigned char lsb, unsigned char msb, unsigned char mmsb) {
    return llsb + (lsb << 8) + (msb << 16) + (mmsb << 24);
}

int checkImageHeader(FILE *readFile) {
    char sff[D88_HEADER_NAME_SIZE + 1], *signatureFromFile = &sff[0];
    int result;
    
    signatureFromFile[D88_HEADER_NAME_SIZE] = 0;
    
    result = readDisk(signatureFromFile, 0, D88_HEADER_NAME_SIZE, readFile);
    //imageRead(fd, signatureFromFile, 0, D88_HEADER_NAME_SIZE);
    if (result != D88_HEADER_NAME_SIZE) {
        msg(LOG_WARN, "   checkImageHeader; Error (%d) reading file.\n", result);
        return 0;
    }
    msg(LOG_TRACE, "   checkImageHeader; signatureFromFile '%s'\n", signatureFromFile);
    
    result = strncmp(d88VfloppySignature, (const char *)signatureFromFile, D88_HEADER_NAME_SIZE);
    if (result == 0) {
        return 1;
    }
    result = strncmp(d88TelediskSignature, (const char *)signatureFromFile, D88_HEADER_NAME_SIZE);
    if (result == 0) {
        return 1;
    }
    msg(LOG_WARN, "   checkImageHeader; signature comparison failed.\n");
    return 0;
}

void createFile(FILE *imageFile, fileData_t *pFileData) {
	pFileData->file = fopen((const char *)pFileData->fileName, "wb");
  	if (pFileData->file == NULL) {
		msg(LOG_ERROR,"failed to open %s for writing\n", pFileData->fileName);
		exit(2);
	} 
	msg(LOG_INFO, " createFile; opened %s for writing\n", pFileData->fileName);
	int i;
	for (i = 0; i < BLOCKS_ON_DISK; i++) { // block 0 is the directory
		if (pFileData->fileBlocks[i] == 0) {
			msg(LOG_DEBUG, " createFile; position %X is 0, finished\n", i);
			break;	// end of blocks
		}
		msg(LOG_DEBUG, " createFile; position %X has block %X\n", i,  pFileData->fileBlocks[i]);
		if (pFileData->fileBlocks[i + 1] == 0) {
			lastBlockCopy(imageFile, pFileData, i);
		} else {
			otherBlockCopy(imageFile, pFileData, i);
		}
	}
	fclose(pFileData->file);
}

void createImage(char *imageName, d88Header_t *pD88Header) {
	// sector header
	struct d88sct_t sectorHeader;
	sectorHeader.c      = 0;	// cylinder,track 0 - 39
	sectorHeader.h      = 0;	// head 0 - 1
	sectorHeader.r      = 0;	// sector 1 - 16
	sectorHeader.n      = 1;	// ?
	
	sectorHeader.nsec    = 0x1000;
	sectorHeader.dens    = 0;
	sectorHeader.del     = 0;
	
	sectorHeader.stat    = 0;
	sectorHeader.rsrv[0] = 0;
	sectorHeader.rsrv[1] = 0;
	sectorHeader.rsrv[2] = 0;
	
	sectorHeader.rsrv[3] = 0;
	sectorHeader.rsrv[4] = 0;
	sectorHeader.size    = 0x0100;

	// Fill the sector with format pattern
	unsigned char sectorBuffer[SECTOR];
	memset(sectorBuffer, FORMATPATTERN, SECTOR);
	
	msg(LOG_INFO, " createImage; opening image %s\n", imageName);
	FILE *imageFile = fopen(imageName, "wb");
	if (imageFile == NULL) {
		fprintf(stderr," failed to open %s\n", imageName);
		exit(2);
	} 

	// Filling the image
	unsigned char sector, head, track;
	msg(LOG_INFO, " createImage; Creating a disk image with %d tracks, %d sides, %d sectors of %d bytes\n",
		TRACKS, HEADS, SECTORSPERTRACK, SECTOR);
	fwrite(pD88Header, SECTORBASE, 1, imageFile);
	for (track = 0; track < TRACKS; track++) {
		sectorHeader.c = track;
		for (head = 0; head < HEADS; head++) {
			sectorHeader.h = head;
			for (sector = 1; sector <= SECTORSPERTRACK; sector++) {
				sectorHeader.r = sector;
				fwrite(&sectorHeader, SECTORHEADER, 1, imageFile);
				fwrite(sectorBuffer, SECTOR, 1, imageFile);
			}
		}
	} 
}

FILE *openImageRO(char *imageName) {
	FILE *imageFile = openFileRO(imageName);
        createImageIndex(imageFile);
	return imageFile;
}

FILE *openFileRO(char *imageName) {
        int result, lSize;
	FILE *imageFile = fopen(imageName, "rb");
	if (imageFile == NULL) {
		msg(LOG_ERROR, " failed to open %s\n", imageName);
		exit(1);
	} else {
		result = fseek (imageFile , 0 , SEEK_END);
		lSize = ftell (imageFile);
		msg(LOG_DEBUG, " openFileRO; returns: %d. File is %d bytes (0x%X)\n", result, lSize, lSize);
	}
	return imageFile;
}

FILE *openImageRW(char *imageName) {
        FILE *imageFile = openFileRW(imageName);
        createImageIndex(imageFile);
	return imageFile;
}

FILE *openFileRW(char *imageName) {
	int result, lSize;
	FILE *imageFile = fopen(imageName, "r+b");
	if (imageFile == NULL) {
		msg(LOG_ERROR, " failed to open %s\n", imageName);
		exit(1);
	} else {
		result = fseek (imageFile , 0 , SEEK_END);
		lSize  = ftell (imageFile);
		msg(LOG_DEBUG, " openFileRW; returned: %d. File is %d bytes (0x%X)\n", result, lSize, lSize);
	}
	return imageFile;
}

int readDisk(char *buffer, long int location, unsigned int size, FILE *readFile) {
//	printf("readDisk; readDisk: %p, buffer: %p\n", readFile, buffer);
	int result;
	msg(LOG_DEBUG, "   readDisk; reading 0x%X bytes", size);
		
	msg(LOG_DEBUG, " at %lX,", location);
	result = fseek(readFile, location, SEEK_SET);
	msg(LOG_DEBUG, " fseek returned: %d,", result);
	
	result = fread(buffer, size, 1, readFile);
	msg(LOG_DEBUG, " fread returned: %d\n", result);
        return result;
}

int writeDisk(char *buffer, long int location, unsigned int size, FILE *writeFile) {
	int result;
	msg(LOG_DEBUG, "  writeDisk; writing 0x%X bytes,", size);
			
	if (location != 0) {	// file writes don't need fseek for append only files
		msg(LOG_DEBUG, " at %lX,", location);
		result = fseek(writeFile, location, SEEK_SET);
		msg(LOG_DEBUG, " fseek returned: %d,", result);
	}
	
	result = fwrite(buffer, size, 1, writeFile);
	msg(LOG_DEBUG, " fwrite returned: %d\n", result);
        return result;
}
