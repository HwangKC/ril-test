/*
 * File:    modem-control.c   
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

#include "modem-control.h"
#include <telephony/ril.h>
#include <telephony/ril_cdma_sms.h>
#include <pthread.h>
#include "utils.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <dlfcn.h>
#include <cutils/properties.h>
#include <cutils/sockets.h>
#include <linux/capability.h>
#include <linux/prctl.h>
#include <private/android_filesystem_config.h>

extern void RIL_startEventLoop();

typedef struct _Modem
{
	int already_init;
	RIL_RadioState radio_state;
	RIL_RadioFunctions callbacks;
	pthread_mutex_t unsol_mutex;
	int unsol_listen_id;
	void* listener_ctx;
	void* user_data;
	size_t user_len;
	volatile int response_available;
	RIL_Errno e;
	void (*listener_func)(int unsolResponse, const void* data, size_t datalen, void* listener_ctx);
}Modem;

static Modem s_modem;

static int wait_for_response()
{
	while(!s_modem.response_available) usleep(100);
	s_modem.response_available = 0;

	return 1;
}

static void response_void()
{
	return;
}

static void response_call_list(void* response, size_t response_len)
{
	if(response == NULL || response_len == 0) return;

	ModemCall* call_list = (ModemCall* ) (s_modem.user_data);
	size_t call_num = response_len / sizeof(RIL_Call* );

	size_t i;
	for(i=0; i<call_num; i++)
	{
        /* each call info */
        RIL_Call *p_cur = ((RIL_Call **) response)[i];
		
		call_list[i].state = p_cur->state;
		call_list[i].index = p_cur->index;
		call_list[i].toa = p_cur->toa;
		call_list[i].isMpty = p_cur->isMpty;
		call_list[i].als = p_cur->als;
		call_list[i].isVoice = p_cur->isVoice;
		call_list[i].isVoicePrivacy = p_cur->isVoicePrivacy;

		if(p_cur->name!=NULL)strcpy(call_list[i].name, p_cur->name);
		if(p_cur->number!=NULL)strcpy(call_list[i].number, p_cur->number);

		call_list[i].numberPresentation = p_cur->numberPresentation;
		call_list[i].namePresentation = p_cur->namePresentation;
		//ignore uusInfo
	}

    s_modem.user_len = call_num;

	return;
}

static void response_sim_status(void* response, size_t response_len)
{
	if(response == NULL || response_len == 0) return;

	SimStatus* status = (SimStatus* )(s_modem.user_data);
    RIL_CardStatus* p_cur = (RIL_CardStatus* ) response;
    status->card_state = p_cur->card_state;
    status->universal_pin_state = p_cur->universal_pin_state;
    //ignore other fileds.
    
	return;
}

static void response_strings(void* response, size_t response_len)
{
	if(response == NULL || response_len == 0) return;

	char** strings = (char** )s_modem.user_data;
	size_t num = response_len / sizeof(char* ); 
	size_t i = 0;

	for(i=0; i<num; i++)
	{
		if(((char** )response)[i] != NULL)
		{
			strcpy(strings[i], ((char**)response)[i]);
		}
	}                                                                             

   	s_modem.user_len = num;

   	return;
}

static void response_string(void* response, size_t response_len)
{	
	if(response == NULL || response_len == 0) return;

	char* string = (char* )s_modem.user_data;
	strcpy(s_modem.user_data, response);
   	s_modem.user_len = strlen(string);

   	return;
}

static void response_ints(void* response, size_t response_len)
{
	if(response == NULL || response_len == 0) return;

	int* ints = (int* )s_modem.user_data;
	size_t num_ints = response_len / sizeof(int* );
	memcpy(s_modem.user_data, response, num_ints * sizeof(int));
	s_modem.user_len = num_ints;

	return;
}

static void response_pin_remain_times(void* response, size_t response_len)
{
	if(response == NULL || response_len == 0) return;

	PinRemain* remain = (PinRemain* )s_modem.user_data;
	remain->pin  = ((int*) response)[0];
	remain->pin2 = ((int*) response)[1];
	remain->puk  = ((int*) response)[2];
	remain->puk2 = ((int*) response)[3];
	//ignore user_len

	return;
}

extern void RIL_requestTimedCallback (RIL_TimedCallback callback,
		void *param, const struct timeval *relativeTime);

