/*
 * vfwrite.c version 2.2 part of the vfman 2.2 package
 *
 * This version delivered in 2013 by Fred Jan Kraan (fjkraan@xs4all.nl)
 *
 * vfwrite is placed under the GNU General Public License in March 2010.
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
	argData_t *pArgData = &argData;
	parseArgs(argc, argv, pArgData);
	debug = pArgData->debug;
        setLogLevel(pArgData->debug);
	
	if (pArgData->imageName == NULL || pArgData->fileName == NULL)
	{
		msg(LOG_ERROR, "%s <imageName> <fileName>\n", argv[0]);
		msg(LOG_ERROR, "vferase 2.1 (c)2010 F.J. Kraan\n");
		exit(1);
	}

	if (checkFileExistance(pArgData->imageName) == 0)
	{
		msg(LOG_WARN, "Image '%s' not found, exiting.\n", argv[1]);
	}
	char *imageName = pArgData->imageName;
	FILE *imageFile = NULL;
	
	fileData_t *pFileData = &fileData;

	pFileData->fileName = (unsigned char*)pArgData->fileName;
	
	imageFile = openImageRW(imageName);	// open image

	convertFileName(pFileData);
	
	findFileExtends(imageFile, pFileData);
	
	if (pFileData->found == 1) 
	{
		msg(LOG_DEBUG, "main; imageFile: %p, pFileData: %p\n", imageFile, pFileData);
		eraseFile(imageFile, pFileData);
	}
	fclose(imageFile);		// close image
	return 0;
}
