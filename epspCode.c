/*
 * epspCode.c version 2.2 part of the Epspd88 2.2 package
 *
 * Copyright 2010-2013 Fred Jan Kraan
 *
 * This version delivered in 2010 by Fred Jan Kraan (fjkraan@xs4all.nl)
 *
 * EpspCode is placed under the GNU General Public License in June 2009.
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
 * epspCode - epsp functions for epspd.c.
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "epspv3.h"
#include "fdc.h"
#include "epspHeaders.h"
#include "logger.h"

//extern int epsp_port;
int commandForMe = 0;

unsigned char dataMsg[256];
char fntMode = 0;       /* 0 = comp sending, 1 = vFloppy sending */
char myID, otherID;             /* DID, SID data */

extern struct driveParameters driveParam;
extern struct driveInfo driveInfo;
struct behaviour protocolHandler;

int trackCount = 1;

extern char *deviceNames[DEVICE_NAME_SIZE];
extern char *label[LABEL_SIZE];

unsigned char sumChars(unsigned char *msg, int msgSize) {
        int i, checksum = 0;
        for (i = 0; i < msgSize; i++) {
                checksum += msg[i];
        }
        checksum = 0x100 - checksum;
        checksum &= 0xFF;
        return checksum;
}

int composeSendReturnCodeTextBlock(unsigned char *dataMsg) {
	msg(LOG_TRACE, "  composeSendReturnCodeTextBlock; return code: %02X\n", driveParam.returnCode & 0xFF);
        int dataMsgSize = RETURN_CODE_TEXT_BLOCK_SEND_SIZE;

        dataMsg[0] = STX;
        dataMsg[1] = driveParam.returnCode;
        dataMsg[2] = ETX;
        dataMsg[3] = 0;
        dataMsg[3] = sumChars(dataMsg, dataMsgSize);
        return(dataMsgSize);
}

int composeSendResetTextBlock(unsigned char *dataMsg) {
	msg(LOG_TRACE, "  composeSendResetTextBlock; return code: %02X\n", driveParam.returnCode & 0xFF);
        int dataMsgSize = RESET_DATA_SEND_MSG_SIZE;

        dataMsg[0] = STX;
        dataMsg[1] = driveParam.returnCode;
        dataMsg[2] = ETX;
        dataMsg[3] = 0;
        dataMsg[3] = sumChars(dataMsg, dataMsgSize);
        return(dataMsgSize);
}

int composeSendReadTextBlock(unsigned char *dataMsg) {
        int dataMsgSize = READ_DATA_SEND_MSG_SIZE;
	int result = readBlock(&dataMsg[1]); 
	char cmdReturnCode = (result) ? NTERM : BDOS_RDERR;
        dataMsg[0] = STX;               /* Text block header   */
        dataMsg[dataMsgSize - 3] = cmdReturnCode;   /* Command return code */
        dataMsg[dataMsgSize - 2] = ETX; /* Text block end      */
        dataMsg[dataMsgSize - 1] = 0;   /* Text block CKS      */
        dataMsg[dataMsgSize - 1] = sumChars(dataMsg, dataMsgSize);
	msg(LOG_TRACE, "  composeSendReadTextBlock; return code: %02X\n", driveParam.returnCode & 0xFF);
        return(dataMsgSize);
}

int composeSendWriteTextBlock(unsigned char *dataMsg) {
	int result = writeBlock(&dataMsg[1]);
        int dataMsgSize = WRITE_DATA_SEND_MSG_SIZE;
	char cmdReturnCode = (result) ? NTERM : BDOS_WRERR;
	msg(LOG_TRACE, "  composeSendWriteTextBlock; return code: %02X\n", cmdReturnCode & 0xFF);
        dataMsg[0] = STX;
        dataMsg[1] = cmdReturnCode;
        dataMsg[2] = ETX;
        dataMsg[3] = 0;
        dataMsg[3] = sumChars(dataMsg, dataMsgSize);
        return(dataMsgSize);
}

int composeSendWriteHSTTextBlock(unsigned char *dataMsg) {
        int dataMsgSize = WRITEHST_DATA_SEND_MSG_SIZE;
	msg(LOG_TRACE, "  composeSendWriteHSTTextBlock; return code: %02X\n", 0);

        dataMsg[0] = STX;
        dataMsg[1] = 0;
        dataMsg[2] = ETX;
        dataMsg[3] = 0;
        dataMsg[3] = sumChars(dataMsg, dataMsgSize);
        return(dataMsgSize);
}

