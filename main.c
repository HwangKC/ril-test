/*
 * File:    main.c    
 * Author:  wupeng <wp.4163196@gmail.com>
 * Brief:   running the modem test case.
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

#include <stdio.h>
#include <unistd.h>
#include <cutils/properties.h>
#include "modem-control.h"
#include "utils.h"

typedef struct _Context
{
	int unsolResoponse;
	const void* unsol_data;
	size_t unsol_datalen;
	int has_sim;
}Context;

static Context s_context;

static int s_context_init()
{
	memset(&s_context, 0, sizeof(Context));
	
	//get has sim param.
	char sim_status[MAX_BUFFER_SIZE];
	if ( 0 == property_get("test.simstatus", sim_status, NULL)) {
		LOGE("SIM option is needed, execute setprop test.simstatus <0/1>.\n");
		exit(-1);
	}
	                                                                 
	s_context.has_sim = atoi(sim_status);
	LOGD("test context init success!");

	return 1;
}

const char* radio_state_to_string(RIL_RadioState s) 
{
    switch(s) 
    {
        case RADIO_STATE_OFF: return "RADIO_OFF";
        case RADIO_STATE_UNAVAILABLE: return "RADIO_UNAVAILABLE";
        case RADIO_STATE_SIM_NOT_READY: return "RADIO_SIM_NOT_READY";
        case RADIO_STATE_SIM_LOCKED_OR_ABSENT: return "RADIO_SIM_LOCKED_OR_ABSENT";
        case RADIO_STATE_SIM_READY: return "RADIO_SIM_READY";
        case RADIO_STATE_RUIM_NOT_READY:return"RADIO_RUIM_NOT_READY";
        case RADIO_STATE_RUIM_READY:return"RADIO_RUIM_READY";
        case RADIO_STATE_RUIM_LOCKED_OR_ABSENT:return"RADIO_RUIM_LOCKED_OR_ABSENT";
        case RADIO_STATE_NV_NOT_READY:return"RADIO_NV_NOT_READY";
        case RADIO_STATE_NV_READY:return"RADIO_NV_READY";
        default: return "<unknown state>";
    }
}

static void test_sms_listener_func(int unsolResponse, const void* data, size_t datalen, void* ctx)
{
	int* sms_recvd = (int* )ctx;
	if(unsolResponse == RIL_UNSOL_RESPONSE_NEW_SMS) *sms_recvd++;
	return;
}

static int is_radio_on()
{
	RIL_RadioState state;
	state = modem_get_radio_state();

	return	(state == RADIO_STATE_SIM_NOT_READY
			  || state == RADIO_STATE_SIM_LOCKED_OR_ABSENT
			  || state == RADIO_STATE_SIM_READY
			  || state == RADIO_STATE_RUIM_NOT_READY
			  || state == RADIO_STATE_RUIM_READY
			  || state == RADIO_STATE_RUIM_LOCKED_OR_ABSENT
			  || state == RADIO_STATE_NV_NOT_READY
			  || state == RADIO_STATE_NV_READY);
}

static int is_icc_ready()
{
	RIL_RadioState state;
	state = modem_get_radio_state();

	return (state == RADIO_STATE_SIM_READY
			|| state == RADIO_STATE_RUIM_READY);
}

static int is_cdma()
{
	RIL_RadioState state;
	state = modem_get_radio_state();

	return (state == RADIO_STATE_RUIM_NOT_READY
			|| state == RADIO_STATE_RUIM_READY
			|| state == RADIO_STATE_RUIM_LOCKED_OR_ABSENT
			|| state == RADIO_STATE_NV_NOT_READY
			|| state == RADIO_STATE_NV_READY);
}

static int is_gsm()
{
	RIL_RadioState state;
	state = modem_get_radio_state();

	return (state == RADIO_STATE_SIM_NOT_READY
			|| state == RADIO_STATE_SIM_LOCKED_OR_ABSENT
			|| state == RADIO_STATE_SIM_READY);
}


static int is_icc_locked_or_absent()
{
	RIL_RadioState state;
	state = modem_get_radio_state();

	return (state == RADIO_STATE_SIM_LOCKED_OR_ABSENT
			|| state == RADIO_STATE_RUIM_LOCKED_OR_ABSENT);
}

static int is_registration_ready()
{
	char* register_state[50];
	int i;
	for(i=0; i<50; i++)
	{
		register_state[i] = (char*) alloca(MAX_BUFFER_SIZE * sizeof(char));
	}

	RIL_Errno ret = modem_get_registration_state(register_state);

	return (ret==RIL_E_SUCCESS && (atoi(register_state[0])==REGISTER_HOME || atoi(register_state[0])==REGISTER_ROAMING)) ? 1 : 0;
}

static int init_test()
{	
	LOGD("-----%s start-----\n", __func__);
	LOGD("sim context: %d\n", s_context.has_sim);
	RIL_RadioState state;
	while((state = modem_get_radio_state()) == RADIO_STATE_UNAVAILABLE)
	{
		sleep(1);
	}

	modem_set_radio_power(1);
	LOGD("%s: sleep for a while after set radio power 1", __func__);
	sleep(15);

	if(s_context.has_sim)
	{
				
		//if locked or absent, you should unlock it.
		/*if(is_icc_locked_or_absent(state))
		{
			unlock.
			sleep;
			state = modem_get_radio_state();
		}*/

		if(!is_icc_ready() || !is_registration_ready())
		{
			LOGE("%s failed, radio state: %s, registion ready: %d", __func__, 
				radio_state_to_string(modem_get_radio_state()), is_registration_ready());
			return 0;
		}
	}
	else
	{
		if(!is_icc_locked_or_absent())
		{
			LOGE("%s failed, radio state: %s", __func__, radio_state_to_string(modem_get_radio_state()));
			return 0;
		}
	}
	LOGD("-----%s end-----\n", __func__);

	return 1;
}

