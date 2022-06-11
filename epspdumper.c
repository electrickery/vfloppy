/*
 * epspdumper.c version 1.3 part of the vfloppy 1.4 package
 *
 * Copyright 1996 Justin Mitchell (madmitch@discordia.org.uk) and friends.
 *
 * This version delivered in 2002 by Fred Jan Kraan (fjkraan@xs4all.nl)
 *
 * epspdumper is placed under the GNU General Public License in July 2002.
 *
 *  This file is part of Vfloppy 1.5.
 *
 *  Vfloppy is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Vfloppy is distributed in the hope that it will be useful,
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
 * epspdumper - passive epsp data viewer. To be used as monitor on the serial
 * line between the PX-4 or PX-8 and the Terminal Disk Drive (TF-20, PF-10, 
 * epspdv3, ...) Currently no attempt is made to structure the data into 
 * 
 * meaningful information.
 * epspd -s <port> 
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include "epspv2.h"
#include "fdc.h"


int epsp_port;
int drive_fd[4];
int debug = 3;
int unit0 = 0;
int unit1 = 0;
int dataSize = 0;

char dataMsg[256];
char fntMode = 0;	/* 0 = comp sending, 1 = vFloppy sending */
char myID, otherID;		/* DID, SID data */
char fdcCmd;
char selectedUnit = 0x00;


/* Actually serial port read */
int blk_read(int fd, char *blk, int len)
{
	int sofar  = 0;
	int syncup = 1;
	
	while(sofar < len)
	{
		int got = read(fd, blk, 1);
		if(got <= 0)
		{
			if(got < 0)
			{
				perror("epspd: read");
				exit(1);
			}
			
			if(len == 0)
			{
				fprintf(stderr, "epspd: Lost connection.\n");
				exit(1);
			}
		
		}
		if (debug > 2) {
			int i;
			for (i = 0; i < got; i++) 
				printf("%02X ", (int)blk[i] & 0xFF);
			
		}

		fflush(stdout);
		sofar  += got;
		blk    += got;
		syncup = 0;
	}
	if (debug > 2)
		printf("\n");

	return sofar;
}

void setSerial(char *s_device) {
	struct termios t; 
        if ( s_device[0] != '\0' ) {
                printf("\nSerial port %s specified", s_device);
                epsp_port=open(s_device, O_RDWR);
                if(epsp_port==-1)
                {
                        printf("\n setSerial; Error opening serial port %s. Exiting...\n", s_device);
                        perror(s_device);
                        exit(2);
                } else {
                        printf("\n setSerial; Serial port \'%s\' (fd: %d) opened succesfully.", s_device, epsp_port);
                        label[epsp_port] = s_device;
                }
        } else {
                printf("\nNo serial port specified. Exiting...\n");
                exit(1);
        }

        if(tcgetattr(epsp_port, &t)==-1) {
                perror("tcgetattr");
                printf("\n setSerial; Some error with the serial port\n");
                exit(2);
        }

        cfsetispeed(&t, B38400);
        cfsetospeed(&t, B38400);
        t.c_iflag=t.c_iflag&~(ISTRIP|INLCR|ICRNL|IGNCR|IUCLC|IXON|IXANY|IXOFF);
        t.c_oflag=t.c_oflag&~(OPOST);
        t.c_cflag=t.c_cflag&~(CSIZE|PARENB);
        t.c_cflag|=CS8;
        t.c_lflag=t.c_lflag&~(ISIG|ICANON|ECHO);
        t.c_cc[VTIME]=0;
        t.c_cc[VMIN]=1;

        if(tcsetattr(epsp_port, TCSANOW, &t)==-1) {
                perror("tcsetattr");
                exit(2);
        }
}

int main(int argc, char *argv[]) {
        char *s_device = "\0";
	char ch;
	char *optstr = "d:0:1:2:3:s:";

	while( -1 != (ch=getopt(argc,argv,optstr))) {
		switch(ch) {
			case 's':           
				s_device = optarg;
				printf("option: -s : %s\n", s_device); 
				break;
			case 'd':
				debug = atoi(optarg);
				printf("option: -d : %d\n", debug); 
				break;
			case '?':
				printf("unrecognized option: %c\n",optopt);
				break;
			default:
				printf("error?  condition unaccounted for?\n");
				break;

		}
	}

	if (debug > 1) {
		printf("\nEPSPD version 1.5 (2009-0m-dd)");
		printf("\nDebug level: %d", debug);
	}
	setSerial(s_device);
	
	/*
	 *	Up.. time to go (Clunk, ..(wait), Clunk, Clunk)
	 */
	 
	printf("\nEPSPD: Starting EPSP disk services on %s", s_device);
	printf("\n");
	
	while(1) {
		blk_read(epsp_port, dataMsg, 256);
	}
	
	return(1);
}