static void onRequestComplete(RIL_Token t, RIL_Errno e, void *response, size_t response_len)
{
	//consider only one request user once a time.
	//LOGD("%s\n", __func__);
	ASSERT(s_modem.response_available == 0);

	int request = (int )t;

	switch(request)
	{
		case RIL_REQUEST_GET_SIM_STATUS:
			response_sim_status(response, response_len);
			break;

		case RIL_REQUEST_ENTER_SIM_PIN:
		case RIL_REQUEST_ENTER_SIM_PUK:
		case RIL_REQUEST_ENTER_SIM_PIN2:
		case RIL_REQUEST_ENTER_SIM_PUK2:
		case RIL_REQUEST_CHANGE_SIM_PIN:
		case RIL_REQUEST_CHANGE_SIM_PIN2:
		case RIL_REQUEST_ENTER_NETWORK_DEPERSONALIZATION:
			response_ints(response, response_len);
			break;		

		case RIL_REQUEST_GET_CURRENT_CALLS:
			response_call_list(response, response_len);
			break;

		case RIL_REQUEST_DIAL:
		case RIL_REQUEST_HANGUP:
		case RIL_REQUEST_HANGUP_WAITING_OR_BACKGROUND:
		case RIL_REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND:
		case RIL_REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE:
		case RIL_REQUEST_CONFERENCE:
		case RIL_REQUEST_UDUB:
		case RIL_REQUEST_RADIO_POWER:
		case RIL_REQUEST_DTMF:
		case RIL_REQUEST_SEND_USSD:
		case RIL_REQUEST_CANCEL_USSD:
		case RIL_REQUEST_SET_CLIR:
		case RIL_REQUEST_SET_CALL_FORWARD:
		case RIL_REQUEST_SET_CALL_WAITING:
		case RIL_REQUEST_SMS_ACKNOWLEDGE:
		case RIL_REQUEST_ANSWER:
		case RIL_REQUEST_DEACTIVATE_DATA_CALL:
		case RIL_REQUEST_CHANGE_BARRING_PASSWORD:
		case RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC:
		case RIL_REQUEST_SET_NETWORK_SELECTION_MANUAL:
		case RIL_REQUEST_DTMF_START:
		case RIL_REQUEST_DTMF_STOP:
		case RIL_REQUEST_SEPARATE_CONNECTION:
		case RIL_REQUEST_SET_MUTE:
		case RIL_REQUEST_RESET_RADIO:
		case RIL_REQUEST_SCREEN_STATE:
		case RIL_REQUEST_SET_SUPP_SVC_NOTIFICATION:
		case RIL_REQUEST_DELETE_SMS_ON_SIM:
		case RIL_REQUEST_SET_BAND_MODE:
		case RIL_REQUEST_STK_SET_PROFILE:
		case RIL_REQUEST_STK_SEND_TERMINAL_RESPONSE:
		case RIL_REQUEST_STK_HANDLE_CALL_SETUP_REQUESTED_FROM_SIM:
		case RIL_REQUEST_EXPLICIT_CALL_TRANSFER:
		case RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE:
		case RIL_REQUEST_SET_LOCATION_UPDATES:
		case RIL_REQUEST_CDMA_SET_SUBSCRIPTION:
		case RIL_REQUEST_CDMA_SET_ROAMING_PREFERENCE:
		case RIL_REQUEST_SET_TTY_MODE:
		case RIL_REQUEST_CDMA_SET_PREFERRED_VOICE_PRIVACY_MODE:
		case RIL_REQUEST_CDMA_FLASH:
		case RIL_REQUEST_CDMA_BURST_DTMF:
		case RIL_REQUEST_CDMA_VALIDATE_AND_WRITE_AKEY:
		case RIL_REQUEST_CDMA_SMS_ACKNOWLEDGE:
		case RIL_REQUEST_GSM_SET_BROADCAST_SMS_CONFIG:  
		case RIL_REQUEST_GSM_SMS_BROADCAST_ACTIVATION:  
		case RIL_REQUEST_CDMA_SMS_BROADCAST_ACTIVATION:  
		case RIL_REQUEST_CDMA_DELETE_SMS_ON_RUIM:  
		case RIL_REQUEST_EXIT_EMERGENCY_CALLBACK_MODE:  
		case RIL_REQUEST_SET_SMSC_ADDRESS:  
		case RIL_REQUEST_REPORT_SMS_MEMORY_STATUS:  
		case RIL_REQUEST_REPORT_STK_SERVICE_IS_RUNNING:  
			response_void(response, response_len);
			break;

		case RIL_REQUEST_GET_IMSI: 
		case RIL_REQUEST_GET_IMEI: 
		case RIL_REQUEST_GET_IMEISV: 
		case RIL_REQUEST_BASEBAND_VERSION: 
		case RIL_REQUEST_STK_GET_PROFILE: 
		case RIL_REQUEST_STK_SEND_ENVELOPE_COMMAND:
		case RIL_REQUEST_GET_SMSC_ADDRESS:
			response_string(response, response_len);
			break;

		case RIL_REQUEST_GET_SIM_PIN_REMAIN_TIMES:
			response_pin_remain_times(response, response_len);
			break;

		case RIL_REQUEST_REGISTRATION_STATE:
			response_strings(response, response_len);
			break;
		
		default:
			LOGE("unknown RIL_REQUEST_XX!!!");
			break;
	}
	
	s_modem.e = e;
	s_modem.response_available = 1;

	return;
}

