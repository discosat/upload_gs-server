/**
 * Copied and edited from: https://github.com/spaceinventor/libcsp/blob/60e4804ea8451e6202ce2c5c5abc0342ad3b55a4/examples/csp_server.c
 */

#include <csp/csp_debug.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <pthread.h>
#include <stdio.h>

#include <csp/csp.h>
#include <csp/drivers/usart.h>
#include <csp/drivers/can_socketcan.h>
#include <csp/interfaces/csp_if_zmqhub.h>
#include <param/param_server.h>
#include <dtp/dtp.h>

#include <vmem/vmem_server.h>
#include <vmem/vmem_file.h>
#include <vmem/vmem_ring.h>
#include "vmem_storage.h"
#include "vmem_ring_buffer.h"
#include "vmem_dtp_server.h"

/* Server port, the port the server listens on for incoming connections from the client. */
#define SERVER_PORT 10
/* CSP read timeout in seconds*/
#define CSP_TIMEOUT 50

/* These functions must be provided in arch specific way */
void *dtp_server_task(void *param);
void *upload_server_task(void *param);

// Vmem server stuff.
static void *vmem_server_task(void *param)
{
    printf("main:vmem_server_task: received param address: %p\n", &param);
    vmem_server_loop(param);
    return NULL;
}

// For DTP server stuff.
void *dtp_server_task(void *param)
{
    bool keep_running = true;
    dtp_server_main(&keep_running);
    return NULL;
}

// For uploading.
void *upload_server_task(void *param)
{
    static csp_socket_t sock = {0};
    sock.opts = CSP_O_RDP;
    csp_bind(&sock, INDECES_PORT);
    csp_listen(&sock, 1); // This allows only one simultaneous connection

    csp_conn_t *conn;

    printf("%sUpload server listening on port %d %s\n", "\x1B[33m", SERVER_PORT, "\x1B[0m");

    while (1)
    {
        if ((conn = csp_accept(&sock, 10000)) == NULL)
        {
            continue;
        }

        printf("%s\t - Incomming upload request... %s\n", "\x1B[33m", "\x1B[0m");

        csp_packet_t *request = csp_read(conn, CSP_TIMEOUT);

        /* Is this correct?*/
        uint32_t observation_amount = vmem_ring_get_amount_of_elements(&vmem_images);

        csp_packet_t *response = csp_buffer_get(sizeof(uint32_t));
        response->length = sizeof(uint32_t);
        memcpy(response->data, &observation_amount, sizeof(uint32_t));
        csp_send(conn, response);
        csp_buffer_free(response);

        csp_buffer_free(request);

        printf("%s\t - Closing CSP connection... %s\n", "\x1B[33m", "\x1B[0m");

        csp_close(conn);
    }
}

// For routing.
void *router_task(void *param)
{
    printf("main:router_task: received param address: %p\n", &param);
    while (1)
    {
        csp_route_work();
    }
    return NULL;
}

/* Commandline options */
static uint8_t server_address = 0;

/* Test mode, check that server & client can exchange packets */
static bool test_mode = false;
static unsigned int server_received = 0;
static unsigned int run_duration_in_sec = 3;

enum DeviceType
{
    DEVICE_UNKNOWN,
    DEVICE_CAN,
    DEVICE_KISS,
    DEVICE_ZMQ,
};

#define __maybe_unused __attribute__((__unused__))

static struct option long_options[] = {
    {"kiss-device", required_argument, 0, 'k'},
#if (CSP_HAVE_LIBSOCKETCAN)
#define OPTION_c "c:"
    {"can-device", required_argument, 0, 'c'},
#else
#define OPTION_c
#endif
#if (CSP_HAVE_LIBZMQ)
#define OPTION_z "z:"
    {"zmq-device", required_argument, 0, 'z'},
#else
#define OPTION_z
#endif
#if (CSP_USE_RTABLE)
#define OPTION_R "R:"
    {"rtable", required_argument, 0, 'R'},
#else
#define OPTION_R
#endif
    {"interface-address", required_argument, 0, 'a'},
    {"connect-to", required_argument, 0, 'C'},
    {"test-mode", no_argument, 0, 't'},
    {"test-mode-with-sec", required_argument, 0, 'T'},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0}};

void print_help()
{
    csp_print("Usage: csp_server [options]\n");
    if (CSP_HAVE_LIBSOCKETCAN)
    {
        csp_print(" -c <can-device>  set CAN device\n");
    }
    if (1)
    {
        csp_print(" -k <kiss-device> set KISS device\n");
    }
    if (CSP_HAVE_LIBZMQ)
    {
        csp_print(" -z <zmq-device>  set ZeroMQ device\n");
    }
    if (CSP_USE_RTABLE)
    {
        csp_print(" -R <rtable>      set routing table\n");
    }
    if (1)
    {
        csp_print(" -a <address>     set interface address\n"
                  " -t               enable test mode\n"
                  " -T <dration>     enable test mode with running time in seconds\n"
                  " -h               print help\n");
    }
}

