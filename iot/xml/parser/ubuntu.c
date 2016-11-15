/*
 *  Copyright 2016 People Power Company
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

/**
 * This module is responsible for parsing firmware upgrade commands from the server
 * @author Hoebus Liang
 */

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdbool.h>
#include <rpc/types.h>
#include <stdio.h>

#include <libxml/parser.h>

#include "iotapi.h"
#include "ioterror.h"
#include "iotdebug.h"
#include "iotcommandlisteners.h"
#include "iotparser.h"
#include "eui64.h"
#include "upgrade.h"

int system_upgrade ( upgrade_t *upgrade )
{
    char command[256];
    int ret = 0;
    if ( strcmp(upgrade->action, "FIRMWARE_UPGRADE") == 0 )
    {
	ret = chdir("/tmp");
	snprintf(command, sizeof(command), "/usr/bin/wget %s", upgrade->url);
	ret = system(command);
	snprintf(command, sizeof(command), "/usr/bin/md5sum `/usr/bin/basename %s` | /usr/bin/cut -b1-32 > `/usr/bin/basename %s`.md5", upgrade->url, upgrade->url);
	ret = system(command);
	snprintf(command, sizeof(command), "/bin/echo %s > `/usr/bin/basename %s`.md5.check", upgrade->md5, upgrade->url);
	ret = system(command);
	snprintf(command, sizeof(command), "/usr/bin/diff `/usr/bin/basename %s`.md5 `/usr/bin/basename %s`.md5.check", upgrade->url, upgrade->url);
	if ( system(command) == 0 )
	{
	    snprintf(command, sizeof(command), "/sbin/sysupgrade `/usr/bin/basename %s`", upgrade->url);
	    ret = system(command);
	}
    }
    else if ( strcmp(upgrade->action, "INSTALL") == 0 )
    {
	snprintf(command, sizeof(command), "/bin/opkg install %s", upgrade->name);
	ret = system(command);
    }
    else if ( strcmp(upgrade->action, "UNINSTALL") == 0 )
    {
	snprintf(command, sizeof(command), "/bin/opkg remove %s", upgrade->name);
	ret = system(command);
    }
    else if ( strcmp(upgrade->action, "UPDATE") == 0 )
    {
	snprintf(command, sizeof(command), "/bin/opkg update %s", upgrade->name);
	ret = system(command);
    }
    else if ( strcmp(upgrade->action, "REBOOT") == 0 )
    {
	ret = system("/sbin/reboot");
    }

    return ret;
}
