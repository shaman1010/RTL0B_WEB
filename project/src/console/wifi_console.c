/*
 * wifi_console.c
 *
 *  Created on: 03/04/2017
 *      Author: pvvx
 */

#include "platform_opts.h"
#include "device.h"
#include "autoconf.h"
#include "FreeRTOS.h"
#include "diag.h"
#include "wifi_api.h"
#include "hal_platform.h"
#include "freertos_pmu.h"

#include "console_api.h"
#include "lwip/netif.h"

#define LOCAL static


extern struct netif xnetif[NET_IF_NUM];
extern char str_rom_41ch3Dch0A[]; // "========================================\n" 41


//==========================================================
//--- CONSOLE --------------------------

// ATPN=<SSID>[,password[,encryption[,auto reconnect[,reconnect pause]]]: WIFI Connect to AP
LOCAL void fATPN(int argc, unsigned char *argv[]){
	if(argc > 1) {
		if(argv[1][0] == '?') {
			show_wifi_st_cfg();
		}
		else {
			strncpy(wifi_st_cfg.ssid, argv[1], NDIS_802_11_LENGTH_SSID);
			int pswlen;
			if(argc > 2) {
				pswlen = strlen(wifi_st_cfg.password);
				strncpy(wifi_st_cfg.password, argv[2], NDIS_802_11_LENGTH_SSID);
				if(pswlen > 7) {
					wifi_st_cfg.security = IDX_SECURITY_WPA2_AES_PSK;
				}
				else if(!pswlen) {
					wifi_st_cfg.security = IDX_SECURITY_OPEN;
				}
				else {
					printf("password len < 8!\n");
					wifi_st_cfg.security = IDX_SECURITY_OPEN;
				}
			}
			else {
				// default
				wifi_st_cfg.password[0] = 0;
				wifi_st_cfg.security = IDX_SECURITY_OPEN;
			}
			if(argc > 3) {
				if(pswlen > 7) {
					wifi_st_cfg.security = atoi(argv[3]);
				}
				else {
					printf("password len < 8!\n");
					wifi_st_cfg.security = IDX_SECURITY_OPEN;
				}
			}
			if(argc > 4) {
				wifi_st_cfg.autoreconnect = atoi(argv[4]);
			}
			else wifi_st_cfg.autoreconnect = 0;
			if(argc > 5) {
				wifi_st_cfg.reconnect_pause = atoi(argv[5]);
			}
			else wifi_st_cfg.reconnect_pause = 5;
			show_wifi_st_cfg();
#if CONFIG_WLAN_CONNECT_CB
			connect_close();
#endif
			wifi_run(wifi_run_mode | RTW_MODE_STA);
		}
	}
}

// ATPA=<SSID>[,password[,encryption[,channel[,hidden[,max connections]]]]]: Start WIFI AP
LOCAL void fATPA(int argc, unsigned char *argv[]){
	if(argc > 1) {
		if(argv[1][0] == '?') {
			show_wifi_ap_cfg();
		}
		else {
			strncpy(wifi_ap_cfg.ssid, argv[1], NDIS_802_11_LENGTH_SSID);
			if(argc > 2) {
				strncpy(wifi_ap_cfg.password, argv[2], NDIS_802_11_LENGTH_SSID);
				int i = strlen(wifi_ap_cfg.password);
				if(i > 7) {
					wifi_ap_cfg.security = 1; // IDX_SECURITY_WPA2_AES_PSK;
				}
				else if(i == 0) {
					wifi_ap_cfg.security = 0; // IDX_SECURITY_OPEN;
				}
				else {
					printf("password len < 8!\n");
					wifi_ap_cfg.security = 0; // IDX_SECURITY_OPEN;
				}
			}
			else {
				wifi_ap_cfg.password[0] = 0;
				wifi_ap_cfg.security = 0; // IDX_SECURITY_OPEN;
			}
			if(argc > 3) {
				wifi_ap_cfg.security = (argv[3][0] == '0')? 0 : 1; //RTW_SECURITY_OPEN : RTW_SECURITY_WPA2_AES_PSK;
			}
			if(argc > 4) {
				wifi_ap_cfg.channel = atoi(argv[4]);
			}
			else wifi_ap_cfg.channel = 1;
			if(argc > 5) {
				wifi_ap_cfg.ssid_hidden = atoi(argv[5]);
			}
			else wifi_ap_cfg.ssid_hidden = 0;

			if(argc > 6) {
				wifi_ap_cfg.max_sta = atoi(argv[6]);
			}
			else wifi_ap_cfg.max_sta = 3;

			show_wifi_ap_cfg();
#if CONFIG_WLAN_CONNECT_CB
			connect_close();
#endif
			wifi_run(wifi_run_mode | RTW_MODE_AP);
		}
	}
}

// WIFI Connect, Disconnect
LOCAL void fATWR(int argc, unsigned char *argv[]){
	rtw_mode_t mode = RTW_MODE_NONE;
	if(argc > 1) mode = atoi(argv[1]);
#if CONFIG_WLAN_CONNECT_CB
	connect_close();
#endif
	wifi_run(mode);
}

