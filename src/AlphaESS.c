/**
 * alphaESS.c
 * Jannis Lämmle
 * To access the open.alphaess.com api using rp2040/2350 and w5500 ethernet module
 * Partially adapted from WIZnet Co.,Ltd, BSD-3-Clause
 */

#include "alphaESS.h"

bool alphaESS_run()
{
    /* Initialize */
    uint8_t retval = 0;
    uint8_t dhcp_retry = 0;
    uint8_t dns_retry = 0;
    uint32_t start_ms = 0;
    datetime time;

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
            printf("DHCP success\n");

            break;
        }
        else if (retval == DHCP_FAILED)
        {
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

            return false;
        }

        sleep_ms(100);
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

        return false;
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
                
                g_http_h_buf[0] = 0;
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

    return true;
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