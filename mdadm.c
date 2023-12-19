#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "mdadm.h"
#include "jbod.h"
// helper function that encodes jbod operations
uint32_t encodeop(int cmd, int disknum, int blocknum){
  uint32_t jbodop = 0;
  jbodop |= (cmd << 26);
  jbodop |= (disknum << 22);
  jbodop |= (blocknum);
  return jbodop;
}


//helper function makes calculations easy to translate the address into the parameters needs to seek to disk/block
void translateaddr(uint32_t addr, uint32_t *blocknum, uint32_t *blockoffset, uint32_t *disknum){

  *blocknum = (addr % JBOD_DISK_SIZE) / 256;
  *blockoffset = addr % JBOD_BLOCK_SIZE;
  *disknum = addr / JBOD_DISK_SIZE;
}

int mount;
//function mounts device when mount is 0.
int mdadm_mount(void)
{
  uint32_t jbodop = encodeop(JBOD_MOUNT, 0, 0);
  if (mount == 0)
  {
    jbod_operation(jbodop, NULL);
    mount = 1;
    return 1;
  }
  return -1;
}

//function unmounts device when mount is 1 or not 0.
int mdadm_unmount(void)
{
  uint32_t jbodop = encodeop(JBOD_UNMOUNT, 0, 0);
  if (mount != 0)
  {
    jbod_operation(jbodop, NULL);
    mount = 0;
    return 1;
  }
  return -1;
}

//function to find the minimum between length and the number of bytes to read.
int min(int a, int b) {
  if (a < b)
    return a;
  return b;
}


int mdadm_read(uint32_t addr, uint32_t len, uint8_t *buf){
//Invalid Paramaters that make read fail.
  if (mount == 0)
  {
    return -1;
  }
  if (len > 1024)
  {
    return -1;
  }
  if (addr + len > 0xfffff)
  {
    return -1;
  }
  if (buf == NULL && len > 0)
  {
    return -1;
  }

  uint32_t num_read = 0; //number of bytes read so far in block/disk
  uint32_t numbytestoread; //number of bytes left to read in block/disk

  while (len > 0) {
    uint32_t blocknum, offset, disknum;
    uint8_t readbuf[256];
    translateaddr(addr, &blocknum, &offset, &disknum);
    //makes calculations easy to translate the address into the parameters needs to seek to disk/block
    
    uint32_t jbodopseekdisk = encodeop(JBOD_SEEK_TO_DISK, disknum, 0);
    uint32_t jbodopseekblock = encodeop(JBOD_SEEK_TO_BLOCK, 0, blocknum);
    uint32_t jbodopread = encodeop(JBOD_READ_BLOCK, 0, 0);

    jbod_operation(jbodopseekdisk,NULL);
    jbod_operation(jbodopseekblock, NULL);
    jbod_operation(jbodopread,readbuf);
//operations used to seek for the disk and block that is being read, read operation reads the block into the temporary buffer    

    numbytestoread = min(len, min(256, 256 - offset));
// numbytestoread takes the min of len and 256-offset, whcih is number of bytes left to read
    memcpy(buf + num_read, readbuf + offset, numbytestoread);
// memcpy takes memory of the rest of the block/disk and the rest of the memory in the next block and copies to buffer
    
    num_read += numbytestoread; // num_read is now incremented
    len -= numbytestoread; // len is now incremented
    addr += numbytestoread; // addr is now incremented
  }
  return num_read;
}


int mdadm_write(uint32_t addr, uint32_t len, const uint8_t *buf) {
  //Invalid Parameters that make write fail.
  if (mount == 0){
    return -1;
  }
  if (addr + len > 0xfffff + 1){
    return -1;
  }
  if (len > 1024){
    return -1;
  }
  if (buf == NULL && len > 0){
    return -1;
  }

  uint32_t num_read = 0; //number of bytes read so far in block/disk
  uint32_t numbytestoread; //number of bytes left to read in block/disk

  while (len > 0) {
    uint32_t blocknum, offset, disknum;
    uint8_t readbuf[256];
    translateaddr(addr, &blocknum, &offset, &disknum);
    //makes calculations easy to translate the address into the parameters needs to seek to disk/block
    
    uint32_t jbodopseekdisk = encodeop(JBOD_SEEK_TO_DISK, disknum, 0);
    uint32_t jbodopseekblock = encodeop(JBOD_SEEK_TO_BLOCK, 0, blocknum);
    uint32_t jbodopread = encodeop(JBOD_READ_BLOCK, 0, 0);
    uint32_t jbodopwrite = encodeop(JBOD_WRITE_BLOCK, 0, 0);

    jbod_operation(jbodopseekdisk,NULL);
    jbod_operation(jbodopseekblock, NULL);
    jbod_operation(jbodopread,readbuf);   
//operations used to seek for the disk and block that is being read, read operation reads the block into the temporary buffer    

    numbytestoread = min(len, min(256, 256 - offset));
// numbytestoread takes the min of len and 256-offset, whcih is number of bytes left to read
    
    memcpy(readbuf + offset, buf + num_read, numbytestoread);
// memcpy takes memory of the rest of the block/disk and the rest of the memory in the next block and copies to temporary buffer
   

    num_read += numbytestoread; // num_read is now incremented
    len -= numbytestoread; // len is now incremented
    addr += numbytestoread; // addr is now incremented
    

    jbod_operation(jbodopseekdisk,NULL);
    jbod_operation(jbodopseekblock, NULL);
    jbod_operation(jbodopwrite, readbuf);
//operations used to seek for the disk and block that is being written, write operation is used
  }
  return num_read;

}