static int test_sim()
{
	LOGD("-----%s start-----\n", __func__);
	RIL_Errno ret = RIL_E_SUCCESS;
	if(s_context.has_sim)
	{
		ASSERT(is_icc_ready());

		SimStatus sim_status; 
		memset(&sim_status, 0, sizeof(SimStatus));
		ret = modem_get_sim_status(&sim_status);
		ASSERT(ret==RIL_E_SUCCESS && sim_status.card_state==RIL_CARDSTATE_PRESENT);

		PinRemain remain;
		ret = modem_get_pin_remain_times(&remain);
		ASSERT(ret==RIL_E_SUCCESS && remain.pin==3 && remain.pin2==3 && remain.puk==10 && remain.puk2==10);

		//old pin first.
		///*wupeng test.
		const char* pin_pwd1[2] = {"1234", "4321"};
		const char* pin_pwd2[2] = {"4321", "1234"};
		ret = modem_change_sim_pin(pin_pwd1);
		ASSERT(ret == RIL_E_SUCCESS);

		ret = modem_change_sim_pin(pin_pwd2);
		ASSERT(ret == RIL_E_SUCCESS);

		ret = modem_change_sim_pin(pin_pwd2);
		ASSERT(ret == RIL_E_PASSWORD_INCORRECT);
		//*/

		//import sim contacts.
		//import sim sms.
		//set sim facility lock, reset modem, check the state, it should be enter-pin state.
	}
	else
	{
		ASSERT(is_icc_locked_or_absent());
	}
	LOGD("-----%s end-----\n", __func__);

	return 1;
}