static void onUnsolicitedResponse(int unsolResponse, const void *data, size_t datalen)
{
	pthread_mutex_lock(&(s_modem.unsol_mutex));
	switch(unsolResponse)
	{
		case RIL_UNSOL_RESPONSE_RADIO_STATE_CHANGED:
			s_modem.radio_state = s_modem.callbacks.onStateRequest();
			break;
		default:
			break;
	}

	if(s_modem.unsol_listen_id == unsolResponse && s_modem.listener_func != NULL)
	{
		s_modem.listener_func(unsolResponse, data, datalen, s_modem.listener_ctx);
	}

	pthread_mutex_unlock(&(s_modem.unsol_mutex));
}

struct RIL_Env ril_env = {
	onRequestComplete,
	onUnsolicitedResponse,
	RIL_requestTimedCallback
};

static void usage(const char *argv0)
{
	fprintf(stderr, "Usage: %s -l <ril impl library> [-- <args for impl library>]\n", argv0);
}

/*
 *  * switchUser - Switches UID to radio, preserving CAP_NET_ADMIN capabilities.
 *   * Our group, cache, was set by init.
 *    */
static void switchUser() {
	prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0);
	setuid(AID_RADIO);

	struct __user_cap_header_struct header;
	struct __user_cap_data_struct cap;
	header.version = _LINUX_CAPABILITY_VERSION;
	header.pid = 0;
	//bypass file read write permission.
	//cap.effective = cap.permitted = 1 << CAP_NET_ADMIN;
	cap.effective = cap.permitted = (1 << CAP_NET_ADMIN) | (1<< CAP_DAC_OVERRIDE);
	cap.inheritable = 0;
	capset(&header, &cap);
}

int modem_init(int argc, char** argv)
{
	//this func code almost copy from main func of rild.c.
	if(s_modem.already_init) return 0;

	memset(&s_modem, 0, sizeof(Modem));
	switchUser();

	const char* rilLibPath = NULL;
	char **rilArgv;
	void* dlHandle;
	const RIL_RadioFunctions *(*rilInit)(const struct RIL_Env *, int, char **);
	const RIL_RadioFunctions *funcs;
	int hasLibArgs = 0;

	int i;
	for (i = 1; i < argc ;) {
		if (0 == strcmp(argv[i], "-l") && (argc - i > 1)) {
			rilLibPath = argv[i + 1];
			i += 2;
		} else if (0 == strcmp(argv[i], "--")) {
			i++;
			hasLibArgs = 1;
			break;
		} else {
			usage(argv[0]);
			exit(-1);
		}
	}

	dlHandle = dlopen(rilLibPath, RTLD_NOW);
	if (dlHandle == NULL) {
		fprintf(stderr, "dlopen failed: %s\n", dlerror());
		return 0;
	}
	rilInit = (const RIL_RadioFunctions *(*)(const struct RIL_Env *, int, char **))dlsym(dlHandle, "RIL_Init");
	RIL_startEventLoop();

	rilArgv = argv + i - 1;
	argc = argc - i + 1;
	// Make sure there's a reasonable argv[0]
	rilArgv[0] = argv[0];

	funcs = rilInit(&ril_env, argc, rilArgv);
	memcpy(&(s_modem.callbacks), funcs, sizeof(RIL_RadioFunctions));

	pthread_mutex_init(&(s_modem.unsol_mutex), NULL);
	s_modem.unsol_listen_id = -1;
	s_modem.listener_ctx = NULL;
	s_modem.listener_func = NULL;
	s_modem.radio_state = RADIO_STATE_UNAVAILABLE;

	s_modem.already_init = 1;
	//sleep(30);
	
	return 1;
}

void modem_set_unsol_listener(int unsol_id, UNSOL_LISTENER_FUNC listener, void* ctx)
{
	pthread_mutex_lock(&(s_modem.unsol_mutex));
	s_modem.unsol_listen_id = unsol_id;
	s_modem.listener_func = listener;
	s_modem.listener_ctx = ctx;
	pthread_mutex_unlock(&(s_modem.unsol_mutex));
}

void modem_unset_unsol_listener(int unsol_id)
{
	pthread_mutex_lock(&(s_modem.unsol_mutex));
	s_modem.unsol_listen_id = -1;
	s_modem.listener_func = NULL;
	s_modem.listener_ctx = NULL;
	pthread_mutex_unlock(&(s_modem.unsol_mutex));
}

//fill user_data and user_len in onRequestComplete.
static RIL_Errno request_ril(int request, void* data, size_t datalen, void* user_data, size_t* user_len)
{
	s_modem.user_data = user_data;
	s_modem.callbacks.onRequest(request, data, datalen, (RIL_Token) request);

	wait_for_response();
	if(user_len != NULL) *user_len = s_modem.user_len;

	s_modem.user_data = NULL;
	s_modem.user_len = 0;

	return s_modem.e;
}

