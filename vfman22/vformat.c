/*
 * vformat.c version 2.2 part of the vfman 2.2 package
 *
 * This version delivered in 2013 by Fred Jan Kraan (fjkraan@xs4all.nl)
 *
 * vfread is placed under the GNU General Public License in July 2002.
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
 * vformat - create a d88 image file for a fixed disk configuration.
 *           The disk geometry is 40 tracks, double sided, 16 sectors
 *           per track, 256 bytes per sector. 
 * 
 * vformat <imageName>
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

/* Exit codes:
 * 1 - error opening header image
 * 2 - error opening image
 * 3 - image file already exists
 */
int debug = 0;

// file data instance
fileData_t fileData; 

//
d88Header_t d88Header;

// image data instance
imageData_t imageData;

// argument and options instance
argData_t argData;

int main(int argc,char *argv[])
{
	argData_t *pArgData = &argData;
	parseArgs(argc, argv, pArgData);
	
	char *imageName       = pArgData->imageName;
	char *imageHeaderName = "D88 VFloppy";
	
	d88Header_t *pD88Header = &d88Header;
	strcpy((char *)&pD88Header->title, imageHeaderName);
	
	debug = pArgData->debug;

	if (pArgData->imageName == NULL) 
	{
		fprintf(stderr, "Usage: %s <imageName>\n", argv[0]);
		fprintf(stderr, "vformat 2.1 (c)2013 F.J. Kraan\n");
		exit(0);
	}
	
	
	if (checkFileExistance(imageName) == 1) 
	{
		fprintf(stderr," %s already exists, exiting\n", imageName);
		exit(1);
	}
	
	initImageHeader(pD88Header);
	
	createImage(imageName, pD88Header);
	
	return 0;
}

