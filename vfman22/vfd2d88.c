/*
 * vfd2d88.c version 2.2 part of the vfman 2.2 package
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

/*
 * vfd2d88 - converts an old type vfloppy image to the d88 format
 * 
 * vfd2d88 <vfImageName> <d88ImageName>
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
#include "imageCode.h"

int debug = 0;

// file data instance
fileData_t fileData; 

//
d88Header_t d88Header;

// image data instance
imageData_t imageData;

// argument and options instance
argData_t argData;

int main(int argc,char **argv)
{	
	argData_t *pArgData = &argData;
	parseArgs(argc, argv, pArgData);
	debug = pArgData->debug;
	
	if (pArgData->imageName == NULL || pArgData->fileName == NULL)
	{
		fprintf(stderr, "%s <vfdImageName> <d88ImageName>\n", argv[0]);
		fprintf(stderr, "vfd2d88 2.0 (c)2010 F.J. Kraan\n");
		exit(1);
	}

	if (checkFileExistance(pArgData->imageName) == 0)
	{
		printf("VFD image '%s' not found, exiting.\n", argv[1]);
	}
	char *vfdImageName = pArgData->imageName;
	FILE *vfdImageFile = NULL;
	
	if (checkFileExistance(pArgData->fileName) == 0)
	{
		printf("D88 image '%s' not found, exiting.\n", argv[2]);
	}
	fileData_t *pFileData = &fileData;
	fileDataInit(pFileData);

	pFileData->fileName = (unsigned char*)pArgData->fileName;
	
	vfdImageFile = openFileRO(vfdImageName);	// open image
	pFileData->file = openFileRW((char *)pFileData->fileName);	// open file

	int i, j, k = 0, vfdOffset = 0x80;
	unsigned char sector[SECTOR];
	
	for (i = 0; i < BLOCKS_ON_DISK; i++)
	{
		for (j = 0; j < SECTORSPERBLOCK; j++)
		{
			readDisk((char *)sector, k * SECTOR + vfdOffset, SECTOR, vfdImageFile);
			writeDisk((char *)sector, getBlockSectorLocation(i, j), SECTOR, pFileData->file);
			k++;
		}
	}
	
	fclose(vfdImageFile);		// close image
	fclose(pFileData->file);	// close file
	return 0;
}