int composeSendTrackNoTextBlock(unsigned char *dataMsg) {
	int dataMsgSize = FORMAT_DATA_SEND_MSG_SIZE;
 	msg(LOG_TRACE, "  composeSendTrackNoTextBlock; track count: %0X, return code: %02X\n", trackCount, 0);

        dataMsg[0] = STX;
        dataMsg[1] = 0; /* trackNo MSB */
        dataMsg[2] = trackCount; /* trackNo LSB */
        dataMsg[3] = 0;
        dataMsg[4] = ETX;
        dataMsg[5] = 0;
        dataMsg[5] = sumChars(dataMsg, dataMsgSize);
        trackCount += 2;
        if (trackCount > 39) trackCount = 0xFFFF;
        return(dataMsgSize);
}

int readBlock(unsigned char *data) {
    int driveId = (driveParam.unit - '1') * 2 + (driveParam.drive - 1);
//    int location = getSectorLocation(driveParam.track, driveParam.sector, driveId);
    int location = getSectorLocation(driveParam.track, driveParam.sector, driveId);
    msg(LOG_DEBUG, "  readBlock; Drive: (%d/%d) %d, location: %05X\n", 
            driveParam.unit, driveParam.drive, driveId, location);
    imageRead(driveParam.imageFileDesc, data, location, BLOCK_SIZE);
    return(1);
}

int writeBlock(unsigned char *data) {
    int driveId = (driveParam.unit - '1') * 2 + (driveParam.drive - 1);
//    int location = getSectorLocation(driveParam.track, driveParam.sector, driveId);
    int location = getSectorLocation(driveParam.track, driveParam.sector, driveId);
    msg(LOG_DEBUG, "  writeBlock; Drive: %d/%d, location: %05X\n", 
            driveParam.unit, driveParam.drive, location);
    int result = imageWrite(driveParam.imageFileDesc, driveParam.data, location, BLOCK_SIZE);
    return(result);
}

/* Actually serial port read */
int epspRead(int fd, unsigned char *blk, int len) {
        int sofar  = 0;

        msg(LOG_TRACE, "   epspRead; fd(%d, %s, [%02X])  ", fd, deviceNames[fd], len); 
        while(sofar < len) {
                int got = read(fd, blk, 1);
                if(got <= 0) {
                        if(got < 0) {
                                perror("epspd: read");
                                exit(1);
                        }

                        if(len == 0) {
                                fprintf(stderr, "epspd: Lost connection.\n");
                                exit(1);
                        }

                }
                if (getLogLevel() >= LOG_TRACE) {
                        int i;
                        for (i = 0; i < got; i++)
                                msg(LOG_TRACE, "%02X ", (int)blk[i] & 0xFF);

                }

                fflush(stdout);
                sofar  += got;
                blk    += got;
//                syncup = 0;
        }
        if (getLogLevel() >= LOG_TRACE) msg(LOG_TRACE, "\n");

        return sofar;
}

void epspWrite(int fd, unsigned char *message, int messageSize) {
        if (getLogLevel() >= LOG_TRACE) {
                int i;
                msg(LOG_TRACE, "   epspWrite; fd(%d, %s): ", fd, deviceNames[fd]);
                for (i = 0; i < messageSize; i++)
                        msg(LOG_TRACE, "%02X ", (int)message[i] & 0xFF);
                msg(LOG_TRACE, "\n");
        }
        write(fd, message, messageSize);
}