csp_iface_t *add_interface(enum DeviceType device_type, const char *device_name)
{
    csp_iface_t *default_iface = NULL;

    if (device_type == DEVICE_KISS)
    {
        csp_usart_conf_t conf = {
            .device = device_name,
            .baudrate = 115200, /* supported on all platforms */
            .databits = 8,
            .stopbits = 1,
            .paritysetting = 0,
        };
        int error = csp_usart_open_and_add_kiss_interface(&conf, CSP_IF_KISS_DEFAULT_NAME, &default_iface);
        if (error != CSP_ERR_NONE)
        {
            csp_print("failed to add KISS interface [%s], error: %d\n", device_name, error);
            exit(1);
        }
        default_iface->is_default = 1;
    }

    if (CSP_HAVE_LIBSOCKETCAN && (device_type == DEVICE_CAN))
    {
        int error = csp_can_socketcan_open_and_add_interface(device_name, CSP_IF_CAN_DEFAULT_NAME, server_address, 1000000, true, &default_iface);
        if (error != CSP_ERR_NONE)
        {
            csp_print("failed to add CAN interface [%s], error: %d\n", device_name, error);
            exit(1);
        }
        default_iface->is_default = 1;
    }

    if (CSP_HAVE_LIBZMQ && (device_type == DEVICE_ZMQ))
    {
        int error = csp_zmqhub_init(server_address, device_name, 0, &default_iface);
        if (error != CSP_ERR_NONE)
        {
            csp_print("failed to add ZMQ interface [%s], error: %d\n", device_name, error);
            exit(1);
        }
        default_iface->is_default = 1;
    }

    return default_iface;
}

/* main - initialization of CSP and start of server task */
int main(int argc, char *argv[])
{

    const char *device_name = NULL;
    enum DeviceType device_type = DEVICE_UNKNOWN;
    const char *rtable __maybe_unused = NULL;
    csp_iface_t *default_iface;
    int opt;

    while ((opt = getopt_long(argc, argv, OPTION_c OPTION_z OPTION_R "k:a:tT:h", long_options, NULL)) != -1)
    {
        switch (opt)
        {
        case 'c':
            device_name = optarg;
            device_type = DEVICE_CAN;
            break;
        case 'k':
            device_name = optarg;
            device_type = DEVICE_KISS;
            break;
        case 'z':
            device_name = optarg;
            device_type = DEVICE_ZMQ;
            break;
#if (CSP_USE_RTABLE)
        case 'R':
            rtable = optarg;
            break;
#endif
        case 'a':
            server_address = atoi(optarg);
            break;
        case 't':
            test_mode = true;
            break;
        case 'T':
            test_mode = true;
            run_duration_in_sec = atoi(optarg);
            break;
        case 'h':
            print_help();
            exit(EXIT_SUCCESS);
        case '?':
            // Invalid option or missing argument
            print_help();
            exit(EXIT_FAILURE);
        }
    }

    // If more than one of the interfaces are set, print a message and exit
    if (device_type == DEVICE_UNKNOWN)
    {
        csp_print("Only one of the interfaces can be set.\n");
        print_help();
        exit(EXIT_FAILURE);
    }

    csp_print("Initialising CSP\n");

    /* Init CSP */
    csp_init();

    /* Add interface(s) */
    default_iface = add_interface(device_type, device_name);

    /* Setup routing table */
    if (CSP_USE_RTABLE)
    {
        if (rtable)
        {
            int error = csp_rtable_load(rtable);
            if (error < 1)
            {
                csp_print("csp_rtable_load(%s) failed, error: %d\n", rtable, error);
                exit(1);
            }
        }
        else if (default_iface)
        {
            csp_rtable_set(0, 0, default_iface, CSP_NO_VIA_ADDRESS);
        }
    }

    csp_print("Connection table\r\n");
    csp_conn_print_table();

    csp_print("Interfaces\r\n");
    csp_iflist_print();

    if (CSP_USE_RTABLE)
    {
        csp_print("Route table\r\n");
        csp_rtable_print();
    }

    static pthread_t router_handle;
    pthread_create(&router_handle, NULL, &router_task, NULL);

    static pthread_t vmem_server_handle;
    pthread_create(&vmem_server_handle, NULL, &vmem_server_task, NULL);

    static pthread_t dtp_server_handle;
    pthread_create(&dtp_server_handle, NULL, &dtp_server_task, NULL);

    static pthread_t upload_server_handle;
    pthread_create(&upload_server_handle, NULL, &upload_server_task, NULL);

    /* Wait for execution to end (ctrl+c) */
    while (1)
    {
        sleep(run_duration_in_sec);

        if (test_mode)
        {
            /* Test mode, check that server & client can exchange packets */
            if (server_received < 5)
            {
                csp_print("Server received %u packets\n", server_received);
                exit(EXIT_FAILURE);
            }
            csp_print("Server received %u packets\n", server_received);
            exit(EXIT_SUCCESS);
        }
    }

    return 0;
}
