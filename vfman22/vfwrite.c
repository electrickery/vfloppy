/*
 * vfwrite.c version 2.2 part of the vfman 2.2 package
 *
 * This version delivered in 2010 by Fred Jan Kraan (fjkraan@xs4all.nl)
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

/*
 * vfwrite - writes a file to the image imageName
 * 
 * vfwrite <imageName> <fileName>
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

int debug = 0;

// file data instance
fileData_t fileData; 

//
d88Header_t d88Header;

// image data instance
imageData_t imageData;

// argument and options instance
argData_t argData;

int main(int argc,char **argv) {
        setLogLevel(LOG_INFO);
	argData_t *pArgData = &argData;
	parseArgs(argc, argv, pArgData);
	debug = pArgData->debug;
        setLogLevel(pArgData->debug);
	//blurp
	if (pArgData->imageName == NULL || pArgData->fileName == NULL) {
		msg(LOG_ERROR, "%s <imageName> <fileName>\n", argv[0]);
		msg(LOG_ERROR, "vfwrite 2.2 (c)2022 F.J. Kraan\n");
		exit(1);
	}
        
        // open image
	if (checkFileExistance(pArgData->imageName) == 0) {
		msg(LOG_WARN, "Image '%s' not found, exiting.\n", argv[1]);
	}
	char *imageName = pArgData->imageName;
	FILE *imageFile = openImageRW(imageName);
        
        // check file in file system
	if (checkFileExistance(pArgData->fileName) == 0) {
		msg(LOG_WARN, "File '%s' not found, exiting.\n", argv[2]);
	}
	fileData_t *pFileData = &fileData;
	fileDataInit(pFileData);
        
        // check file in image
	pFileData->fileName = (unsigned char*)baseFileName(pArgData->fileName);
	convertFileName(pFileData);
	pFileData->fileSize = getFileSize(pFileData);
	// check current file size in image
	findFileExtends(imageFile, pFileData);
	findFileBlocks(imageFile, pFileData);
        // check free space in image
	int freeBlocks = getFreeBlockCount(imageFile);
	int freeExtends = getFreeDirectorySpace(imageFile);
	// open file        
	pFileData->file = openFileRO((char *)pFileData->fileName);
	// report on free and needed sizes
	msg(LOG_DEBUG, "main; freeBlocks: %X, newFileBlocks: %X, fileInImageBlocks: %X\n", 
	       freeBlocks, getSizeInBlocks(pFileData->fileSize), getImageFileBlockCount(pFileData));
	msg(LOG_DEBUG, "main; freeExtends: %X, newFileExtends: %X, fileInImageExtends: %X\n",
	       freeExtends, getSizeInExtends(pFileData->fileSize), getImageFileExtendCount(pFileData));
	       
	int netBlocks  = 
		freeBlocks  - getSizeInBlocks(pFileData->fileSize)  + getImageFileBlockCount(pFileData);
	int netExtends = 
		freeExtends - getSizeInExtends(pFileData->fileSize) + getImageFileExtendCount(pFileData);
	// decide if it will fit and report 
	if (netBlocks >= 0 && netExtends >= 0) {
		msg(LOG_DEBUG, "main; enough free blocks & extends.\n");
	} else {
		msg(LOG_WARN, "main; not enough space for file in image. Net blocks: %d, Net Extends: %d\n",
		       netBlocks, netExtends);
		exit(2);
	}
        
        // erase the current file in the image
	if (pFileData->found == 1) {
		msg(LOG_DEBUG, "main; imageFile: %p, pFileData: %p\n", imageFile, pFileData);
		eraseFile(imageFile, pFileData);
	}
        
        // write file data and directory data to image
	fileDataInit(pFileData);
	convertFileName(pFileData);  // reinstates CP/M filename in pFileData->cpmFileName
	findUnusedBlocksExtends(imageFile, pFileData);
	writeDirectory(imageFile, pFileData);
	writeFile(imageFile, pFileData);

        // report space left
        int newFreeBlocks = getFreeBlockCount(imageFile);
        int newFreeExtends = getFreeDirectorySpace(imageFile);
        msg(LOG_INFO, "%d blocks free, (%d bytes). %d directory entries free\n", 
               newFreeBlocks, newFreeBlocks, newFreeExtends);
        
        // closing
	fclose(imageFile);		// close image
	fclose(pFileData->file);	// close file
	return 0;
}