int recENQBlock(int epsp_port) {
        unsigned char addrMsg[ENC_BLOCK_SIZE];

        int len = epspRead(epsp_port, addrMsg, ENC_BLOCK_SIZE);
        if (len == ENC_BLOCK_SIZE) {
                msg(LOG_DEBUG, "  recENQBlock; good: ENQ block size ok\n");
        } else {
                printf("  recENQBlock; ENQ block not expected size\n");
                return(0);
        }

        if (addrMsg[0] == PS) {
                msg(LOG_DEBUG, "  recENQBlock; good: PS received\n");
        } else {
                msg(LOG_WARN, "  recENQBlock; unexpected character: %02X\n", addrMsg[0]);
                return(0);
        }
        msg(LOG_DEBUG, "  recENQBlock; DID is a %s\n", label[(int)addrMsg[1]]);
        msg(LOG_DEBUG, "  recENQBlock; SID is a %s\n", label[(int)addrMsg[2]]);
        if (addrMsg[3] == ENQ) {
                msg(LOG_DEBUG, "  recENQBlock; ENQ present\n");
        } else {
                msg(LOG_WARN, "  recENQBlock; unexpected character: %02X\n", addrMsg[0]);
                return(0);
	}
        if (driveInfo.units[addrMsg[1] - DID_DE]) {
		commandForMe = 1;
		enableInteraction();
        	myID = addrMsg[1];
        	otherID = addrMsg[2];
	} else {
		commandForMe = 0;
		disableInteraction();
	}
        return(1);
}

int recReturnCodeTextBlock(unsigned char *dataMsg) {
	msg(LOG_INFO, "Reset command.\n");
	return(1);
}

int getDriveId() { 
	/* Hack */
	return((driveParam.unit - DID_DE) * 2 + driveParam.drive -1);
}

int recReadTextBlock(unsigned char *dataMsg) {
	msg(LOG_INFO, "Read command. ");
        msg(LOG_INFO, "Unit: %s, Drive: %02X, Track: %02X, Sector: %02X\n",
                label[(int)driveParam.unit], dataMsg[1],  dataMsg[2],  dataMsg[3]);
        driveParam.drive  = dataMsg[1];
        driveParam.track  = dataMsg[2];
        driveParam.sector = dataMsg[3];
	driveParam.imageFileDesc = driveInfo.drive_fd[getDriveId()];
	msg(LOG_DEBUG, "  recReadTextBlock; addressed fd(%d, %s)\n", 
        	driveParam.imageFileDesc, deviceNames[driveParam.imageFileDesc]);
	return(1);
}

int recWriteTextBlock() {
	msg(LOG_INFO, "Write command. ");
        msg(LOG_INFO, "Unit: %s, Drive: %02X, Track: %02X, Sector: %02X Type: %s\n",
               	label[(int)driveParam.unit], dataMsg[1],  dataMsg[2],  dataMsg[3], label[(int)dataMsg[4]]);
        driveParam.drive  = dataMsg[1];
        driveParam.track  = dataMsg[2];
        driveParam.sector = dataMsg[3];
        driveParam.writetype = dataMsg[4];
	driveParam.imageFileDesc = driveInfo.drive_fd[getDriveId()];
	msg(LOG_DEBUG, "  recReadTextBlock; addressed fd(%d, %s)\n", 
		driveParam.imageFileDesc, deviceNames[driveParam.imageFileDesc]);
	memcpy(driveParam.data, &dataMsg[5], BLOCK_SIZE);
	return(1);
}
int recDriveCodeTextBlock() {
	msg(LOG_INFO, "Format/Copy command. ");
	msg(LOG_INFO, "Unit: %s, Drive: %02X\n", label[(int)driveParam.unit], dataMsg[1]);
	driveParam.drive  = dataMsg[1];
	return(1);
}

char getEmptyTextBlockSize() {
	return((char)0);
}

char getReadTextBlockSize() {
	return((char)READ_DATA_SEND_SIZE);
}

char getTrackNoTextBlockSize() {
	return((char)FORMAT_DATA_SEND_SIZE);
}

