/*
 * epsp.h version 1.2 part of the vfloppy 1.2 package
 *
 * Copyright 1996 Justin Mitchell (madmitch@discordia.org.uk) and friends.
 *
 * This version delivered in 2002 by Fred Jan Kraan (fjkraan@xs4all.nl)
 *
 * vfread is placed under the GNU General Public License in July 2002.
 *
 *  This file is part of Vfloppy 1.2.
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
 * epsp.h - Epson serial protocol implementation
 *
 */
 
/* command message */
struct epsp
{
	unsigned char head;
	unsigned char fmt;
	unsigned char did;
	unsigned char sid;
	unsigned char fnc;
	unsigned char siz;
};

/* data message */
struct epsp_msg
{
	struct epsp epsp __attribute__(());
	char data[256] __attribute__(());
};

char *label[256];

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
