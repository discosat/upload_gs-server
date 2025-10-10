#include <unistd.h>
#include <stdio.h>
#include <vmem/vmem_client.h>
#include <param/param.h>
#include <param/param_client.h>
#include <vmem/vmem_ring.h>
#include "vmem_upload_local.h"
#include "vmem_upload_param.h"
#include "vmem_ring_buffer.h"
#include "protobuf/metadata.pb-c.h"

/* Local ring-buffer */
void upload(unsigned char *data, int num, int len) 
{
	printf("vmem_upload_local:upload\n");
}
