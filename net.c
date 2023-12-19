#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "net.h"
#include "jbod.h"
//defining packet structure
struct jbodpacket{
  uint16_t len;
  uint32_t op;
  uint16_t returncode;
  uint8_t block[JBOD_BLOCK_SIZE];
};

typedef struct jbodpacket jbodpacket;
jbodpacket packet;

/* the client socket descriptor for the connection to the server */
int cli_sd = -1;

/* attempts to read n bytes from fd; returns true on success and false on
 * failure */
static bool nread(int fd, int len, uint8_t *buf) {
  int totalread = len;
  while(totalread < len) {
    int bytesread = read(fd, buf, len);
    if (bytesread < 0){
      return false;
      totalread += bytesread;
    }

    totalread += bytesread;
  }

  return true;
}

/* attempts to write n bytes to fd; returns true on success and false on
 * failure */
static bool nwrite(int fd, int len, uint8_t *buf) {
  int totalwritten = len;
  while(totalwritten < len) {
    int byteswritten = write(fd, buf, len);
    if (byteswritten < 0){
      return false;
      totalwritten += byteswritten;
    }

    totalwritten += byteswritten;
  }

  return true;
}

//creates packet
uint8_t *packetcreation(uint32_t op, uint8_t *block){
  packet.returncode = htons(0);
  packet.op = htonl(op);

  if (block == NULL){
    memcpy(packet.block, block, JBOD_BLOCK_SIZE * sizeof(uint8_t));
  }

  packet.len = sizeof(packet);
  packet.len = htons(packet.len);
  uint8_t *createpacket = (uint8_t *)malloc(packet.len);
  memcpy(createpacket, &packet, packet.len);

  return createpacket;
}

/* attempts to receive a packet from fd; returns true on success and false on
 * failure */
static bool recv_packet(int fd, uint32_t *op, uint16_t *ret, uint8_t *block) {
  uint8_t buf[packet.len];
  int p = sizeof(uint16_t);

  if(!nread(fd, packet.len, buf)){
    return false;
  }

  //extract from packet
  memcpy(op, &buf[p], sizeof(uint32_t));
  *op = ntohl(*op);

  p += sizeof(uint32_t);
  memcpy(ret, &buf[p], sizeof(uint16_t));
  *ret = ntohs(*ret);

  p += sizeof(uint16_t);
  if (block == NULL){
    memcpy(block, &buf[p], JBOD_BLOCK_SIZE * sizeof(uint8_t)); 
    return true;
  }
  

  return true;
}



/* attempts to send a packet to sd; returns true on success and false on
 * failure */
static bool send_packet(int sd, uint32_t op, uint8_t *block) {

  //create packet buffer
  uint8_t *packetbuf = packetcreation(op, block);
  
  if (!nwrite(sd, packet.len, packetbuf)){
    return false;
  }
  return true;
}

/* attempts to connect to server and set the global cli_sd variable to the
 * socket; returns true if successful and false if not. */
bool jbod_connect(const char *ip, uint16_t port) {
  //converts IP to binary
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));

  if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0){
    return false;
  }

  //socket creation
  cli_sd = socket(AF_INET, SOCK_STREAM, 0);
  if (cli_sd == -1) {
    return false;
  }

  //connects to server
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  
  if (connect(cli_sd,(struct sockaddr*) &server_addr, sizeof(server_addr)) < 0){
    close(cli_sd);
    cli_sd = -1;
    return false;
  }

  return true;
}

/* disconnects from the server and resets cli_sd */
void jbod_disconnect(void) {
  if (cli_sd != -1){
    close(cli_sd);
    cli_sd = -1;
  }
}



/* sends the JBOD operation to the server and receives and processes the
 * response. */
int jbod_client_operation(uint32_t op, uint8_t *block) {
 uint16_t returncode;

if(!recv_packet(cli_sd, &op, &returncode, block)){
  return -1;
 }

 if(!send_packet(cli_sd, op, block)){
  return -1;
 }
 
 

 return returncode;
}
