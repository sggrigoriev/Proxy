#ifndef UPGRADE_H
#define UPGRADE_H

// For Firmware Upgrade
typedef struct upgrade_t {
  char *action;
  char *url;
  unsigned long size;
  char *md5;
  char *name;
} upgrade_t;

int system_upgrade ( upgrade_t *upgrade );
#endif
