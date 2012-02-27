/*
 * File:    modem-control.h
 * Author:  wupeng <wp.4163196@gmail.com>
 * Brief:   modem-control interface.
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

#ifndef MODEM_CONTROL_H_
#define MODEM_CONTROL_H_
#include <telephony/ril.h> 
#include "modem-types.h"

typedef void (*UNSOL_LISTENER_FUNC) (int unsol_id, const void* data, size_t datalen, void* listener_ctx);

int modem_init(int argc, char** argv);
void modem_set_unsol_listener(int unsol_id, UNSOL_LISTENER_FUNC listener, void* ctx);
void modem_unset_unsol_listener(int unsol_id);

RIL_Errno modem_dial(const char* number);
RIL_Errno modem_get_current_calls(ModemCall* p_call, size_t* call_num);
RIL_Errno modem_get_sim_status(SimStatus* status);
RIL_Errno modem_get_pin_remain_times(PinRemain* remain);
RIL_Errno modem_change_sim_pin(const char* pin[2]);
RIL_Errno modem_hangup(int index);
RIL_Errno modem_get_imsi(char* imsi, size_t* len);
RIL_Errno modem_get_imei(char* imei, size_t* len);
RIL_Errno modem_get_baseband_version(char* baseband, size_t* len);
RIL_Errno modem_set_radio_power(int on);
RIL_Errno modem_get_registration_state(char** register_state);
RIL_Errno modem_send_sms(const char* smsc, const char* pdu);
RIL_Errno modem_send_cdma_sms(const char* dest, const char* bearer_data);
RIL_RadioState modem_get_radio_state();
RIL_Errno modem_reset();

void modem_destroy();

#endif
