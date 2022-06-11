/*
 * epspProtocol.c version 2.2 part of the Epspd88 2.2 package
 *
 * Copyright 2009 - 2013 Fred Jan Kraan
 *
 * This version delivered in 2009 by Fred Jan Kraan (fjkraan@xs4all.nl)
 *
 * epspCode is placed under the GNU General Public License in June 2009.
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
 * epspProtocol - epsp protocol functions for epspd.c.
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

extern char *deviceNames[DEVICE_NAME_SIZE];
extern char *label[LABEL_SIZE];

//extern int epsp_port;

//extern struct driveParameters driveParam;
//extern struct behaviour protocolHandler;

void sendNAK(int epsp_port) {
        unsigned char dp=NAK;
        epspWrite(epsp_port, &dp, 1);
        msg(LOG_DEBUG, "  sendNAK; Sent NAK\n");
}

int waitForACK(int epsp_port) {
        unsigned char dp;
        epspRead(epsp_port, &dp, 1);
        if (dp == ACK) {
                msg(LOG_DEBUG, "  waitForACK; good: ACK received\n");
                return(1);
        } else if (dp == NAK) {
                msg(LOG_WARN, "  waitForACK; warn: > > > > > > NAK received < < < < < <\n");
        } else {
                msg(LOG_WARN, "  waitForACK; warning: no ACK but %02X received\n", dp & 0xFF);
        }
        return(0);
}

void sendACK(int epsp_port) {
        unsigned char dp=ACK;
        epspWrite(epsp_port, &dp, 1);
        msg(LOG_DEBUG, "  sendACK; Sent ACK\n");
}

int waitForEOT(int epsp_port) {
        unsigned char dp;
        epspRead(epsp_port, &dp, 1);
        if (dp == EOT) {
                msg(LOG_DEBUG, "  waitForEOT; good: EOT received\n");
                return(1);
        }
        msg(LOG_WARN, "  waitForEOT; warning: no EOT but %02X received\n", dp & 0xFF);
        return(0);
}

void sendEOT(int epsp_port) {
        unsigned char dp = EOT;
        epspWrite(epsp_port, &dp, 1);
        msg(LOG_DEBUG, "  sendEOT; Sent EOT\n");
}

void nullFunction() {
}

int nullFunctionOk() {
	return(1);
}

