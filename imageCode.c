/*
 * imageCode.c version 2.2 part of the Epspd88 2.2 package
 *
 * Copyright 1996 Justin Mitchell (madmitch@discordia.org.uk) and friends as Vfloppy.
 *
 * This version delivered in 2013 by Fred Jan Kraan (fjkraan@xs4all.nl)
 *
 * Epspd88 is placed under the GNU General Public License in June 2009.
 *
 *  This file is part of Epspd88 2.2.
 *
 *  Epspd88 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Epspd88 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Epspd88; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * imageCode - image functions for epspd.c.
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "epspHeaders.h"
#include "epspv3.h"
#include "fdc.h"
#include "logger.h"
#include "imageCode.h"


//ImageIndex_t ii[D88_MAX_SECTORS], *imageIndex = &ii;

ImageIndex_t ii0[D88_MAX_SECTORS], *imageIndex0 = &ii0[0];
ImageIndex_t ii1[D88_MAX_SECTORS], *imageIndex1 = &ii1[0];
ImageIndex_t ii2[D88_MAX_SECTORS], *imageIndex2 = &ii2[0];
ImageIndex_t ii3[D88_MAX_SECTORS], *imageIndex3 = &ii3[0];
ImageIndex_t *imageIndex4[4] = { &ii0[0], &ii1[0], &ii2[0], &ii3[0] };

extern struct driveInfo driveInfo;

extern char *deviceNames[DEVICE_NAME_SIZE];

unsigned long int getSectorLocation(int track, int record, int driveId) {
    unsigned long location = 0;
    int i;
    int side   = 0;
    ImageIndex_t *imageIndex = imageIndex4[driveId];
    int diskSector, diskSectorHalf;
    int logicalSector = record - 1;
    
    // Mapping logical records (128 bytes) to physical disk sectors (256 bytes)
    // including the translation to two sides and that the disk sectors number
    // from 1 to 16.
    if (record <= D88_SECTORS_PER_SIDE * 2) {  // Fix found by Ivo Burkart, 2015-10-27. Thanks!
        diskSector = (logicalSector / 2) + 1;
        side = 0;
     } else {
        diskSector = (logicalSector / 2) - D88_SECTORS_PER_SIDE + 1;
        side = 1;
    }
    diskSectorHalf = logicalSector & 1;

    msg(LOG_DEBUG, "   getSectorLocation; Drive: %d, Track %d, Record: %d > Side: %d, Sector: %d, %s half",
            driveId, track, record, side, diskSector, (diskSectorHalf) ? "second" : "first");
    for (i = 0; i < D88_MAX_SECTORS; i++) {       
        if (imageIndex[i].track == track && imageIndex[i].side == side && imageIndex[i].sector == diskSector) {
            msg(LOG_TRACE, "  Location: %05X @ index: %d", imageIndex[i].sectorLocation, i);
            location = imageIndex[i].sectorLocation + (diskSectorHalf ? CPMLOGICALSECTORSIZE : 0);
            break;
        }
    }
    msg(LOG_DEBUG, "\n");
    return location;
}

int createImageIndex(int fileDescriptor, int driveId) {
    D88Header_t hdr, *header = &hdr;
    char in[D88_HEADER_NAME_SIZE], *imageName = &in[0];
    
    ImageIndex_t *imageIndex = imageIndex4[driveId];
    
    D88SectorHeader_t sh, *sectorHeader = &sh;
        
    int location, result, sectorCount = 0;
    
    imageRead(fileDescriptor, (unsigned char *)header, 0, sizeof(D88Header_t));
    memcpy(imageName, header, D88_HEADER_NAME_SIZE);
    msg(LOG_TRACE, "    createImageIndex; image name: '%s'\n", imageName);
    msg(LOG_TRACE, "    createImageIndex; image protect & type:  %02X %02X\n", 
            (unsigned char)header->protect, (unsigned char)header->type);
    msg(LOG_TRACE, "    createImageIndex; image size:  %07X\n", 
            valueComposer(header->size.addrLLSB, header->size.addrLSB, 
            header->size.addrMSB, header->size.addrMMSB));
    
    location = D88_SECTOR_BASE_LOCATION;
    while(1) {
//        if (location == 0) break;
//        if (sectorCount > D88_MAX_TRACK_SIDES) break;
        msg(LOG_TRACE, "    createImageIndex; track/side start location at: %07X\n", location);
        result = imageRead(fileDescriptor, (unsigned char *)sectorHeader, location, sizeof(D88SectorHeader_t));
        if (result != sizeof(D88SectorHeader_t)) break;
        imageIndex[sectorCount].track  = sectorHeader->track;
        imageIndex[sectorCount].side   = sectorHeader->side;
        imageIndex[sectorCount].sector = sectorHeader->sector;
        imageIndex[sectorCount].sectorLocation = location + sizeof(D88SectorHeader_t);
        imageIndex[sectorCount].flags  = sectorHeader->sectorSize;
        imageIndex[sectorCount].flags += (sectorHeader->density == 0) ? SD_DOUBLE_DENSITY : 0;
        imageIndex[sectorCount].flags += (sectorHeader->density == 1) ? SD_HIGH_DENSITY   : 0;
        msg(LOG_TRACE, "    createImageIndex; Track: %d, Side: %d, Sector: %d, flags: %02X, Location: %07X\n", 
                imageIndex[sectorCount].track, imageIndex[sectorCount].side, imageIndex[sectorCount].sector, 
                imageIndex[sectorCount].flags, imageIndex[sectorCount].sectorLocation);
        location += sectorHeader->sizeLSB + (sectorHeader->sizeMSB << 8) + sizeof(D88SectorHeader_t);
        sectorCount++;
    }
    msg(LOG_DEBUG,"    createImageIndex; indexed %d sectors in imageIndex %d\n", sectorCount, driveId);
    return 1;
}

int imageRead(int fd, unsigned char *data, unsigned long location, int len)
{
        int err;
        int ct;
        msg(LOG_TRACE, "\n   imageRead; fd(%d, %s) block=%06lX len=%02X:  ", fd, deviceNames[fd], location, len);
        msg(LOG_TRACE, "Dir.block: %02lX\n", (((location / 0x80) - 0x11) / 16 + 1) & 0xFF);
        if(lseek(fd, location, SEEK_SET) == -1)
        {
                perror("\n> > > > > > imageRead: lseek < < < < < <");
                msg(LOG_ERROR, "\n   imageRead; fd(%d, %s) location=%06lX len=%02X:  ", fd, deviceNames[fd], location, len);
                return -1;
        }
        err = read(fd, data, len);
        if (getLogLevel() >= LOG_TRACE) {
            msg(LOG_TRACE, "   imageRead; ");
                for(ct=0; ct < len; ct++)
                        msg(LOG_TRACE, "%02X ",data[ct] & 0xFF);
        }
        msg(LOG_TRACE, "\n");
        if (err < 0) {
                msg(LOG_ERROR, "    imageRead; err=%d\n", err);
        }
        return err;
}

int imageWrite(int fd, unsigned char *data, unsigned long block, int len)
{
        int err;
        int ct;
        msg(LOG_TRACE, "   imageWrite; fd(%d, %s) block %06lX, len %02X:  ", fd, deviceNames[fd], block, len);
        msg(LOG_TRACE, "Dir.block: %02lX\n", (((block / 0x80) - 0x11) / 16 + 1) & 0xFF);
        if(lseek(fd, block, 0) == -1)
        {
                perror("\n> > > > > > imageWrite: lseek < < < < < <");
                printf("\n   imageWrite; fd(%d, %s) block=%06lX len=%02X:  ", fd, deviceNames[fd], block, len);
                return -1;
        }
        err = write(fd, data, len);
        if (getLogLevel() >= LOG_TRACE) {
            msg(LOG_TRACE, "   imageWrite; ");
                for(ct=0; ct < len; ct++)
                        msg(LOG_TRACE, "%02X ",data[ct] & 0xFF);
        }
        if (err < 0) {
                msg(LOG_ERROR, "   imageWrite; err=%d\n", err);
                return err;
        }
        msg(LOG_TRACE, "\n");
        return 1;
}

int valueComposer(unsigned char llsb, unsigned char lsb, unsigned char msb, unsigned char mmsb) {
    return llsb + (lsb << 8) + (msb << 16) + (mmsb << 24);
}

int checkImageHeader(int fd) {
    unsigned char sff[D88_HEADER_NAME_SIZE + 1], *signatureFromFile = &sff[0];
    int result;
    
    signatureFromFile[D88_HEADER_NAME_SIZE] = 0;
    
    result = imageRead(fd, signatureFromFile, 0, D88_HEADER_NAME_SIZE);
    if (result != D88_HEADER_NAME_SIZE) {
        msg(LOG_WARN, "   checkImageHeader; Error (%d) reading file %d.\n", result, fd);
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