static int test_cc()
{

	LOGD("-----%s start-----\n", __func__);
	if(!s_context.has_sim) 
	{
		LOGD("no sim, %s ignored", __func__);
		LOGD("-----%s end-----\n", __func__);
		return 1;
	}

	if(!is_icc_ready() || !is_registration_ready())
	{
		LOGE("%s warning: icc not ready, reset modem", __func__);
		exit_if_fail(modem_reset() == RIL_E_SUCCESS);
		exit_if_fail(init_test() == 1); 
	}

	ModemCall call_list[14];
	memset(call_list, 0, sizeof(call_list));
	
	int i;
	for(i=0; i<28; i++)
	{
		ModemCall* call_cur = NULL;
		RIL_Errno ret = RIL_E_SUCCESS;
		size_t call_num = 0;
		const char* unicom = "10000";
		const char* emergency = "112";
		const char* number_cur = NULL;
		
		if(i<14) number_cur = unicom;
		else number_cur = emergency;

		ASSERT(modem_dial(number_cur) == RIL_E_SUCCESS);
		ret = modem_get_current_calls(call_list, &call_num);
		ASSERT(ret == RIL_E_SUCCESS && call_num == 1);
		call_cur = &call_list[0];

		ASSERT(call_cur->state == RIL_CALL_ALERTING);
		ASSERT(strncmp(call_cur->number, number_cur, strlen(number_cur)) == 0);
		call_num = 0;

		LOGD("cur number %s has been dailed, wait for connect.", number_cur);	
		sleep(5);
		int j = 0;
		for(j; j<10; j++)
		{
			ret = modem_get_current_calls(call_list, &call_num);
			ASSERT(ret == RIL_E_SUCCESS && call_num == 1);
			call_cur = &call_list[0];

			ASSERT(call_cur->state == RIL_CALL_ACTIVE);
			ASSERT(strncmp(call_cur->number, number_cur, strlen(number_cur)) == 0);

			sleep(1);
		}

		ret = modem_hangup(call_cur->index);
		ASSERT(ret == RIL_E_SUCCESS);

		ret = modem_get_current_calls(call_list, &call_num);
		ASSERT(ret == RIL_E_SUCCESS && call_num == 0);
	}

	LOGD("-----%s end-----\n", __func__);

	//TODO remote dial, and receive RING strings.
	//TODO check the state when test end.

	return 1;
}



static int test_gsm_sms()
{
	LOGD("-----%s start-----\n", __func__);
	//TODO support completely.
	return 1;

	/*test common sms.*/
	int i = 0;
	int sms_recvd = 0;
	for(i=0; i<10; i++)
	{
		//TODO construct the pdu send sms to myself, short sms, ems
		const char* pdu = "0891683108200105F011000B813119169083F80000A806C9363C3CA603";
		const char* smsc = NULL;

		modem_set_unsol_listener(RIL_UNSOL_RESPONSE_NEW_SMS, test_sms_listener_func, &sms_recvd);

		ASSERT(modem_send_sms(smsc, pdu) == RIL_E_SUCCESS);
		
		sleep(10);
		ASSERT(sms_recvd >= i+1);
		
		modem_unset_unsol_listener(RIL_UNSOL_RESPONSE_NEW_SMS);
	}
	
	/*test invalid sms.*/
	for(i=0; i<10; i++)
	{
		//TODO construct the pdu send sms to myself, short sms, ems
		const char* pdu0 = "sdfsaf";
		const char* smsc0 = "sdfsfs";

		ASSERT(modem_send_sms(smsc0, pdu0) != RIL_E_SUCCESS);
	}

	/*test concatenated sms.*/
	for(i=0; i<3; i++)
	{
		sms_recvd = 0;
		modem_set_unsol_listener(RIL_UNSOL_RESPONSE_NEW_SMS, test_sms_listener_func, &sms_recvd);

		int j = 0;
		for(j=0; j<5; j++)
		{
			const char* pdu1 = "dsafdsafsfsff"; 
			const char* smsc1 = "dasfsdfsfsdfsdfs";
			ASSERT(modem_send_sms(smsc1, pdu1) == RIL_E_SUCCESS);
		}

		sleep(30);
		ASSERT(sms_recvd >= 5);

		modem_unset_unsol_listener(RIL_UNSOL_RESPONSE_NEW_SMS);
	}

	LOGD("-----%s end-----\n", __func__);
	
	//TODO maybe I should check the radio state here.
	//TODO read SIM-SMS 
	return 1;
}