RIL_Errno modem_dial(const char* number)
{
	return_val_if_fail(number!=NULL, RIL_E_GENERIC_FAILURE);

	RIL_Dial dial;
	memset(&dial, 0, sizeof(RIL_Dial));
	dial.address = number;

	return request_ril(RIL_REQUEST_DIAL, &dial, sizeof(dial), NULL, NULL);
}

RIL_Errno modem_hangup(int index)
{
	return_val_if_fail(index>=0, RIL_E_GENERIC_FAILURE);

	return request_ril(RIL_REQUEST_HANGUP, &index, 1 * sizeof(int* ), NULL, NULL);
}

RIL_Errno modem_get_current_calls(ModemCall* p_call, size_t* call_num)
{
	return_val_if_fail(p_call!=NULL && call_num!=NULL, RIL_E_GENERIC_FAILURE);

	return request_ril(RIL_REQUEST_GET_CURRENT_CALLS, NULL, 0, p_call, call_num);
}

RIL_Errno modem_get_sim_status(SimStatus* status)
{
	return_val_if_fail(status!=NULL, RIL_E_GENERIC_FAILURE);

	return request_ril(RIL_REQUEST_GET_SIM_STATUS, NULL, 0, status, NULL);
}

RIL_Errno modem_get_pin_remain_times(PinRemain* remain)
{
	return_val_if_fail(remain!=NULL, RIL_E_GENERIC_FAILURE);

	return request_ril(RIL_REQUEST_GET_SIM_PIN_REMAIN_TIMES, NULL, 0, remain, NULL);
}

RIL_Errno modem_change_sim_pin(const char* pin[2])
{
	return_val_if_fail(pin!=NULL, RIL_E_GENERIC_FAILURE);

	return request_ril(RIL_REQUEST_CHANGE_SIM_PIN, pin, sizeof(char* ) * 2, NULL, NULL);
}

RIL_Errno modem_get_imsi(char* imsi, size_t* len)
{
	return_val_if_fail(imsi!=NULL, RIL_E_GENERIC_FAILURE);

	return request_ril(RIL_REQUEST_GET_IMSI, NULL, 0, imsi, len);
}

RIL_Errno modem_get_imei(char* imei, size_t* len)
{
	return_val_if_fail(imei!=NULL, RIL_E_GENERIC_FAILURE);

	return request_ril(RIL_REQUEST_GET_IMEI, NULL, 0, imei, len);
}

RIL_Errno modem_get_baseband_version(char* baseband, size_t* len)
{
	return_val_if_fail(baseband!=NULL, RIL_E_GENERIC_FAILURE);

	return request_ril(RIL_REQUEST_BASEBAND_VERSION, NULL, 0, baseband, len);
}

RIL_Errno modem_set_radio_power(int on)
{
	return_val_if_fail(on >= 0, RIL_E_GENERIC_FAILURE);

	return request_ril(RIL_REQUEST_RADIO_POWER, &on, 1 * sizeof(int*), NULL, NULL);
}

//see telephony/ril.h RIL_REQUEST_REGISTRATION_STATE
//responseStrings.
RIL_Errno modem_get_registration_state(char** register_state)
{
	return_val_if_fail(register_state!=NULL, RIL_E_GENERIC_FAILURE);

	return request_ril(RIL_REQUEST_REGISTRATION_STATE, NULL, 0, register_state, NULL);
}

RIL_Errno modem_send_cdma_sms(const char* dest, const char* bearer_data)
{
	return_val_if_fail(dest!=NULL && bearer_data!=NULL, RIL_E_GENERIC_FAILURE);

	RIL_CDMA_SMS_Message message;
	//TODO
	//set message args.
	//bearer data to binary.
	//message
	return request_ril(RIL_REQUEST_CDMA_SEND_SMS, &message, 1 * sizeof(RIL_CDMA_SMS_Message* ), NULL, NULL);
	
}

RIL_Errno modem_send_sms(const char* smsc, const char* pdu)
{
	return_val_if_fail(smsc!=NULL && pdu!=NULL, RIL_E_GENERIC_FAILURE);

	const char* data[2] = {smsc, pdu};
	
	return request_ril(RIL_REQUEST_SEND_SMS, data, 2 * sizeof(char*), NULL, NULL);
}

RIL_RadioState modem_get_radio_state()
{
	return s_modem.radio_state;
}

RIL_Errno modem_reset()
{
	RIL_Errno ret;
	ret = modem_set_radio_power(0);
	return_val_if_fail(ret==RIL_E_SUCCESS, ret);

	return modem_set_radio_power(1);
}

void modem_destroy()
{}

