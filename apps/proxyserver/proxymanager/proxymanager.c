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

/**
 * This module contains all the functionality needed to start the proxy
 * @author David Moss
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <linux/limits.h>

#include "libconfigio.h"

#include "ioterror.h"
#include "iotdebug.h"
#include "proxymanager.h"
#include "proxy.h"
#include "proxyserver.h"
#include "proxyconfig.h"
#include "proxycli.h"

/**************** Private Prototypes ****************/
char *_proxymanager_getUrlFromConfigFile(char *url, int maxsize);

char *_proxymanager_getProxyActivationKeyFromConfigFile(char *buffer, int maxsize);

bool _proxymanager_useSslFromConfigFile();

char *_proxymanager_getProxySslCertificateFromConfigFile(char *buffer, int maxsize);


/**************** Public Functions ****************/
/**
 * We extract configuration values from the configuration file and use those
 * values to start the proxy which connects to the cloud services
 */
void proxymanager_startProxy() {
  char buffer[PROXY_URL_SIZE];

#ifdef PROXY_ACTIVATION
  // Pull the activation key from the config file
  proxyconfig_setActivationToken(_proxymanager_getProxyActivationKeyFromConfigFile(buffer, sizeof(buffer)));
#endif

  // Set SSL to true or false
  proxyconfig_setSsl(_proxymanager_useSslFromConfigFile());

  // Set the SSL certificate path, which may or may not exist
  proxyconfig_setCertificate(_proxymanager_getProxySslCertificateFromConfigFile(buffer, sizeof(buffer)));

  // Start the proxy with our URL
  proxy_start(_proxymanager_getUrlFromConfigFile(buffer, sizeof(buffer)));

}

/**************** Private Functions ****************/

/**
 * Read the configuration file to extract the information that makes up the
 * URL of the server.  This is also the only function I've ever written
 * in my life where it made sense to use "goto".
 *
 * @param url Destination buffer for the URL
 * @param maxsize Maximum size of the URL buffer
 * @return the URL
 */
char * _proxymanager_getUrlFromConfigFile(char *url, int maxsize) {
  bzero(url, maxsize);
  char *cwd = NULL;
  char buf[1024];
  char dataFormat[PATH_MAX];

  cwd = getcwd(buf, sizeof(buf));

  if ( cwd != NULL )
  {
      printf("current dir name: (%s)\n", cwd);
  }

  printf("proxycli_getConfigFilename: (%s)\n", proxycli_getConfigFilename());

  if(libconfigio_read(proxycli_getConfigFilename(), CONFIGIO_CLOUD_HOST, url + strlen(url), maxsize - strlen(url)) == -1) {
    printf("libconfigio_read: HOST fail\n");
    goto fail;
  }

  snprintf(url + strlen(url), maxsize - strlen(url), ":");

  printf("proxycli_getConfigFilename: (%s)\n", proxycli_getConfigFilename());
  if(libconfigio_read(proxycli_getConfigFilename(), CONFIGIO_CLOUD_PORT, url + strlen(url), maxsize - strlen(url)) == -1) {
    printf("libconfigio_read: PORT fail\n");
    goto fail;
  }

  snprintf(url + strlen(url), maxsize - strlen(url), "/");

  if(libconfigio_read(proxycli_getConfigFilename(), CONFIGIO_CLOUD_URI, url + strlen(url), maxsize - strlen(url)) == -1) {
    printf("libconfigio_read: URI fail\n");
    goto fail;
  }

  if( libconfigio_read(proxycli_getConfigFilename(), CONFIGIO_DATA_FORMAT_TOKEN_NAME, dataFormat, sizeof(dataFormat)) == -1 )
  {
      printf("libconfigio_read: DATA_FORMAT fail\n");
      goto fail;
  }

  snprintf(url + strlen(url), maxsize - strlen(url), (const char*) dataFormat);

  return url;

  fail:
      SYSLOG_ERR("Couldn't read from config file %s. You need to activate your proxy.", proxycli_getConfigFilename());
      printf("Couldn't read from config file %s. You need to activate your proxy.\n", proxycli_getConfigFilename());
      strncpy(url, DEFAULT_PROXY_URL, maxsize);
      return url;
}


/**
 * Get the activation key from our configuration file
 * @param buffer Buffer to store the activation key in
 * @param maxsize Maximum size of the buffer
 * @return The buffer
 */
char *_proxymanager_getProxyActivationKeyFromConfigFile(char *buffer, int maxsize) {
  bzero(buffer, maxsize);
  libconfigio_read(proxycli_getConfigFilename(), CONFIGIO_CLOUD_ACTIVATION_KEY, buffer, maxsize);

  if(strlen(buffer) == 0) {
    SYSLOG_ERR("For maximum security, activate your proxy with a username and password.\n");
    printf("For maximum security, activate your proxy with a username and password.\n");
    return NULL;
  }

  return buffer;
}

/**
 * Determine whether or not to use SSL from the configuration file
 * @return True if we are to use SSL, and an SSL certificate will also be needed
 */
bool _proxymanager_useSslFromConfigFile() {
  char buffer[8];
  int i;

  libconfigio_read(proxycli_getConfigFilename(), CONFIGIO_CLOUD_USE_SSL, buffer, sizeof(buffer));

  for(i = 0; buffer[i]; i++) {
    buffer[i] = tolower(buffer[i]);
  }

  return (strcmp(buffer, "true") == 0);
}

/**
 * Get the path to the SSL certificate from our configuration file
 * @param buffer Buffer to store the path in
 * @param maxsize Maximum size of the buffer
 * @return The buffer
 */
char *_proxymanager_getProxySslCertificateFromConfigFile(char *buffer, int maxsize) {
  bzero(buffer, maxsize);
  libconfigio_read(proxycli_getConfigFilename(), CONFIGIO_PROXY_SSL_CERTIFICATE_PATH, buffer, maxsize);
  return buffer;
}



