/*
 * File:    modem-types.h   
 * Author:  wupeng <wp.4163196@gmail.com>
 * Brief:   almost copy from telephony/ril.h, to get rid of no decontruction
 * 			in C.
 *
 * Copyright (c) 2011 - 2012  wupeng <wp.4163196@gmail.com>
 *
 * Licensed under the Academic Free License version 2.1
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * History:
 * ================================================================
 * 2011-10-31 wupeng <wp.4163196@gmail.com> created
 *
 */

#ifndef MODEM_TYPES_H_
#define MODEM_TYPES_H_
#include <telephony/ril.h>
#include "utils.h"

//these typedefs almost copy from ril.h, but may remove/modify some members, as there is no construct/deconstruct function in C, which cause some problems.
typedef struct _ModemCall
{
	RIL_CallState   state;
    int             index;      /* Connection Index for use with, eg, AT+CHLD */
    int             toa;        /* type of address, eg 145 = intl */
    char            isMpty;     /* nonzero if is mpty call */
    char            isMT;       /* nonzero if call is mobile terminated */
    char            als;        /* ALS line indicator if available
                                   (0 = line 1) */
    char            isVoice;    /* nonzero if this is is a voice call */
    char            isVoicePrivacy;     /* nonzero if CDMA voice privacy mode is active */
    char            number[MAX_BUFFER_SIZE];     /* Remote party number */
    int             numberPresentation; /* 0=Allowed, 1=Restricted, 2=Not Specified/Unknown 3=Payphone */
    char            name[MAX_BUFFER_SIZE];       /* Remote party name */
    int             namePresentation; /* 0=Allowed, 1=Restricted, 2=Not Specified/Unknown 3=Payphone */
    //RIL_UUS_Info *  uusInfo;    /* NULL or Pointer to User-User Signaling Information */
}ModemCall;

typedef struct _PinRemain
{
	int pin;
	int pin2;
	int puk;
	int puk2;
}PinRemain;

typedef struct _SimStatus
{
	RIL_CardState card_state;
	RIL_PinState  universal_pin_state;             /* applicable to USIM and CSIM: RIL_PINSTATE_xxx */
}SimStatus;

#endif
