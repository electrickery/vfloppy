/*
 * Epspd.c version 2.2 part of the Epspd88 2.2 package
 *
 * Copyright 1996 Justin Mitchell (madmitch@discordia.org.uk) and friends as Vfloppy.
 *
 * This version delivered in 2010 by Fred Jan Kraan (fjkraan@xs4all.nl)
 *
 * Epspd is placed under the GNU General Public License in June 2009.
 *
 *  This file is part of Epspd88 2.2.
 *
 *  Epspd is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Epspd is distributed in the hope that it will be useful,
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
 * epspd - emulates up to four virtual disks for the Epson PX-4 and PX-8
 *         computers using the epsp protocol. 
 * epspd -s <port> [-0 <img0>] [-1 <img1>] [-2 <img2>] [-3 <img3>] [-d <debuglevel>]
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include "epspv3.h"
#include "fdc.h"
#include "epspHeaders.h"
#include "logger.h"

int epsp_port;

//int drive_fd[4];
FILE images[4];
//int units[2];

char *label[LABEL_SIZE];
char *deviceNames[DEVICE_NAME_SIZE];

struct driveParameters driveParam;
struct driveInfo driveInfo;

void initDpData() {
	int i;
	for (i=0; i < BLOCK_SIZE; i++)
		driveParam.data[i] = 0;
}
void initLabel() {
	char *empty = "";
	int i;
	for (i=0; i < LABEL_SIZE; i++) {
		label[i] = empty;
	}
	label[STANDARD_WRITE]   = "Standard";   	/* 0x00 */
	label[FLUSH_WRITE]      = "Flush";              /* 0x01 */
	label[SEQUENTIAL_WRITE] = "Sequential"; 	/* 0x02 */
	label[SID_MAPLE]    = "PX-8 (22h)";		/* 0x22 */
	label[SID_PINE]     = "PX-4 (23h)";		/* 0x23 */
	label[DID_DE]       = "D/E (31h)";		/* 0x31 */
	label[DID_FG]       = "F/G (32h)";		/* 0x32 */
	label[FDC_RESET]    = "PINE_RESET (0Dh)";	/* 0x0D */
	label[FDC_RESET_M]  = "MAPLE_RESET (0Eh)";	/* 0X0E */
	label[FDC_READ]     = "FDC_READ (77h)";		/* 0x77 */
	label[FDC_WRITE]    = "FDC_WRITE (78h)";	/* 0x78 */
	label[FDC_WRITEHST] = "FDC_WRITEHST (79h)";	/* 0x79 */
	label[FDC_COPY]     = "FDC_COPY (7Ah)";		/* 0x7A */
	label[FDC_FORMAT]   = "FDC_FORMAT (7Ch)";	/* 0x7C */
}

void initDeviceNames() {
	char *empty = "";
	int i;
	for (i = 0; i < DEVICE_NAME_SIZE; i++) {
		deviceNames[i] = empty;
	}
}

