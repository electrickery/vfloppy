 /*
 * imageCode.h version 2.2 part of the Vfman 2.2 package
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
 *  along with Vfman; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

//imageCode.h - header data for imageCode.c - image oriented functions for vfread.c,
//        vfwrite.c, vf=erase.c and vf2d88.c

#ifndef IMAGECODE_H
#define	IMAGECODE_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#define D88_MAX_TRACK_SIDES  164
#define D88_HEADER_NAME_SIZE  17
#define D88_HEADER_RSRV_SIZE   9
#define D88_SECTOR_BASE_LOCATION 0x02B0   
#define D88_MAX_SECTORS     1280 // 40 * 2 * 16
#define D88_SECTORS_PER_SIDE 16
    
#define BLOCKSTARTTRACK   4
#define BLOCKSPERCYLINDER 4
#define BLOCKSPERTRACK    2

typedef struct {
    unsigned char track;
    unsigned char side;
    unsigned char sector;
    unsigned char flags;
    long sectorLocation;
} ImageIndex_t;

// masks for ImageIndex_t->flags
#define SD_SECTOR_SIZE_MASK  0x03 // bits 0 and 1
#define SD_DATA_DOUBLER_MASK 0x04 // bit 2
#define SD_DOUBLE_DENSITY    0x08 // bit 3 
#define SD_HIGH_DENSITY      0x10 // bit 4 

#define SD_SIZE_128  0x00
#define SD_SIZE_256  0x01
#define SD_SIZE_512  0x02
#define SD_SIZE_1024 0x03

#define CPMLOGICALSECTORSIZE 128

typedef struct {
    char addrLLSB;
    char addrLSB;
    char addrMSB;
    char addrMMSB;
} SectorAddress_t;

typedef struct {
    char name[D88_HEADER_NAME_SIZE]; // 0x00 - 0x011
    char rsvr[D88_HEADER_RSRV_SIZE]; // 0x12 - 0x1B
    char protect;                    // 0x1C
    char type;                       // 0x1D
    SectorAddress_t size;            // 0x1E
    SectorAddress_t sectorAddress[D88_MAX_TRACK_SIDES];
} D88Header_t;

typedef struct {
    char track;
    char side;
    char sector;
    char sectorSize;
    char nsecLSB;          // number is sector in this track?
    char nsecMSB;
    char density;
    char del;           // deleted?
    char stat;          // status?
    char rsrv[5];
    char sizeLSB;
    char sizeMSB;
} D88SectorHeader_t;

//0000:0000 - 0000:02AF   Header consisting of:
//  0000:0000 - 0000:0011	Image name              0x12
//  0000:0012 - 0000:001B	rsvr (reserved?)        0x0A
//  0000:001C - 0000:001D	protect & type          0x02
//  0000:001E - 0000:0021   size                        0x04
//  0000:0022 - 0000:02AF	Track index locator
//0000:02B0 - 0000:02BF	First sector header; 00 00 01 01  10 00 00 00  00 00 00 00  00 00 00 01
//0000:02C0 - 0000:03BF	First sector
//0000:03C0 - 0000:03CF	Second sector header; 00 00 02 01  10 00 00 00  00 00 00 00  00 00 00 01
//0000:03D0 - 0000:04CF	Second sector
//...
//...
//0005:51A0 - 0005:51AF	Last sector header: 27 01 10 01  01 00 00 00  00 00 00 00  00 00 00 01
//0005:5200 - 0005:52AF	Last sector
//
//Sector header bytes:
//0 	track (c)
//1 	side (h)
//2 	sector (r)
//3 	sector size (n), (1=128 bytes, 1=256 bytes, 2=512 bytes, 3=1024 bytes)
//4-5 	nsec (number of sectors in this track)
//6 	dens (density; 0=double density, 1=high density, ?=single density)
//7 	del (?)
//8 	stat (?)
//9-D 	rsrv (reserved?)
//E-F 	size (little endian)

/* imageCode.c */
unsigned long int getSectorLocation(int track, int record);
long int getBlockSectorLocation(int block, int sector);
//unsigned long int getSectorLocation2(int track, int record, int driveId);
int createImageIndex(FILE *readFile);
unsigned long blockMap(int, int);
int imageRead(int, unsigned char *, unsigned long, int);
int imageWrite(int, unsigned char *, unsigned long, int);
int getTrackFromBlock(int block);
int getSideFromBlock(int sector);
int valueComposer(unsigned char llsb, unsigned char lsb, unsigned char msb, unsigned char mmsb);
int checkImageHeader(FILE *file);
int readDisk(char *buffer, long int location, unsigned int size, FILE *readFile);
int writeDisk(char *buffer, long int location, unsigned int size, FILE *writeFile);

#ifdef	__cplusplus
}
#endif

#endif	/* IMAGECODE_H */

