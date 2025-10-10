#include "vmem_dtp_server.h"
#include <dtp/dtp.h>
#include <dtp/platform.h>
#include <vmem/vmem.h>
#include <vmem/vmem_ring.h>
#include "vmem_ring_buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <csp/csp.h>

typedef struct observation_meta {
    uint16_t index;
    uint32_t size;
    uint32_t obid;
} observation_meta_t;

// Read the observation at specified index within ring buffer
static uint32_t observation_read(uint16_t index, uint32_t offset_within_observation, void *output, uint32_t size) {
    	printf("vmem_dtp_server:observation_read");
	return NULL; // Assume that everything has been read, since the vmem api doesn't return any value to indicate how much data is read
}

// This method is implemented to tell the DTP server which payload to use
// The provided payload_id is intepreted as the index
bool get_payload_meta(dftp_payload_meta_t *meta, uint16_t payload_id) {
    	printf("vmem_dtp_server:get_payload_meta");
	return true;
}

// Get size of metadata for a specific observation
static uint32_t observation_get_meta_size(uint16_t index) {
    	printf("vmem_dtp_server:observation_get_meta_size\n");
	return NULL;
}

// Get metadata of a specific observation
static Metadata *observation_get_metadata(uint16_t index) {
    	printf("vmem_dtp_server:observation_get_metadata\n");
	return NULL;
}

// Get metadata for a specific observation which is to be transferred, in this case only OBID, size and index
observation_meta_t observation_get_dtp_meta(uint16_t index) {
	printf("vmem_dtp_server:observation_get_dtp_meta\n");
	return NULL;
}

// Server for serving metadata of ring buffer observation
void dtp_indeces_server() {    
	printf("vmem_dtp_server:dtp_indices_server");
}
