/*#define YC_DEVICE_TYPE_32*/
#if 0
#include <stdio.h>
#define yc_printf_1(format, arg...) printf(format, ##arg)
#else
#define yc_printf_1(format, arg...)
#endif
/*
 *  Copyright 2013 People Power Company
 *
 *  This code was developed with funding from People Power Company
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
/*
 *  This is AIOX creature.
 */

#include <time.h>
#include <stdint.h>
#include <net/if.h>
#include <assert.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <unistd.h>

#include "pc_settings.h"
#include "ioterror.h"
#include "eui64.h"
#include <stddef.h>


#ifndef EUI64_BYTES_SIZE
#define EUI64_BYTES_SIZE 8
#endif

/*
 yctung
#define DEVICE_TYPE_SIZE 8
#define DEVICE_TYPE_SIZE 9
 yctung
*/

/**
 * Obtain the 48-bit MAC dest and convert to an EUI-64 value from the
 * hardware NIC
 *
 * @param dest Buffer of at least 8 bytes
 * @param destLen Length of the buffer
 * @return SUCCESS if we are able to capture the EUI64
 */
error_t eui64_toBytes(uint8_t *dest, size_t destLen) {
  struct ifreq *ifr;
  struct ifconf ifc;
  char buf[1024];
  int sock, i;
  int ok = 0;

  assert(dest);

  if(destLen < EUI64_BYTES_SIZE) {
    return FAIL;
  }

  memset(dest, 0x0, destLen);

  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock == -1) {
    return -1;
  }
/* yctung
  ifc.ifc_len = sizeof(buf);
  ifc.ifc_buf = buf;
  ioctl(sock, SIOCGIFCONF, &ifc);

  ifr = ifc.ifc_req;
  for (i = 0; i < ifc.ifc_len / sizeof(struct ifreq); ifr++) {
      if (strcmp(ifr->ifr_name, "eth0") == 0 || strcmp(ifr->ifr_name, "eth1")
	      == 0 || strcmp(ifr->ifr_name, "wlan0") == 0 || strcmp(ifr->ifr_name,
		  "br0") == 0) {
	  if (ioctl(sock, SIOCGIFFLAGS, ifr) == 0) {
	      if (!(ifr->ifr_flags & IFF_LOOPBACK)) {
		  if (ioctl(sock, SIOCGIFHWADDR, ifr) == 0) {
		      ok = 1;
		      break;
		  }
	      }
	  }
      }
  }
*/
    char *ifName[] = {"br-lan", "eth0", "eth1"};    /* for aiox mt7688 */
    char buf2[64];
    int j;
    struct ifreq ifr2;

    /* for aiox mt7688 */
    printf("%s:search list = ", __FUNCTION__);
    for (j = 0; j < sizeof(ifName)/sizeof(ifName[0]); j++)
        printf("%s, ", ifName[j]);
    printf("\n");
    buf2[0] = 0;
    ifr = &ifr2;
    for (j = 0; j < sizeof(ifName)/sizeof(ifName[0]); j++)
    {
        ifr->ifr_addr.sa_family = AF_INET;
        strncpy(ifr->ifr_name, ifName[j], IFNAMSIZ - 1);
        printf("%s:1,%s==========\n", __FUNCTION__, ifr->ifr_name);
        if (ioctl(sock, SIOCGIFHWADDR, ifr) == 0)
        {printf("%s:2,hit==========\n", __FUNCTION__);
            strcpy(buf2, ifr->ifr_name);
            ok = 1;
            goto lhit;
        }
    }
    if (!ok)
    {
        /* for intel platform, ubuntu, rpi */
        printf("%s:3,fail to search, so search active interface except loopback interface==========\n", __FUNCTION__);
        ifc.ifc_len = sizeof(buf);
        ifc.ifc_buf = buf;
        ioctl(sock, SIOCGIFCONF, &ifc);
        for (ifr = ifc.ifc_req, i = 0; i < ifc.ifc_len / sizeof(struct ifreq); i++, ifr++)
        {printf("%s:4,%s==========\n", __FUNCTION__, ifr->ifr_name);
            if ((strncmp(ifr->ifr_name, "lo", strlen("lo"))) &&
                (ioctl(sock, SIOCGIFFLAGS, ifr) == 0) &&
                (!(ifr->ifr_flags & IFF_LOOPBACK)) &&
                (ioctl(sock, SIOCGIFHWADDR, ifr) == 0))
            {printf("%s:5,hit==========\n", __FUNCTION__);
                strcpy(buf2, ifr->ifr_name);
                ok = 1;
                goto lhit;
            }
        }
    }
lhit :
/* yctung */
  close(sock);
  if (ok) {
    /* Convert 48 bit MAC dest to EUI-64 */
    memcpy(dest, ifr->ifr_hwaddr.sa_data, 6);
    /* Insert the converting bits in the middle */
    /* dest[3] = 0xFF;
    dest[4] = 0xFE;
    memcpy(&dest[5], &(ifr->ifr_hwaddr.sa_data[3]), 3);
    */
    pu_log(LL_DEBUG, "%s:use interface %s, %02X:%02X:%02X:%02X:%02X:%02X==========", __FUNCTION__, buf2, dest[0], dest[1], dest[2], dest[3], dest[4], dest[5]);
  } else {
#ifndef ZBAPL_USE_THREAD
    pu_log(LL_ERROR, "Couldn't read MAC dest to seed EUI64");
#endif
    return FAIL;
  }

  return SUCCESS;
}

/**
 * Copy the EUI64 into a string
 * @return 1 if we are able to capture the EUI64 else return 0
 */
error_t eui64_toString(char *dest, size_t destLen) {

    uint8_t byteAddress[EUI64_BYTES_SIZE];

    memset(dest, 0, destLen);

    if (destLen < EUI64_STRING_SIZE)
        return 0;

    /* uuid = aioxGW-${MAC_ADDRESS} = 6 + 1 + 12 = 19 */
    if (eui64_toBytes(byteAddress, sizeof(byteAddress)) != SUCCESS)
        return 0;

    /*  aioxGW --> device type 31 (voilin gateway) */
    /*  AX0 --> device type 32 (aiox test gateway) */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat="
#ifndef YC_DEVICE_TYPE_32
    snprintf(dest, destLen, "%04s%02X%02X%02X%02X%02X%02X",
#else
    snprintf(dest, destLen, "AX0-%04s%02X%02X%02X%02X%02X%02X",
#endif
             "0000",
             byteAddress[0], byteAddress[1], byteAddress[2], byteAddress[3],
             byteAddress[4], byteAddress[5]);
#pragma GCC diagnostic pop
    return 1;
}
