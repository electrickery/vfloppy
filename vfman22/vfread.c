/*
 * vfread.c version 2.2 part of the vfman 2.2 package
 *
 * This version delivered in 2013 by Fred Jan Kraan (fjkraan@xs4all.nl)
 *
 * vfread is placed under the GNU General Public License in March 2010.
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
 *  along with Vfloppy; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * vfread - two modes; 
 *   without fileName, the directory of the image is displayed
 *   with fileName, the file is extracted from the image
 * 
 * vfread <imageName> [<fileName>]
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

int debug = 2;

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
		
	if (pArgData->imageName == NULL) // No image name means print usage and exit
	{
                msg(LOG_ERROR, "%s <imageName> [<fileName>]\n", argv[0]);
                msg(LOG_ERROR, "vfread 2.3 (c)2023 F.J. Kraan\n");
		exit(1);
	}

	imageData_t *pImageData = &imageData;
	pImageData->fileName = pArgData->imageName;
	pImageData->file = 0;
	
	if (checkFileExistance(pImageData->fileName) == 0) // Non-existing image means exit
	{
		msg(LOG_ERROR, " %s does not exists, exiting\n",
                        pImageData->fileName);
		exit(1);
	}	
	
	if (pArgData->fileName == NULL) // No 2nd arguments means print directory
	{
		msg(LOG_DEBUG, "main; directory mode: reading %s\n",
                        pImageData->fileName);
		
		pImageData->file = openImageRO(pImageData->fileName);
		
		printDirectory(pImageData->file);
		
		int freeBlocks = getFreeBlockCount(pImageData->file);
		int freeExtends = getFreeDirectorySpace(pImageData->file);
		msg(LOG_INFO, "%d blocks free, (%d bytes). %d directory entries free\n", 
		       freeBlocks, freeBlocks * BLOCK, freeExtends);
		
		fclose(pImageData->file);
		
		exit(0);
	}
	if (checkFileExistance(pArgData->fileName) == 1) // no overwiting files
	{
		fprintf(stderr," %s already exists, exiting\n", argv[2]);
		exit(1);
	}
	
	// Being here means exporting a file from the image
	fileData_t *pFileData = &fileData;
	fileDataInit(pFileData);
	
	pFileData->fileName = (unsigned char*)pArgData->fileName;

	msg(LOG_DEBUG, "main; file copy mode: retrieving %s from %s\n",
                pFileData->fileName, pImageData->fileName);
	
	convertFileName(pFileData);
	
	pImageData->file = openImageRO(pImageData->fileName);
	
	findFileExtends(pImageData->file, pFileData);
	
	if (pFileData->found == 1)
	{	
		findFileBlocks(pImageData->file, pFileData);
		
		createFile(pImageData->file, pFileData);
	}
	else
		msg(LOG_INFO, "main; file '%s' not found in %s\n",
                        pFileData->fileName, pImageData->fileName);
	
	fclose(pImageData->file);
	
	exit(0);
}