int recHeaderBlockDataProccessing(unsigned char* cmdMsg) {
	if (!commandForMe) return(1);

	/* Header block data processing */
	driveParam.unit = cmdMsg[2];
	driveParam.command = cmdMsg[4];
        msg(LOG_DEBUG, "  recHeaderBlockDataProccessing; Text block size is %02X\n", cmdMsg[5] & 0xFF);
	driveParam.dataSize = cmdMsg[5] & 0xFF;
	msg(LOG_DEBUG, "  recHeaderBlockDataProccessing; FNC is %s\n", label[(int)driveParam.command]);
        switch (driveParam.command) {
        case FDC_RESET:
        case FDC_RESET_M:
		protocolHandler.recTextBlockFunc = recReturnCodeTextBlock;
		protocolHandler.composeSendTextBlock = composeSendReturnCodeTextBlock;
		protocolHandler.getTextBlockSize = getEmptyTextBlockSize;
		break;
	case FDC_READ:
		protocolHandler.recTextBlockFunc = recReadTextBlock;
		protocolHandler.composeSendTextBlock = composeSendReadTextBlock;  
		protocolHandler.getTextBlockSize = getReadTextBlockSize;
		break;
	case FDC_WRITE:
		protocolHandler.recTextBlockFunc = recWriteTextBlock;
		protocolHandler.composeSendTextBlock = composeSendWriteTextBlock;  
		protocolHandler.getTextBlockSize = getEmptyTextBlockSize;
		break;
	case FDC_WRITEHST:
		protocolHandler.recTextBlockFunc = recReturnCodeTextBlock;
		protocolHandler.composeSendTextBlock = composeSendReturnCodeTextBlock;
		protocolHandler.getTextBlockSize = getEmptyTextBlockSize;
		break;
	case FDC_COPY:
	case FDC_FORMAT:
		protocolHandler.recTextBlockFunc = recDriveCodeTextBlock;
		protocolHandler.composeSendTextBlock = composeSendTrackNoTextBlock;
		protocolHandler.getTextBlockSize = getTrackNoTextBlockSize;
		break;
	default:
		printf("  recHeaderBlockDataProccessing; warning: unknown command received: %02X\n", 
			driveParam.command);
		break;
	}
        return(1);
}

int recHeaderBlockProcessing(unsigned char *cmdMsg, int len) {

	/* Header block protocol checks */
        if (len == HEADER_BLK_SIZE) {
                msg(LOG_DEBUG, "  recHeaderBlock; good: Header block size ok\n");
        } else {
                msg(LOG_WARN, "  recHeaderBlock; warn: Header block size incorrect\n");
                return(0);
        }
        if (cmdMsg[0] == SOH) {
                msg(LOG_DEBUG, "  recHeaderBlock; good: SOH received\n");
        } else {
                msg(LOG_WARN, "  recHeaderBlock; unexpected character: %02X\n", cmdMsg[0]);
                return(0);
        }
	/* contents checks */
        if (cmdMsg[1] == 0x00 || cmdMsg[1] == 0x01) {
                msg(LOG_DEBUG, "  recHeaderBlock; good: FNT = %d\n", cmdMsg[1]);
                fntMode = cmdMsg[1];
        } else {
                msg(LOG_WARN, "  recHeaderBlock; unexpected character: %02X\n", cmdMsg[0]);
                return(0);
        }
        msg(LOG_DEBUG, "  recHeaderBlock; DID is %s\n", label[(int)cmdMsg[2]]);
        if (cmdMsg[2] == otherID) { msg(LOG_DEBUG, "  recHeaderBlock; good: DID matches otherID\n"); }
        msg(LOG_DEBUG, "  recHeaderBlock; SID is a %s\n", label[(int)cmdMsg[3]]);
        if (cmdMsg[3] == myID) { msg(LOG_DEBUG, "  recHeaderBlock; good: SID matches myID\n"); }
        
        int checksum = sumChars(cmdMsg, HEADER_BLK_SIZE);
        if (checksum == 0) {
                msg(LOG_DEBUG, "  recHeaderBlock; good: checksum adds up to 0\n");
        } else {
                msg(LOG_WARN, "  recHeaderBlock; warning: checksum does not add up to 0\n");
                return(0);
        }
	return(recHeaderBlockDataProccessing(cmdMsg));
}

int recHeaderBlock(int epsp_port) {
        unsigned char cmdMsg[HEADER_BLK_SIZE];

        int len = epspRead(epsp_port, cmdMsg, HEADER_BLK_SIZE);

	return(protocolHandler.recHeaderBlockProcessingFunc(cmdMsg, len));
}