LOCAL void fATWI(int argc, unsigned char *argv[]) {
#if 1
	if(argc > 2) {
		uint8_t c = argv[1][0] | 0x20;
		int i = atoi(argv[2]);
		if(c == 's') {
			printf("Save settings(%d)..\n", i);
			write_wifi_cfg(i);
		}
		else if(c == 'l') {
			printf("Set wifi_cfg.load_flg %d\n", i);
			wifi_cfg.load_flg = atoi(argv[2]);
		}
		else if(c == 'm') {
			printf("Set wifi_cfg.mode %d\n", i);
			wifi_cfg.mode = atoi(argv[2]);
		}
	}
#endif
	rtw_wifi_setting_t Setting;
	if((wifi_run_mode & RTW_MODE_AP)
		&& wifi_get_setting(wlan_ap_name, &Setting) == 0) {
		wifi_show_setting(wlan_ap_name, &Setting);
//		show_wifi_ap_ip();
		printf("\tIP: " IPSTR "\n\n", IP2STR(&xnetif[WLAN_AP_NETIF_NUM].ip_addr));
//#if CONFIG_DEBUG_LOG > 1
		show_wlan_info(WLAN_AP_NETIF_NUM);
//#endif
	}
	if((wifi_run_mode & RTW_MODE_STA)
		&& wifi_get_setting(wlan_st_name, &Setting) == 0) {
		wifi_show_setting(wlan_st_name, &Setting);
//		show_wifi_st_ip();
		printf("\tIP: " IPSTR "\n\n", IP2STR(&xnetif[WLAN_ST_NETIF_NUM].ip_addr));
//#if CONFIG_DEBUG_LOG > 1
		show_wlan_info(WLAN_ST_NETIF_NUM);
//#endif
	}
	printf("\nWIFI config:\n");
	printf(&str_rom_41ch3Dch0A[8]); // "================================\n"
	show_wifi_cfg();
	printf("\nWIFI AP config:\n");
	printf(&str_rom_41ch3Dch0A[8]); // "================================\n"
	show_wifi_ap_cfg();
	printf("\nWIFI ST config:\n");
	printf(&str_rom_41ch3Dch0A[8]); // "================================\n"
	show_wifi_st_cfg();
#if SDK_VER_NUM > 0x4000
	if(wifi_mode & RTW_MODE_AP) {
		printf("\nWIFI AP clients:\n");
		printf(&str_rom_41ch3Dch0A[8]); // "================================\n"
		show_wifi_ap_clients();
	}
#endif
	printf("\n");
}

extern uint8_t rtw_power_percentage_idx;
extern int rltk_set_tx_power_percentage(rtw_tx_pwr_percentage_t power_percentage_idx);

void fATWT(int argc, unsigned char *argv[]) {
	(void) argc; (void) argv;
	if(argc > 1) {
		int txpwr = atoi(argv[1]);
		debug_printf("set tx power (%d)...\n", txpwr);
		if(rltk_set_tx_power_percentage(txpwr) != RTW_SUCCESS) {
			error_printf("Error set tx power (%d)!", wifi_cfg.tx_pwr);
		}
	}
	printf("TX power = %d\n", rtw_power_percentage_idx);
}

/*
//-- Test tsf (64-bits counts, 1 us step) ---

//#include "hal_com_reg.h"
#define WIFI_REG_BASE               0x40080000
#define REG_TSFTR						0x0560
#define REG_TSFTR1						0x0568	// HW Port 1 TSF Register

#define ReadTSF_Lo32() (*((volatile unsigned int *)(WIFI_REG_BASE + REG_TSFTR)))
#define ReadTSF_Hi32() (*((volatile unsigned int *)(WIFI_REG_BASE + REG_TSFTR1)))

LOCAL uint64_t get_tsf(void)
{
	return *((uint64_t *)(WIFI_REG_BASE + REG_TSFTR));
}
*/

void fATSF(int argc, unsigned char *argv[])
{
	(void) argc; (void) argv;
	uint64_t tsf = wifi_get_tsf();
	printf("\nTSF: %08x%08x\n", (uint32_t)(tsf>>32), (uint32_t)(tsf));
}
#if 0
/*------------------------------------------------------------------------------
 * power saving mode
 *----------------------------------------------------------------------------*/
