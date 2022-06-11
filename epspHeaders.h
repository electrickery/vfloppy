/*
 * epspHeaders.h version 2.1 part of the Epspd88 2.1 package
 *
 * Copyright 2010 by Fred Jan Kraan (fjkraan@xs4all.nl)
 *
 * Epspd88 is placed under the GNU General Public License in April 2010.
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

/* epspProtocol.c */
int  waitForEOT(int epsp_port);
void sendACK(int epsp_port);
void sendNAK(int epsp_port);
int  waitForACK(int epsp_port);
void sendEOT(int epsp_port);
void nullFunction();
int  nullFunctionOk();

int  recENQBlock(int epsp_port);
int  recHeaderBlock(int epsp_port);
int  recTextBlock(int epsp_port);
void sendHeaderBlock(int epsp_port);
void sendTextBlock(int epsp_port);

void enableInteraction();
void disableInteraction();

void epspWrite(int, unsigned char *, int);
int  epspRead(int, unsigned char *, int);

/* imageCode.c */
unsigned long int getSectorLocation(int track, int record, int driveId);
//unsigned long int getSectorLocation2(int track, int record, int driveId);
int createImageIndex(int fd, int driveId);
unsigned long blockMap(int, int);
int imageRead(int, unsigned char *, unsigned long, int);
int imageWrite(int, unsigned char *, unsigned long, int);
int valueComposer(unsigned char llsb, unsigned char lsb, unsigned char msb, unsigned char mmsb);
int checkImageHeader(int fd);

/* epspCode.c */
int readBlock(unsigned char *data);
int writeBlock(unsigned char *data);

/* epspdv3.c */
void initDpData();
void initLabel();
void initDeviceNames();
void setSerial(char *s_device);
int mountDiskImage(char *dImage, int driveId, char *unitName);
void session();

void usage(char *version);