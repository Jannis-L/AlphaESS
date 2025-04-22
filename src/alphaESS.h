/**
 * alphaESS.h
 * Jannis Lämmle
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
static void wizchip_dhcp_init();
static void wizchip_dhcp_assign();
static void wizchip_dhcp_conflict();
bool alphaESS_run();

/* Timer */
static bool repeating_timer_callback(struct repeating_timer *t);