void setSerial(char *s_device) {
	struct termios t; 
        if ( s_device[0] != '\0' ) {
                msg(LOG_DEBUG, "\nSerial port %s specified.\n", s_device);
                epsp_port = open(s_device, O_RDWR);
                if(epsp_port == -1)
                {
			msg(LOG_ERROR," setSerial; Error opening serial port '%s'. Exiting...\n", s_device);
                        perror(s_device);
                        exit(2);
                } else {
                        msg(LOG_DEBUG, " setSerial; Serial port \'%s\' (fd: %d) opened succesfully.\n", 
                                s_device, epsp_port);
			deviceNames[epsp_port] = s_device;
                }
        } else {
                msg(LOG_DEBUG, "No serial port specified. Exiting...\n");
                exit(1);
        }

        if(tcgetattr(epsp_port, &t)==-1) {
                perror("tcgetattr");
                msg(LOG_ERROR," setSerial; Some error with the serial port '%s'.\n", s_device);
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

int mountDiskImage(char *dImage, int driveId, char *unitName) {
	int fd = open(dImage, O_RDWR);
	if (fd == -1) {
		msg(LOG_ERROR, "Some error with opening %s. Exiting...\n", dImage);
		perror(dImage);
		exit(2);
	}
	int diskNo = driveId & 1;	/* Hack */
        if (!checkImageHeader(fd)) {
            msg(LOG_ERROR, "Image %s is not a D88 file. Not mounted\n", dImage);
            return 0;
        }
//        deviceNames[driveId] = dImage;
        if (!createImageIndex(fd, driveId)) {
            msg(LOG_WARN, "WARN: image %s indexing failed. Not mounted\n");
            close(fd);
            return 0;
        }
        driveInfo.drive_fd[driveId] = fd;
	deviceNames[fd] = dImage;
	msg(LOG_INFO, "Unit: %s, Disk: %d, image file: \'%s\', (fd: %d) mounted ok.\n", unitName, diskNo, dImage, fd);
        return 1;
}

void session() {
    unsigned int sessionCount = 0;
        while(1) {
		msg(LOG_DEBUG, "--%06d------------------------------------------------------------------------\n",
                        sessionCount++);
		if (waitForEOT(epsp_port)) {
			msg(LOG_DEBUG, " session; EOT received at start cycle, expecting address message\n");
		} else {
			printf("WARN: received char not EOT, restarting\n");
			continue;
		} 
		if (recENQBlock(epsp_port)) {
			msg(LOG_DEBUG, " session; ENQ block ok, sending ACK, expecting command message\n");
			sendACK(epsp_port);
		} else {
			msg(LOG_DEBUG, "WARN: ENQ block not ok, sending NAK\n");
			sendNAK(epsp_port);
			continue;
		}
		if (recHeaderBlock(epsp_port)) {
			msg(LOG_DEBUG, " session; Header block ok, sending ACK, expecting data message\n");
			sendACK(epsp_port);
		} else {
			msg(LOG_WARN, "WARN: Header block not ok, sending NAK\n");
			sendNAK(epsp_port);
			continue;
		}	
		if (recTextBlock(epsp_port)) {
			msg(LOG_DEBUG, " session; Text block ok, sending ACK, expecting EOT\n");
			sendACK(epsp_port);
		} else {
			msg(LOG_WARN, "WARN: error in Text block size, sending NAK\n");
			sendNAK(epsp_port);
			continue;
		}
		if (waitForEOT(epsp_port)) {
			msg(LOG_DEBUG, " session; EOT received, to send mode\n");
		} else {
			msg(LOG_WARN, "WARN: received char not EOT, restarting\n");
			continue;
		} 
		sendHeaderBlock(epsp_port);
		msg(LOG_DEBUG, " session; Header Block sent\n");
		if (waitForACK(epsp_port)) {
			msg(LOG_DEBUG, " session; ACK received, sending Text Block\n");
		} else {
			continue;
		}
		sendTextBlock(epsp_port);
		msg(LOG_DEBUG, " session; Text Block sent\n");
		if (waitForACK(epsp_port)) {
			msg(LOG_DEBUG, " session; ACK received, session completed, sending EOT\n");
		} else {
			continue;
		}
		sendEOT(epsp_port);
		msg(LOG_DEBUG, " session; EOT sent, waiting for EOT\n");
		if (waitForEOT(epsp_port)) 
			msg(LOG_DEBUG, " session; EOT received, at end cycle\n");
	}	
}

int main(int argc, char *argv[]) {
	char ch;
	char *optstr = "d:0:1:2:3:s:";
        int debug;
        int mountCount = 0;

	initLabel();
	initDeviceNames();
	initDpData();
        driveInfo.units[0] = 0;
        driveInfo.units[1] = 0;
        driveInfo.drive_fd[0] = -1;
        driveInfo.drive_fd[1] = -1;
        driveInfo.drive_fd[2] = -1;
        driveInfo.drive_fd[3] = -1;
        char *version = "version 2.4beta (2022-06-11)";

        msg(LOG_WARN, "\nEPSPD %s\n", version);

	while( -1 != (ch=getopt(argc,argv,optstr))) {
		switch(ch) {
			case '0':           
			/*	printf("option: -0 : %s\n", optarg); */
				driveInfo.units[0] = 1;
				if (mountDiskImage(optarg, 0, label[DID_DE]))
                                        mountCount++;
				break;
			case '1':           
			/*	printf("option: -1 : %s\n", optarg); */
				driveInfo.units[0] = 1;
				if (mountDiskImage(optarg, 1, label[DID_DE]))
                                        mountCount++;
				break;
			case '2':           
			/*	printf("option: -2 : %s\n", optarg); */
				driveInfo.units[1] = 1;
				if (mountDiskImage(optarg, 2, label[DID_FG]))
                                        mountCount++;
				break;
			case '3':           
			/*	printf("option: -3 : %s\n", optarg); */
				driveInfo.units[1] = 1;
				if (mountDiskImage(optarg, 3, label[DID_FG]))
                                        mountCount++;
				break;
			case 's':           
			/*	printf("option: -s : %s\n", optarg); */
				setSerial(optarg);
				break;
			case 'd':
				debug = atoi(optarg);
                                setLogLevel(debug);
				msg(LOG_DEBUG, " main; set debug level to %d\n", getLogLevel());
				break;
                    case 'h':
                    case 'v':
                        usage(version);
                        exit(0);
			case '?':
				msg(LOG_ERROR, "ERROR: unrecognized option: %c\n",optopt);
                                usage(version);
                                exit(1);
				break;
			default:
				msg(LOG_ERROR, "ERROR:  condition unaccounted for?\n");
                                usage(version);
                                exit(1);
				break;

		}
	}
        if (epsp_port == 0) {
            msg(LOG_ERROR, "ERROR: serial port not defined\n");
            usage(version);
            exit(1);
        }
        if (mountCount ==0) {
            msg(LOG_ERROR, "ERROR: no images mounted\n");
            usage(version);
            exit(1);
        }
            
	
	/*
	 *	Up.. time to go; 'Clunk, waitForIt..., 'Clunk', 'Clunk'
	 */
        
        int i;
        setLogLevel(debug);
        
        msg(LOG_DEBUG, " main; Used devices:\n");
        for (i = 0; i < DEVICE_NAME_SIZE; i++) {
            msg(LOG_DEBUG, "       %d: %s\n", i, deviceNames[i]);
        }
	 
	msg(LOG_INFO, "\nEPSPD: Starting EPSP disk services on %s\n", deviceNames[epsp_port]);

	session();
	return(0);
}

void usage(char *version) {
    printf("\nEPSPD %s\n", version);
    printf("usage:\n");
    printf("  -s <serial port\n");
    printf("  -0 <imageName> for drive D:\n");
    printf("  -1 <imageName> for drive E:\n");
    printf("  -2 <imageName> for drive F:\n");
    printf("  -3 <imageName> for drive G:\n");
    printf("  -d <logLevel>\n");
    printf("  -h  this message\n");
    printf("  -v  this message\n");
}