int recTextBlock(int epsp_port) {
        int rawDataSize = driveParam.dataSize + RECV_MSG_SIZE_OFFSET;
        int len = epspRead(epsp_port, dataMsg, rawDataSize);

	if (!commandForMe) return(1);

	/* Header block protocol checks */
        if (len == rawDataSize) {
                msg(LOG_DEBUG, "  recTextBlock; good: Text block size ok\n");
        } else {
                msg(LOG_WARN, "  recTextBlock; warn: error in Text block size. not %02X but %02X\n", rawDataSize, len);
                return(0);
        }

        if (dataMsg[0] == STX) {
                msg(LOG_DEBUG, "  recTextBlock; good: STX received at pos: 00\n");
        } else {
                msg(LOG_WARN, "  recTextBlock; warn: unexpected character at STX position: %02X\n", dataMsg[0]);
                return(0);
        }
        if (dataMsg[rawDataSize - 2] == ETX) {
                msg(LOG_DEBUG, "  recTextBlock; good: ETX received at pos: %02X\n", (rawDataSize - 2));
        } else {
                msg(LOG_WARN, "  recTextBlock; warn: unexpected character at ETX position(%d): %02X\n", 
			(rawDataSize - 2), dataMsg[0]);
                return(0);
        }
        int checksum = sumChars(dataMsg, rawDataSize);
        if (checksum == 0) {
                msg(LOG_DEBUG, "  recTextBlock; good: checksum adds up to 0\n");
        } else {
                msg(LOG_WARN, "  recTextBlock; warning: checksum does not add up to 0\n");
                return(0);
        }
	/* Text block data processing */
	protocolHandler.recTextBlockFunc(dataMsg);
        return(1);
}

void sendHeaderBlock(int epsp_port) {
        unsigned char cmdMsg[HEADER_BLK_SIZE];

        cmdMsg[0] = SOH;
        cmdMsg[1] = FMT_FROM_FDD;
        cmdMsg[2] = otherID;
        msg(LOG_DEBUG, "  sendHeaderBlock; DID is %s\n", label[(int)otherID]);
        cmdMsg[3] = myID;
        msg(LOG_DEBUG, "  sendHeaderBlock; SID is %s\n", label[(int)myID]);
        cmdMsg[4] = driveParam.command;
        msg(LOG_DEBUG, "  sendHeaderBlock; FNC is %s\n", label[(int)driveParam.command]);
	driveParam.returnCode = 0; /* setting default */
	/* check for actually mounted image */
	int fd = driveInfo.drive_fd[getDriveId()];
	if (fd == -1) {
		msg(LOG_DEBUG, "  sendHeaderBlock; no image mounted on fd(%d). returning error\n", fd);
		protocolHandler.composeSendTextBlock = composeSendReturnCodeTextBlock;
		protocolHandler.getTextBlockSize = getEmptyTextBlockSize;
		driveParam.returnCode = BDOS_RDERR;
	}
	cmdMsg[5] = protocolHandler.getTextBlockSize();
        cmdMsg[6] = sumChars(cmdMsg, HEADER_BLK_SIZE -1);
        epspWrite(epsp_port, cmdMsg, HEADER_BLK_SIZE);
        msg(LOG_DEBUG, "  sendHeaderBlock; Sent command message\n");
}

void sendTextBlock(int epsp_port) {
        unsigned char dataMsgBuffer[SEND_BUFFER_SIZE];
        int dataMsgSize;
	dataMsgSize = protocolHandler.composeSendTextBlock(dataMsgBuffer);

        epspWrite(epsp_port, dataMsgBuffer, dataMsgSize);
        msg(LOG_DEBUG, " sendTextBlock; Sent data message\n");

}

void enableInteraction() {
	protocolHandler.recHeaderBlockProcessingFunc = recHeaderBlockProcessing;
	protocolHandler.sendNAKFunc = sendNAK;
	protocolHandler.sendACKFunc = sendACK;
	protocolHandler.sendEOTFunc = sendEOT;
	protocolHandler.sendHeaderBlockFunc = sendHeaderBlock;
	protocolHandler.sendTextBlockFunc   = sendTextBlock;
}

void disableInteraction() {
	protocolHandler.recHeaderBlockProcessingFunc = recHeaderBlockProcessing;
	protocolHandler.sendNAKFunc = nullFunction;
	protocolHandler.sendACKFunc = nullFunction;
	protocolHandler.sendEOTFunc = nullFunction;
	protocolHandler.sendHeaderBlockFunc = nullFunction;
	protocolHandler.sendTextBlockFunc   = nullFunction;
}