static int test_cdma_sms()
{
	LOGD("-----%s start-----\n", __func__);
	//TODO support completely.
	return 1;

	/*test common sms*/
	int i = 0;
	int sms_recvd = 0;
	for(i=0; i<10; i++)
	{
		//TODO construct the pdu send sms to myself, short sms, ems
		const char* dest_addr = "18665358601";
		const char* bearer_data = "";
		modem_set_unsol_listener(RIL_UNSOL_RESPONSE_CDMA_NEW_SMS, test_sms_listener_func, &sms_recvd);

		ASSERT(modem_send_cdma_sms(dest_addr, bearer_data) == RIL_E_SUCCESS);
		
		sleep(10);
		ASSERT(sms_recvd >= i+1);
		
		modem_unset_unsol_listener(RIL_UNSOL_RESPONSE_CDMA_NEW_SMS);
	}
	/*test invalid sms.*/
	for(i=0; i<10; i++)
	{
		//TODO construct the pdu send sms to myself, short sms, ems
		const char* pdu0 = "sdfsaf";
		const char* smsc0 = "sdfsfs";

		ASSERT(modem_send_sms(smsc0, pdu0) != RIL_E_SUCCESS);
	}

	/*test concatenated sms.*/
	for(i=0; i<3; i++)
	{
		sms_recvd = 0;
		modem_set_unsol_listener(RIL_UNSOL_RESPONSE_NEW_SMS, test_sms_listener_func, &sms_recvd);

		int j = 0;
		for(j=0; j<5; j++)
		{
			const char* pdu1 = "dsafdsafsfsff"; 
			const char* smsc1 = "dasfsdfsfsdfsdfs";
			ASSERT(modem_send_sms(smsc1, pdu1) == RIL_E_SUCCESS);
		}

		sleep(30);
		ASSERT(sms_recvd >= 5);

		modem_unset_unsol_listener(RIL_UNSOL_RESPONSE_NEW_SMS);
	}

	LOGD("-----%s end-----\n", __func__);
	
	//TODO maybe I should check the radio state here.
	//TODO read SIM-SMS 
	return 1;
}

static int test_sms()
{
	if(!s_context.has_sim) 
	{
		LOGD("no sim, %s ignored", __func__);
		return 1;
	}

	if(!is_icc_ready() || !is_registration_ready())
	{
		LOGD("%s warning: icc not ready, reset modem", __func__);

		exit_if_fail(modem_reset() == RIL_E_SUCCESS);
		exit_if_fail(init_test() == 1); 
	}

	if(is_gsm())
		return test_gsm_sms();
	else
		return test_cdma_sms();
}

static int test_misc()
{	
	LOGD("-----%s start-----\n", __func__);
	RIL_Errno ret = RIL_E_SUCCESS;
	char baseband_version[MAX_BUFFER_SIZE] = {0};

	ret = modem_get_baseband_version(baseband_version, NULL);
	ASSERT(baseband_version[0] != 0 && ret == RIL_E_SUCCESS);
	
	if(is_gsm())
	{
		char imei[MAX_BUFFER_SIZE] = {0}; 
		ret = modem_get_imei(imei, NULL);
		ASSERT(imei[0] != 0 && ret == RIL_E_SUCCESS);
	}
	
	if(s_context.has_sim)
	{
		char imsi[MAX_BUFFER_SIZE] = {0};
		ret = modem_get_imsi(imsi, NULL);
		ASSERT(isalnum(imsi[0]) && ret== RIL_E_SUCCESS);
	}
	
	ret = modem_set_radio_power(0);
	ASSERT(ret == RIL_E_SUCCESS);

	RIL_RadioState state = modem_get_radio_state();
	ASSERT(state == RADIO_STATE_OFF);

	ret = modem_set_radio_power(1);
	ASSERT(ret == RIL_E_SUCCESS);
	if(s_context.has_sim)
	{
		LOGD("-----%s sleep for a while after set radio power 1-----\n", __func__);
		sleep(60);
		ASSERT(is_icc_ready() && is_registration_ready());
	}
	else
	{
		LOGD("-----%s sleep for a while after set radio power 1-----\n", __func__);
		sleep(30);
		ASSERT(is_icc_locked_or_absent());
	}

	LOGD("-----%s end-----\n", __func__);

	return 1;
}

int main(int argc, char** argv)
{
	s_context_init();

	if(!modem_init(argc, argv))
	{
		LOGD("%s: modem init failed.", argv[0]);
		return -1;
	}

	exit_if_fail(init_test());
	test_misc();
	test_sim();
	test_cc();
	test_sms();

	modem_destroy();
	LOGD("RIL test end!!!!!");
	for(;;) sleep(30);

	return 0;
}