LOCAL void fATSP_(int argc, unsigned char *argv[])
{
	if(argc > 2) {
		switch (argv[1][0]) {
		case 'a': // acquire
		{
			pmu_acquire_wakelock(1<<atoi(argv[2]));
			break;
		}
		case 'r': // release
		{
			pmu_release_wakelock(1<<atoi(argv[2]));
			break;
		}
		};
	};
	printf("WakeLock Status %d\n", pmu_get_wakelock_status());
}
#endif
/* --------  WiFi Scan ------------------------------- */
LOCAL rtw_result_t scan_result_handler(internal_scan_handler_t* ap_scan_result)
{
	if (ap_scan_result) {
		if(ap_scan_result->scan_cnt) {
			printf("\nScan networks:\n\n");
			printf("N\tType\tMAC\t\t\tSignal\tCh\tWPS\tSecyrity\tSSID\n\n");
			for(int i = 0 ; i < ap_scan_result->scan_cnt; i++) {
				rtw_scan_result_t* record = &ap_scan_result->ap_details[i];
				printf("%d\t", i+1);
			    printf("%s\t", (record->bss_type == RTW_BSS_TYPE_ADHOC)? "Adhoc": "Infra");
			    printf(MAC_FMT, MAC_ARG(record->BSSID.octet));
			    printf("\t%d\t", record->signal_strength);
			    printf("%d\t", record->channel);
			    printf("%d\t", record->wps_type);
			    {
			    	uint8_t * s = rtw_security_to_str(record->security);
			    	printf("%s\t", s);
			    	if(strlen(s) < 8) printf("\t");
			    }
			    record->SSID.val[record->SSID.len] = '\0';
			    printf("%s\n", record->SSID.val);
			}
		}
	} else {
		printf("Scan networks: None!\n");
	}
	return RTW_SUCCESS;
}
/* --------  WiFi Scan ------------------------------- */
void fATSN(int argc, unsigned char *argv[])
{
	(void) argc;
	(void) argv;
	if(wifi_mode == RTW_MODE_NONE) {
		wifi_run(RTW_MODE_PROMISC);
	}
	api_wifi_scan(scan_result_handler);
}

#if defined(CONFIG_ENABLE_WPS_AP) && CONFIG_ENABLE_WPS_AP
extern void cmd_ap_wps(int argc, char **argv);
extern void cmd_wps(int argc, char **argv);
//extern void cmd_wifi_on(int argc, char **argv);
#endif
#if CONFIG_ENABLE_P2P
extern void cmd_wifi_p2p_start(int argc, char **argv);
extern void cmd_wifi_p2p_stop(int argc, char **argv);
extern void cmd_p2p_listen(int argc, char **argv);
extern void cmd_p2p_find(int argc, char **argv);
extern void cmd_p2p_peers(int argc, char **argv);
extern void cmd_p2p_info(int argc, char **argv);
extern void cmd_p2p_disconnect(int argc, char **argv);
extern void cmd_p2p_connect(int argc, char **argv);
extern void cmd_wifi_p2p_auto_go_start(int argc, char **argv);
extern void cmd_p2p_peers(int argc, char **argv);
#endif //CONFIG_ENABLE_P2P

MON_RAM_TAB_SECTION MON_COMMAND_TABLE console_cmd_wifi_api[] = {
		{"STA", 1, fATPN, "=<SSID>[,password[,encryption[,auto-reconnect[,reconnect pause]]]: WIFI Connect to AP"},
		{"AP", 1, fATPA, "=<SSID>[,password[,encryption[,channel[,hidden[,max connections]]]]]: Start WIFI AP"},
#if defined(CONFIG_ENABLE_WPS_AP) && CONFIG_ENABLE_WPS_AP
		{"WPS_AP", 1, cmd_ap_wps, "=<pbc/pin>[,pin]: WiFi AP WPS"},
		{"WPS_ST", 1, cmd_wps, "=<pbc/pin>[,pin]: WiFi Station WPS"},
#endif
#if CONFIG_ENABLE_P2P
		{"P2P_START", 0, cmd_wifi_p2p_start, ": p2p start" },
		{"P2P_ASTART", 0, cmd_wifi_p2p_auto_go_start, ": p2p auto go start" },
		{"P2P_STOP", 0, cmd_wifi_p2p_stop, ": p2p stop"},
		{"P2P_PEERS", 0, cmd_p2p_peers, ": p2p peers" },
		{"P2P_FIND", 0, cmd_p2p_find, ": p2p find"},
		{"P2P_INFO", 0, cmd_p2p_info, ": p2p info"},
		{"P2P_DISCCONNECT", 0, cmd_p2p_disconnect, ": p2p disconnect"},
		{"P2P_CONNECT", 0, cmd_p2p_connect, ": p2p connect"},
#endif
		{"MODE", 0, fATWR, "=[mode]: WIFI Mode: 0 - off, 1 - ST, 2 - AP, 3 - ST+AP, 4 - PROMISC, 5 - P2P"},
		{"WINFO", 0, fATWI, ": WiFi Info"},
#if CONFIG_DEBUG_LOG > 3
		{"TXPOW", 1, fATWT, "=<tx_power>: WiFi tx power: 0 - 100%, 1 - 75%, 2 - 50%, 3 - 25%, 4 - 12.5%"},
		{"TSF", 0, fATSF, ": Get TSF value"},
#endif
//		{"ATWP", 0, fATWP, "=[dtim]: 0 - WiFi ipc/lpc off, 1..10 - on + dtim"},
		{"SCAN", 0, fATSN, ": Scan networks"}
};
