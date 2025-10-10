/**
 * Note: this file may not be necessary anymore. 
 * */

#ifndef UPLOAD_SAT_UPLOAD_H
#define UPLOAD_SAT_UPLOAD_H

#define UPLOAD_TIMEOUT 10000
#define VMEM_LOCATE_TIMEOUT 10000
#define VMEM_NAME "buffer.vmem"

#include <vmem/vmem_file.h>
#include <vmem/vmem.h>
#define BUFFER_LIST_SIZE 10
#define BUFFER_VMEM_SIZE 100000000

/**
 * Upload data to another node at the specified address.
 * @param data Image batch data to upload to radio. 
 * @param len Length of data to upload.
*/
void upload(unsigned char *data, int len);

#endif
