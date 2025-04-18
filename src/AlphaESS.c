/**
 * Jannis Lämmle
 * March 2025
 * To access the open.alphaess.com api using rp2040/2350 and w5500 ethernet module
 * Partially adapted from WIZnet Co.,Ltd, BSD-3-Clause
 */

#include <stdio.h>

#include "port_common.h"

#include "wizchip_conf.h"
#include "w5x00_spi.h"
#include "httpClient.h"

#include "dhcp.h"
#include "dns.h"
#include "sntp.h"

#include "mbedtls/sha512.h"

#include "time.h"

#include "string.h"

// Contains:
// #pragma once
// #define APP_ID "xyz"
// #define APP_SECRET "xyz"
// #define APP_SN "xyz"
#include "secrets.h"

// Socket usages (8 Sockets available on W5500)
#define SOCKET_DHCP 0
#define SOCKET_DNS 1
#define SOCKET_SNTP 2
#define SOCKET_HTTP 3

/* Retry count */
#define DHCP_RETRY_COUNT 5
#define RECV_TIMEOUT 10000000 // 10 seconds

/* NTP */
#define TIMEZONE 21 // UTC
// The timeserver to use
static uint8_t g_sntp_server_ip[4] = {216, 239, 35, 0}; // time.google.com

// Default Network settings
static wiz_NetInfo g_net_info =
{
    .mac = {0x00, 0x08, 0xDC, 0x12, 0x34, 0x56}, // MAC address
    .ip = {192, 168, 11, 2},                     // IP address
    .sn = {255, 255, 255, 0},                    // Subnet Mask
    .gw = {192, 168, 11, 1},                     // Gateway
    .dns = {8, 8, 8, 8},                         // DNS server
    .dhcp = NETINFO_DHCP                         // DHCP enable/disable
};

/* DHCP */
static uint8_t g_dhcp_get_ip_flag = 0;

/* DNS */
static uint8_t g_dns_target_domain[] = "openapi.alphaess.com";
static uint8_t g_http_target_uri[] = "/api/getLastPowerData?sysSn=" APP_SN;
static uint8_t g_dns_target_ip[4] = { 0, };

/* Buffers */
#define ETHERNET_BUF_MAX_SIZE (1024 * 2)
// DHCP and DNS Buffer
static uint8_t g_ethernet_buf[ETHERNET_BUF_MAX_SIZE] = { 0, };
// NTP Request Buffer
static uint8_t g_sntp_buf[ETHERNET_BUF_MAX_SIZE] = { 0, };
// HTTP Receive Buffer
static uint8_t g_http_r_buf[ETHERNET_BUF_MAX_SIZE] = { 0, };
// HTTP Send Buffer
static uint8_t g_http_s_buf[ETHERNET_BUF_MAX_SIZE] = { 0, };
// HTTP Custom header field buffer
static uint8_t g_http_h_buf[ETHERNET_BUF_MAX_SIZE] = { 0, };

/* Timer */
static volatile uint16_t g_msec_cnt = 0;

/* DHCP */
static void wizchip_dhcp_init(void);
static void wizchip_dhcp_assign(void);
static void wizchip_dhcp_conflict(void);

/* Timer */
static bool repeating_timer_callback(struct repeating_timer *t);
static time_t millis(void);

