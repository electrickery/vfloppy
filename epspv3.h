/*
 * epsp.h version 2.1 part of the Epspd88 2.1 package
 *
 * Copyright 1996 Justin Mitchell (madmitch@discordia.org.uk) and friends as VFloppy.
 *
 * This version delivered in 2010 by Fred Jan Kraan (fjkraan@xs4all.nl)
 *
 * Epspd88 is placed under the GNU General Public License in July 2002.
 *
 *  This file is part of Epspd88 2.1.
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
 * epsp.h - Epson serial protocol implementation
 *
 */
 
/* command message */
struct epsp {
	unsigned char head;
	unsigned char fmt;
	unsigned char did;
	unsigned char sid;
	unsigned char fnc;
	unsigned char siz;
};

/* data message */
struct epsp_msg {
	struct epsp epsp __attribute__(());
	char data[256] __attribute__(());
};

#define BLOCK_SIZE			0x80

/* typedef struct { ... } driveParam; */
struct driveParameters {
	int command;
	int unit;
	int drive;
	int track;
	int sector;
	int writetype;
/* move items below */
	int imageFileDesc;
	int dataSize;
	unsigned char data[BLOCK_SIZE];
	char returnCode;
};

#define DRIVES 4
#define UNITS 2
struct driveInfo {
        int drive_fd[DRIVES];
        FILE images[DRIVES];
        int units[UNITS];
//        char *driveName[DRIVES];
};

struct behaviour {
	int  (*recTextBlockFunc)(unsigned char*);
	int  (*composeSendTextBlock)(unsigned char*);
	int  (*recHeaderBlockProcessingFunc)();
	void (*sendNAKFunc)();
	void (*sendACKFunc)();
	void (*sendEOTFunc)();
	void (*sendHeaderBlockFunc)();
	void (*sendTextBlockFunc)();
	char (*getTextBlockSize)();
};

#define LABEL_SIZE	256
#define DEVICE_NAME_SIZE	10

#define FMT_FROM_COMP	0x00
#define FMT_FROM_PINE	0x00
#define FMT_FROM_FDD	0x01

/* disk stations, two drives each */
#define DID_DE		0x31
#define DID_FG		0x32
#define DID_HX		0xE1
/* disk station select command */
#define DS_SEL		0x05

/* machine IDs, Maple is PX-8, Pine is PX-4 */
#define SID_HX20	0x20
#define SID_MAPLE	0x22
#define SID_PINE	0x23

#define NUL		0x00
#define SOH		0x01
#define STX		0x02
#define ETX		0x03
#define EOT		0x04
#define ENQ		0x05
#define ACK		0x06
#define NAK		0x15
#define US		0x31

#define PS		0x31 /* Selection mode */

#define NTERM		0x00 /* Normal termination       */
#define BDOS_RDERR	0xFA /* BDOS read error          */
#define BDOS_WRERR	0xFB /* BDOS write error         */
#define BDOS_DSERR	0xFC /* BDOS drive select error  */
#define BDOS_WPERR	0xFD /* BDOS write protect error */

#define ENC_BLOCK_SIZE			0x04
#define HEADER_BLK_SIZE			0x07

#define RECV_MSG_SIZE_OFFSET		0x04

/* command related block sizes */
#define RESET_DATA_SEND_SIZE		0x00
#define RESET_DATA_SEND_MSG_SIZE	0x04

#define READ_DATA_SEND_SIZE		0x80
#define READ_DATA_SEND_MSG_SIZE		0x84

#define WRITE_DATA_SEND_SIZE		0x00
#define WRITE_DATA_SEND_MSG_SIZE	0x04

#define WRITEHST_DATA_SEND_SIZE		0x00
#define WRITEHST_DATA_SEND_MSG_SIZE	0x04

#define COPY_DATA_SEND_SIZE		0x03 /* ?? */
#define COPY_DATA_SEND_MSG_SIZE		0x06 /* ?? */

#define FORMAT_DATA_SEND_SIZE		0x02 /* ?? */
#define FORMAT_DATA_SEND_MSG_SIZE	0x06 /* ?? */

/* generic block sizes */
#define RETURN_CODE_TEXT_BLOCK_SIZE	0x00
#define RETURN_CODE_TEXT_BLOCK_SEND_SIZE 0x04

#define SEND_BUFFER_SIZE		0x100
#define TRACK_ADJUST			0x04


// CP/M Dimensions
#define RECORD   128
#define BLOCK    2048
#define EXTEND   32
#define HEADS    2
#define DIRTRACK 4

// Disk related
#define SECTOR          0x100
#define TRACKS          0x28
#define BLOCKS_ON_DISK  0x8b
#define SECTORSPERBLOCK 0x08
#define SECTORSPERTRACK 0x10
#define DIRECTORYEXTENDS 0x40

//D88 image map related
#define SECTORHEADER 0x10
#define SECTORBASE   0x02B0
#define BLOCKBASE    0x8AB0
#define TRACK        (SECTORHEADER + SECTOR) * SECTORSPERTRACK