int main()
{
    /* Initialize */
    uint8_t retval = 0;
    uint8_t dhcp_retry = 0;
    uint8_t dns_retry = 0;
    uint32_t start_ms = 0;
    datetime time;

    stdio_init_all();

    wizchip_spi_initialize();
    wizchip_cris_initialize();

    wizchip_reset();
    wizchip_initialize();
    wizchip_check();

    // Register callback to run DHCP and DNS time handlers
    static struct repeating_timer g_timer;
    add_repeating_timer_us(-1000000, repeating_timer_callback, NULL, &g_timer);

    wizchip_dhcp_init();
    DNS_init(SOCKET_DNS, g_ethernet_buf);
    SNTP_init(SOCKET_SNTP, g_sntp_server_ip, TIMEZONE, g_sntp_buf);

    /* Lease DHCP */
    while (1)
    {
        retval = DHCP_run();

        if (retval == DHCP_IP_LEASED)
        {
            if (g_dhcp_get_ip_flag == 0)
            {
                printf("DHCP success\n");

                g_dhcp_get_ip_flag = 1;
                break;
            }
        }
        else if (retval == DHCP_FAILED)
        {
            g_dhcp_get_ip_flag = 0;
            dhcp_retry++;

            if (dhcp_retry <= DHCP_RETRY_COUNT)
            {
                printf("DHCP timeout occurred and retry %d\n", dhcp_retry);
            }
        }

        if (dhcp_retry > DHCP_RETRY_COUNT)
        {
            printf("DHCP failed\n");

            DHCP_stop();

            while (1);
        }

        sleep_ms(1000);
    }


    /* Get network information */
    print_network_information(g_net_info);

    /* Get DNS */
    if (DNS_run(g_net_info.dns, g_dns_target_domain, g_dns_target_ip) > 0)
    {
        printf("DNS success\n");
        printf("Target domain : %s\n", g_dns_target_domain);
        printf("IP of target domain : %d.%d.%d.%d\n", g_dns_target_ip[0], g_dns_target_ip[1], g_dns_target_ip[2], g_dns_target_ip[3]);
    }
    else
    {
        printf("DNS Failed");
        while(1);
    }

    /* Get time */
    absolute_time_t start_time = get_absolute_time();
    do
    {
        retval = SNTP_run(&time);

        if (retval == 1)
        {
            break;
        }
    } while (absolute_time_diff_us(get_absolute_time(), start_time) < RECV_TIMEOUT);

    if (retval != 1)
    {
        printf("SNTP failed : %d\n", retval);

        while (1);
    }

    printf("%d-%02d-%02d, %02d:%02d:%02d\n", time.yy, time.mo, time.dd, time.hh, time.mm, time.ss);


    httpc_init(SOCKET_HTTP, g_dns_target_ip, 80, g_http_s_buf, g_http_r_buf);

    bool send_success = false;
    while(1){
        httpc_connection_handler();
        if(httpc_isSockOpen){
            httpc_connect();
        }
        if(httpc_isConnected){
            if(!send_success){
                request.method = (uint8_t*)HTTP_GET;
                request.uri = (uint8_t *)g_http_target_uri;
                request.host = (uint8_t *)g_dns_target_domain;

                // unix timestamp
                tstamp timeStamp = changedatetime_to_seconds() - 2208988800L; // Seconds since 1900 -> Seconds since 1970
                uint8_t timeStamp_buf[32] = {0};
                sprintf(timeStamp_buf, "%llu", timeStamp);

                // sign = AppId+AppSecret+Timestamp with sha512 encoded as hex
                uint8_t secrets[129] = {0};
                uint8_t sha_out[64] = {0};
                uint16_t len = sprintf(secrets, "%s%s%s", APP_ID, APP_SECRET, timeStamp_buf);
                mbedtls_sha512_ret(secrets, len, sha_out, 0);
                for(int i = 0; i < 64; i++){
                    sprintf(secrets + 2*i, "%02x", sha_out[i]);
                }
                
                httpc_add_customHeader_field(g_http_h_buf, "appId", APP_ID);
                httpc_add_customHeader_field(g_http_h_buf, "timeStamp", timeStamp_buf);
                httpc_add_customHeader_field(g_http_h_buf, "sign", secrets);
                httpc_send_header(&request, g_http_r_buf, g_http_h_buf, 0);
                send_success = true;
            }
		    if(httpc_isReceived > 0)
		    {
                uint16_t len = httpc_recv(g_http_r_buf, httpc_isReceived);

                printf(" >> HTTP Response - Received len: %d\r\n", len);
                printf("======================================================\r\n");
                for(uint16_t i = 0; i < len; i++) printf("%c", g_http_r_buf[i]);
                printf("\r\n");
                printf("======================================================\r\n");
                break;
			}
        }
    }

    /* Infinite loop */
    while (1);
}

/* DHCP */
static void wizchip_dhcp_init(void)
{
    printf("DHCP client running\n");

    DHCP_init(SOCKET_DHCP, g_ethernet_buf);

    reg_dhcp_cbfunc(wizchip_dhcp_assign, wizchip_dhcp_assign, wizchip_dhcp_conflict);
}

static void wizchip_dhcp_assign(void)
{
    getIPfromDHCP(g_net_info.ip);
    getGWfromDHCP(g_net_info.gw);
    getSNfromDHCP(g_net_info.sn);
    getDNSfromDHCP(g_net_info.dns);

    g_net_info.dhcp = NETINFO_DHCP;

    /* Network initialize */
    network_initialize(g_net_info); // apply from DHCP

    print_network_information(g_net_info);
    printf("DHCP leased time : %ld seconds\n", getDHCPLeasetime());
}

static void wizchip_dhcp_conflict(void)
{
    printf("Conflict IP from DHCP\n");

    // halt or reset or any...
    while (1); // this example is halt.
}

/* Timer */
static bool repeating_timer_callback(struct repeating_timer *t)
{
    DHCP_time_handler();
    DNS_time_handler();
}