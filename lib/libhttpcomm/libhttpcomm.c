#if 1
#define yc_printf_msg(format, arg...) pu_log(LL_DEBUG, format, ##arg)
#else
#define yc_printf_msg(format, arg...)
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


/*******************************************************************************
  @file           httpcomm.c
  @date           $Date$ 2010/11/17
  @version        $Revision$
  @author         Yvan Castilloux

  @brief    Code that communicates (sending and receiving) over HTTP and SSL to
            a server (remote or local)

            Relies upon libcurl v 7.19.4 libssh v ??.??.??

  To do:
*******************************************************************************/

#include <assert.h>
#include <curl/curl.h>

/*
#include <libxml/globals.h>
#include <libxml/xmlerror.h>
#include <libxml/parser.h>
#include <libxml/parserInternals.h> // only for xmlNewInputFromFile()
#include <libxml/tree.h>
#include <libxml/debugXML.h>
#include <libxml/xmlmemory.h>
*/
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <rpc/types.h>
#include <limits.h>
#include <stdbool.h>

#include "pu_logger.h"
#include "libhttpcomm.h"

struct HttpIoInfo /// structure used to store data to be sent to the server.
{
    char * buffer;
    int size;
};

static int _libhttpcomm_configureHttp(CURL * curlHandle, CURLSH * shareCurlHandle, struct curl_slist *slist, CURLoption httpMethod,
        const char *url, const char *sslCertPath, const char *authToken, http_timeout_t timeouts,
        int (*ProgressCallback) (void *clientp, double dltotal, double dlnow, double ultotal, double ulnow));

static void _libhttpcomm_closeHttp(CURL * curlHandle, struct curl_slist *slist);

/**********************************************************************************************//**
 * @brief   Called when a message has to be received from the server. this is a standard streamer
 *              if the size of the data to read, equal to size*nmemb, the function can return
 *              what was read and the function will be called again by libcurl.
 *
 * @param   ptr: where the received message resides
 * @param   size: size*nmemb == number of bytes to read
 * @param   nmemb: size*nmemb == number of bytes to read
 * @param   userp: ptr to where the message will be written -> inputted by CURLOPT_WRITEDATA call below
 *
 * @return  number of bytes that were written
 ***************************************************************************************************/
static size_t writer(void *ptr, size_t size, size_t nmemb, void *userp)
{
    struct HttpIoInfo *dataToRead = (struct HttpIoInfo *) userp;
    char *data = (char *)ptr;

    if (dataToRead == NULL || dataToRead->buffer == NULL)
    {
        pu_log(LL_ERROR, "dataToRead == NULL");
        return 0;
    }

    // keeping one byte for the null byte
    if((strlen(dataToRead->buffer)+(size * nmemb)) > (dataToRead->size - 1))
    {
#if __WORDSIZE == 64
        pu_log(LL_WARNING, "buffer overflow would result -> strlen(writeData): %lu, (size * nmemb): %lu, max size: %u",
#else
                pu_log(LL_WARNING, ("buffer overflow would result -> strlen(writeData): %u, (size * nmemb): %u, max size: %u",
#endif
                strlen(dataToRead->buffer), (size * nmemb), dataToRead->size);
        return 0;
    }
    else
    {

    }
    strncat(dataToRead->buffer, data, (size * nmemb));
    return (size * nmemb);
}

/**
 * @brief   Called when a message has to be sent to the server. this is a standard streamer
 *              if the size of the data to write is larger than size*nmemb, this function
 *              will be called several times by libcurl.
 *
 * @param   ptr: where data has to be written
 * @param   size: size*nmemb == maximum number of bytes that can be written each time
 * @param   nmemb: size*nmemb == maximum number of bytes that can be written each time
 * @param   userp: ptr to message to write -> inputted by CURLOPT_READDATA call below
 *
 * @return  number of bytes that were written
 **/
static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *userp)
{
    struct HttpIoInfo *dataToWrite = (struct HttpIoInfo *) userp;
    int dataWritten = 0;

    if (dataToWrite == NULL || dataToWrite->buffer == NULL)
    {
        pu_log(LL_ERROR, "dataToWrite == NULL");
        return 0;
    }

    if (size * nmemb < 1)
    {
        pu_log(LL_ERROR, "size * nmemb < 1");
        return 0;
    }

    if (dataToWrite->size > 0)
    {
        if (dataToWrite->size > (size * nmemb))
        {
            dataWritten = size * nmemb;
            pu_log(LL_DEBUG, "dataToWrite->size = %u is larger than size * nmemb = %u",
                    dataToWrite->size, size * nmemb);
        }
        else
        {
            dataWritten = dataToWrite->size;
        }
        memcpy (ptr, dataToWrite->buffer, dataWritten);
        dataToWrite->buffer += dataWritten;
        dataToWrite->size -= dataWritten;

        return dataWritten; /* we return 1 byte at a time! */
    }
    else
    {
        return 0;
    }

    return 0;
}

/**
 * @brief   Called when creating a curl data structure that will be shared across different
 *              http connections. DNS caching data is such data structure.
 *
 * @param   curlShHandle: pointer to a CURLSH structure that will be instantiated by curl_share_init
 *
 * @return  none
 **/
void libhttpcomm_curlShareInit(CURLSH *curlShHandle)
{
    curlShHandle = curl_share_init();

    if (curlShHandle != NULL)
    {
        if(curl_share_setopt(curlShHandle, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS) != CURLSHE_OK)
        {
            pu_log(LL_ERROR, "curl_easy_setopt CURLSHOPT_SHARE");
        }
    }else
    {
        pu_log(LL_ERROR, "curl_share_init");
    }
}

/**
 * @brief   Called when closing the shared curl data structure
 *
 * @return  none
 **/
void libhttpcomm_curlShareClose(CURLSH *curHandle)
{
    curl_share_cleanup(curHandle);
}

/**
 * @brief   Performs a HTTP Get
 *
 * @param   shareCurlHandle: Curl handle shared across connections
 * @param   url: url of the server (hostname + uri)
 * @param   sslCertPath: location of where the certificate is
 * @param   authToken: authentication token to be added in the header
 * @param   rxBuffer: ptr for storing message received by the server -> must exist
 * @param   maxRxBufferSize: max size of rxBuffer in bytes -> if 0 it is assumed that rxBuffer is of type FILE*
 * @param   timeouts: specifies connect and transfer timeouts for the connection
 * @param   ProgressCallback: function pointer that will be called every second during the connection
 *
 * @return  true for success, false for failure
 */
int libhttpcomm_getMsg(CURLSH * shareCurlHandle, const char *url, const char *sslCertPath, const char *authToken,
                char *rxBuffer, int maxRxBufferSize, http_param_t params,
                int (*ProgressCallback) (void *clientp, double dltotal, double dlnow, double ultotal, double ulnow))
{
    return libhttpcomm_sendMsg(shareCurlHandle, CURLOPT_HTTPGET, url, sslCertPath, authToken,
            NULL, 0, rxBuffer, maxRxBufferSize, params, ProgressCallback);
}
/**
 * @brief   Sends a message through HTTP to PPC servers
 *
 * @param   shareCurlHandle: Curl handle shared across connections
 * @param   httpMethod: CURLOPT_POST or CURLOPT_HTTPGET
 * @param   url: url of the server (hostname + uri)
 * @param   sslCertPath: location of where the certificate is
 * @param   authToken: authentication token to be added in the header
 * @param   msgToSendPtr: ptr to message to send. NULL if none
 * @param   msgToSendSize: send of msgToSendPtr
 * @param   rxBuffer: ptr for storing message received by the server -> must exist
 * @param   maxRxBufferSize: max size of rxBuffer in bytes -> if 0 it is assumed that rxBuffer is of type FILE*
 * @param   timeouts: specifies connect and transfer timeouts for the connection
 * @param   ProgressCallback: function pointer that will be called every second during the connection
 *
 * @return  true for success, false for failure
 */
int libhttpcomm_sendMsg(CURLSH * shareCurlHandle, CURLoption httpMethod, const char *url, const char *sslCertPath, const char *authToken,
                char *msgToSendPtr, int msgToSendSize, char *rxBuffer, int maxRxBufferSize, http_param_t params,
                int (*ProgressCallback) (void *clientp, double dltotal, double dlnow, double ultotal, double ulnow))
{yc_printf_msg("%s:in,TX:%s %s|||%s==========\n", __FUNCTION__, httpMethod == CURLOPT_POST ? "POST" : "GET", url, msgToSendPtr ? msgToSendPtr : "null");
    CURL * curlHandle = NULL;
    CURLcode curlResult;
    char tempString[4097];
    char errorBuffer[CURL_ERROR_SIZE];
    struct HttpIoInfo outBoundCommInfo;
    struct HttpIoInfo inBoundCommInfo;
    struct curl_slist *slist = NULL;
    double connectDuration = 0.0;
    double transferDuration = 0.0;
    double nameResolvingDuration = 0.0;
    long httpResponseCode = 0;
    long httpConnectCode = 0;
    long curlErrno = 0;

    assert (rxBuffer);
    assert(url);

    if (params.verbose == true)
    {
        if (msgToSendPtr != NULL)
        {
            pu_log(LL_ERROR, "httpMethod: 0x%x, url: %s, size: %d, outgoing msg: %s",
                    httpMethod, url, msgToSendSize, msgToSendPtr);
        }else
        {
            pu_log(LL_ERROR, "httpMethod: 0x%x, url: %s", httpMethod, url);
        }
    }

    rxBuffer[0] = 0;

    curlHandle = curl_easy_init();
    if (curlHandle)
    {
	if ( httpMethod == CURLOPT_POST )
	{
	    if ( msgToSendSize > 0 )
	    {
		slist = curl_slist_append(slist, "Content-Type: application/json");
		snprintf(tempString, sizeof(tempString), "Content-Length: %d", msgToSendSize);
		slist = curl_slist_append(slist, tempString);
	    }
	    else if ( msgToSendSize == 0 )
	    {
		snprintf(tempString, sizeof(tempString), "Content-Length: %d", msgToSendSize);
		if ( params.key != NULL )
		{
		    slist = curl_slist_append(slist, params.key);
		}
	    }
	}
	else if ( httpMethod  == CURLOPT_HTTPGET )
	{
	    if ( params.password != NULL )
	    {
            pu_log(LL_ERROR, "password: %s", params.password);
		slist = curl_slist_append(slist, params.password);
	    }
	    if ( params.key != NULL )
	    {
            pu_log(LL_ERROR, "key: %s", params.key);
		slist = curl_slist_append(slist, params.key);
	    }
	}

        if (_libhttpcomm_configureHttp(curlHandle, shareCurlHandle, slist, httpMethod, url,
                sslCertPath, authToken, params.timeouts, ProgressCallback) == false)
        {
            curlErrno = ENOEXEC;
            goto out;
        }

        curlResult = curl_easy_setopt(curlHandle, CURLOPT_ERRORBUFFER, errorBuffer);
        if (curlResult != CURLE_OK)
        {
            pu_log(LL_ERROR, "%s CURLOPT_ERRORBUFFER", curl_easy_strerror(curlResult));
            curlErrno = ENOEXEC;
            goto out;
        }

        // CURLOPT_READFUNCTION and CURLOPT_READDATA in this context refers to
        // data to be sent to the server... so curl will read data from us.
        if(msgToSendPtr != NULL)
        {
	    if ( msgToSendSize > 0 )
	    {
		curlResult = curl_easy_setopt(curlHandle, CURLOPT_READFUNCTION, read_callback);
		if (curlResult != CURLE_OK)
		{
            pu_log(LL_ERROR, "%s CURLOPT_READFUNCTION", curl_easy_strerror(curlResult));
		    curlErrno = ENOEXEC;
		    goto out;
		}

		//creating the curl object
		// TODO: not sure that setting a pointer to a pointer that is scoped elsewhere is correct.
		outBoundCommInfo.buffer = msgToSendPtr;
		outBoundCommInfo.size = msgToSendSize;
		/* pointer to pass to our read function */
		// here you must put the file info
		curlResult = curl_easy_setopt(curlHandle, CURLOPT_READDATA, &outBoundCommInfo);
		if (curlResult != CURLE_OK)
		{
            pu_log(LL_ERROR, "%s CURLOPT_READDATA", curl_easy_strerror(curlResult));
		    curlErrno = ENOEXEC;
		    goto out;
		}
	    }
        }

        // sets maximum size of our internal buffer
        curlResult = curl_easy_setopt(curlHandle, CURLOPT_BUFFERSIZE, maxRxBufferSize);
        if (curlResult != CURLE_OK)
        {
            pu_log(LL_ERROR, "%s CURLOPT_BUFFERSIZE", curl_easy_strerror(curlResult));
            curlErrno = ENOEXEC;
            goto out;
        }

        // CURLOPT_WRITEFUNCTION and CURLOPT_WRITEDATA in this context refers to
        // data received from the server... so curl will write data to us.
	curlResult = curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, writer);
	if (curlResult != CURLE_OK)
	{
        pu_log(LL_ERROR, "%s CURLOPT_WRITEFUNCTION", curl_easy_strerror(curlResult));
	    curlErrno = ENOEXEC;
	    goto out;
	}

	inBoundCommInfo.buffer = rxBuffer;
	inBoundCommInfo.size = maxRxBufferSize;

	curlResult = curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &inBoundCommInfo);
	if (curlResult != CURLE_OK)
	{
        pu_log(LL_ERROR, "%s CURLOPT_WRITEDATA", curl_easy_strerror(curlResult));
	    curlErrno = ENOEXEC;
	    goto out;
	}

	curlResult = curl_easy_perform(curlHandle);

        curl_easy_getinfo(curlHandle, CURLINFO_APPCONNECT_TIME, &connectDuration );
        curl_easy_getinfo(curlHandle, CURLINFO_NAMELOOKUP_TIME, &nameResolvingDuration );
        curl_easy_getinfo(curlHandle, CURLINFO_TOTAL_TIME, &transferDuration );
        curl_easy_getinfo(curlHandle, CURLINFO_RESPONSE_CODE, &httpResponseCode );
        curl_easy_getinfo(curlHandle, CURLINFO_HTTP_CONNECTCODE, &httpConnectCode );
yc_printf_msg("%s:RX:curlResult=%d,httpResponseCode=%ld,httpConnectCode=%ld|||%s==========\n", __FUNCTION__, curlResult, httpResponseCode, httpConnectCode, rxBuffer);
        if (httpResponseCode >= 300 || httpConnectCode >= 300)
        {
            if (params.verbose == true) pu_log(LL_ERROR, "HTTP error response code:%ld, connect code:%ld", httpResponseCode, httpConnectCode);
            curlErrno = EHOSTUNREACH;
            goto out;
        }

        if (curlResult != CURLE_OK)
        {
            if (curlResult != CURLE_ABORTED_BY_CALLBACK)
            {
                if (curl_easy_getinfo(curlHandle, CURLINFO_OS_ERRNO, &curlErrno) != CURLE_OK)
                {
                    curlErrno = ENOEXEC;
                    pu_log(LL_ERROR, "curl_easy_getinfo");
                }
                if (curlResult == CURLE_OPERATION_TIMEDOUT) curlErrno = ETIMEDOUT; /// time out error must be distinctive
                else if (curlErrno == 0) curlErrno = ENOEXEC; /// can't be equalt to 0 if curlResult != CURLE_OK

                if (params.verbose == true) pu_log(LL_WARNING, "%s, %s for url %s",
                        curl_easy_strerror(curlResult), strerror((int)curlErrno), url);
            }else
            {
                curlErrno = EAGAIN;
                if (params.verbose == true) pu_log(LL_DEBUG, "quitting curl transfer");
            }
            goto out;
        }else if (params.verbose == true)
        {
            if (nameResolvingDuration >= 2.0)
            {
                pu_log(LL_WARNING, "connectDuration=%.2lf, nameResolvingDuration=%.2lf, transferDuration=%.2lf, "
                        "httpConnectCode=%ld",
                        connectDuration, nameResolvingDuration, transferDuration, httpConnectCode);
            }
            else
            {
                pu_log(LL_DEBUG, "connectDuration=%.2lf, nameResolvingDuration=%.2lf, transferDuration=%.2lf, "
                        "httpConnectCode=%ld",
                        connectDuration, nameResolvingDuration, transferDuration, httpConnectCode);
            }
        }

        // the following is a special case - a time-out from the server is going to return a
        // string with 1 character in it ...
        if (strlen(rxBuffer) > 1)
        {
            /* put the result into the main buffer and return */
            if (params.verbose == true) pu_log(LL_DEBUG, "received msg length %d", strlen(rxBuffer));
            if(msgToSendPtr != NULL)
            {
              msgToSendPtr[0] = 0;
            }

        }else
        {
            pu_log(LL_DEBUG, "received time-out message from the server");
            rxBuffer[0] = '\0';
            curlErrno = EAGAIN;
            goto out;
        }
    }
    else
    {
        pu_log(LL_ERROR, "curl_easy_init failed");
        curlErrno = ENOEXEC;
    }

    out:
      _libhttpcomm_closeHttp(curlHandle, slist);
      return (int)curlErrno;
}

/**
 * @brief   Sends a message through HTTP to PPC servers
 *
 * @param   shareCurlHandle: Curl handle shared across connections
 * @param   httpMethod: CURLOPT_POST or CURLOPT_HTTPGET
 * @param   url: url of the server (hostname + uri)
 * @param   sslCertPath: location of where the certificate is
 * @param   authToken: authentication token to be added in the header
 * @param   msgToSendPtr: ptr to message to send. NULL if none
 * @param   msgToSendSize: send of msgToSendPtr
 * @param   rxBuffer: ptr for storing message received by the server -> must exist
 * @param   maxRxBufferSize: max size of rxBuffer in bytes -> if 0 it is assumed that rxBuffer is of type FILE*
 * @param   timeouts: specifies connect and transfer timeouts for the connection
 * @param   ProgressCallback: function pointer that will be called every second during the connection
 *
 * @return  true for success, false for failure
 */
int libhttpcomm_postMsg(CURLSH * shareCurlHandle, CURLoption httpMethod, const char *url, const char *sslCertPath, const char *authToken,
                char *msgToSendPtr, int msgToSendSize, char *rxBuffer, int maxRxBufferSize, http_param_t params,
                int (*ProgressCallback) (void *clientp, double dltotal, double dlnow, double ultotal, double ulnow))
{yc_printf_msg("%s:in,TX:%s %s|||%s==========\n", __FUNCTION__, httpMethod == CURLOPT_HTTPPOST ? "HTTPPOST" : httpMethod == CURLOPT_POST ? "POST" : "GET", url, msgToSendPtr ? msgToSendPtr : "null");
    CURL * curlHandle = NULL;
    CURLcode curlResult;
    char tempString[4097];
    char errorBuffer[CURL_ERROR_SIZE];
    struct HttpIoInfo outBoundCommInfo;
    struct HttpIoInfo inBoundCommInfo;
    struct curl_slist *slist = NULL;
    double connectDuration = 0.0;
    double transferDuration = 0.0;
    double nameResolvingDuration = 0.0;
    long httpResponseCode = 0;
    long httpConnectCode = 0;
    long curlErrno = 0;

    assert (rxBuffer);
    assert(url);

    if (params.verbose == true)
    {
        if (msgToSendPtr != NULL)
        {
            pu_log(LL_DEBUG, "httpMethod: 0x%x, url: %s, size: %d, outgoing msg: %s",
                    httpMethod, url, msgToSendSize, msgToSendPtr);
        }else
        {
            pu_log(LL_DEBUG, "httpMethod: 0x%x, url: %s", httpMethod, url);
        }
    }

    rxBuffer[0] = 0;

    curlHandle = curl_easy_init();

    if ( params.key != NULL )
    {
        slist = curl_slist_append(slist, params.key);
        slist = curl_slist_append(slist, "Content-Type:");
    }

    if (curlHandle)
    {
// yctung
//        if ( httpMethod == CURLOPT_POST )
        if ( httpMethod == CURLOPT_HTTPPOST )
// yctung
        {
            if ( msgToSendSize > 0 )
            {
                slist = curl_slist_append(slist, "Content-Type: application/json");
                snprintf(tempString, sizeof(tempString), "Content-Length: %d", msgToSendSize);
                slist = curl_slist_append(slist, tempString);
            }
            else if ( msgToSendSize == 0 )
            {
                snprintf(tempString, sizeof(tempString), "Content-Length: %d", msgToSendSize);
                slist = curl_slist_append(slist, tempString);
            }
        }
// yctung
        else
        if (httpMethod == CURLOPT_POST)
        {
            if (msgToSendSize > 0)
            {
                slist = curl_slist_append(slist, "Content-Type: application/json");
                snprintf(tempString, sizeof(tempString), "Content-Length: %d", msgToSendSize);
                slist = curl_slist_append(slist, tempString);
            }
            else if (msgToSendSize == 0)
            {
                slist = curl_slist_append(slist, "Content-Type: application/json");
                snprintf(tempString, sizeof(tempString), "Content-Length: %d", msgToSendSize);
                slist = curl_slist_append(slist, tempString);
            }
        }
// yctung
        else if ( httpMethod  == CURLOPT_HTTPGET )
        {
            if ( params.password != NULL )
            {
                pu_log(LL_ERROR, "password: %s", params.password);
                slist = curl_slist_append(slist, params.password);
            }
            if ( params.key != NULL )
            {
                pu_log(LL_ERROR, "key: %s", params.key);
                slist = curl_slist_append(slist, params.key);
            }
        }

        if (_libhttpcomm_configureHttp(curlHandle, shareCurlHandle, slist, httpMethod, url,
                sslCertPath, authToken, params.timeouts, ProgressCallback) == false)
        {
            curlErrno = ENOEXEC;
            goto out;
        }

        curlResult = curl_easy_setopt(curlHandle, CURLOPT_ERRORBUFFER, errorBuffer);
        if (curlResult != CURLE_OK)
        {
            pu_log(LL_ERROR, "%s CURLOPT_ERRORBUFFER", curl_easy_strerror(curlResult));
            curlErrno = ENOEXEC;
            goto out;
        }

        // CURLOPT_READFUNCTION and CURLOPT_READDATA in this context refers to
        // data to be sent to the server... so curl will read data from us.
        if(msgToSendPtr != NULL)
        {
            if ( msgToSendSize > 0 )
            {
                curlResult = curl_easy_setopt(curlHandle, CURLOPT_READFUNCTION, read_callback);
                if (curlResult != CURLE_OK)
                {
                    pu_log(LL_ERROR, "%s CURLOPT_READFUNCTION", curl_easy_strerror(curlResult));
                    curlErrno = ENOEXEC;
                    goto out;
                }

                //creating the curl object
                // TODO: not sure that setting a pointer to a pointer that is scoped elsewhere is correct.
                outBoundCommInfo.buffer = msgToSendPtr;
                outBoundCommInfo.size = msgToSendSize;
                /* pointer to pass to our read function */
                // here you must put the file info
                curlResult = curl_easy_setopt(curlHandle, CURLOPT_READDATA, &outBoundCommInfo);
                if (curlResult != CURLE_OK)
                {
                    pu_log(LL_ERROR, "%s CURLOPT_READDATA", curl_easy_strerror(curlResult));
                    curlErrno = ENOEXEC;
                    goto out;
                }
            }
        }

        // sets maximum size of our internal buffer
        curlResult = curl_easy_setopt(curlHandle, CURLOPT_BUFFERSIZE, maxRxBufferSize);
        if (curlResult != CURLE_OK)
        {
            pu_log(LL_ERROR, "%s CURLOPT_BUFFERSIZE", curl_easy_strerror(curlResult));
            curlErrno = ENOEXEC;
            goto out;
        }

        // CURLOPT_WRITEFUNCTION and CURLOPT_WRITEDATA in this context refers to
        // data received from the server... so curl will write data to us.
        curlResult = curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, writer);
        if (curlResult != CURLE_OK)
        {
            pu_log(LL_ERROR, "%s CURLOPT_WRITEFUNCTION", curl_easy_strerror(curlResult));
            curlErrno = ENOEXEC;
            goto out;
        }

        inBoundCommInfo.buffer = rxBuffer;
        inBoundCommInfo.size = maxRxBufferSize;

        curlResult = curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &inBoundCommInfo);
        if (curlResult != CURLE_OK)
        {
            pu_log(LL_ERROR, "%s CURLOPT_WRITEDATA", curl_easy_strerror(curlResult));
            curlErrno = ENOEXEC;
            goto out;
        }

        curlResult = curl_easy_setopt(curlHandle, CURLOPT_POSTFIELDS, NULL);
        if (curlResult != CURLE_OK)
        {
            pu_log(LL_ERROR, "%s CURLOPT_POSTFIELDS", curl_easy_strerror(curlResult));
            curlErrno = ENOEXEC;
            goto out;
        }

        curlResult = curl_easy_perform(curlHandle);

        curl_easy_getinfo(curlHandle, CURLINFO_APPCONNECT_TIME, &connectDuration );
        curl_easy_getinfo(curlHandle, CURLINFO_NAMELOOKUP_TIME, &nameResolvingDuration );
        curl_easy_getinfo(curlHandle, CURLINFO_TOTAL_TIME, &transferDuration );
        curl_easy_getinfo(curlHandle, CURLINFO_RESPONSE_CODE, &httpResponseCode );
        curl_easy_getinfo(curlHandle, CURLINFO_HTTP_CONNECTCODE, &httpConnectCode );
yc_printf_msg("%s:RX:curlResult=%d,httpResponseCode=%ld,httpConnectCode=%ld|||%s==========\n", __FUNCTION__, curlResult, httpResponseCode, httpConnectCode, rxBuffer);
        if (httpResponseCode >= 300 || httpConnectCode >= 300)
        {
            if (params.verbose == true) pu_log(LL_ERROR, "HTTP error response code:%ld, connect code:%ld", httpResponseCode, httpConnectCode);
            curlErrno = EHOSTUNREACH;
            goto out;
        }

        if (curlResult != CURLE_OK)
        {
            if (curlResult != CURLE_ABORTED_BY_CALLBACK)
            {
                if (curl_easy_getinfo(curlHandle, CURLINFO_OS_ERRNO, &curlErrno) != CURLE_OK)
                {
                    curlErrno = ENOEXEC;
                    pu_log(LL_ERROR, "curl_easy_getinfo");
                }
                if (curlResult == CURLE_OPERATION_TIMEDOUT) curlErrno = ETIMEDOUT; /// time out error must be distinctive
                else if (curlErrno == 0) curlErrno = ENOEXEC; /// can't be equalt to 0 if curlResult != CURLE_OK

                if (params.verbose == true) pu_log(LL_WARNING, "%s, %s for url %s",
                        curl_easy_strerror(curlResult), strerror((int)curlErrno), url);
            }else
            {
                curlErrno = EAGAIN;
                if (params.verbose == true) pu_log(LL_DEBUG, "quitting curl transfer");
            }
            goto out;
        }else if (params.verbose == true)
        {
            if (nameResolvingDuration >= 2.0)
            {
                pu_log(LL_WARNING, "connectDuration=%.2lf, nameResolvingDuration=%.2lf, transferDuration=%.2lf, "
                        "httpConnectCode=%ld",
                        connectDuration, nameResolvingDuration, transferDuration, httpConnectCode);
            }
            else
            {
                pu_log(LL_DEBUG, "connectDuration=%.2lf, nameResolvingDuration=%.2lf, transferDuration=%.2lf, "
                        "httpConnectCode=%ld",
                        connectDuration, nameResolvingDuration, transferDuration, httpConnectCode);
            }
        }

        // the following is a special case - a time-out from the server is going to return a
        // string with 1 character in it ...
        if (strlen(rxBuffer) > 1)
        {
            /* put the result into the main buffer and return */
            if (params.verbose == true) pu_log(LL_DEBUG, "received msg length %d", strlen(rxBuffer));
            if(msgToSendPtr != NULL && msgToSendSize > 0 )
            {
              msgToSendPtr[0] = 0;
            }

        }else
        {
            pu_log(LL_DEBUG, "received time-out message from the server");
            rxBuffer[0] = '\0';
            curlErrno = EAGAIN;
            goto out;
        }
    }
    else
    {
        pu_log(LL_ERROR, "curl_easy_init failed");
        curlErrno = ENOEXEC;
    }

    out:
      _libhttpcomm_closeHttp(curlHandle, slist);
      return (int)curlErrno;
}

/**
 * @brief   Get a file through HTTP from PPC servers
 *
 * @param   shareCurlHandle: Curl handle shared across connections
 * @param   url: url of the server (hostname + uri)
 * @param   sslCertPath: location of where the certificate is
 * @param   authToken: authentication token to be added in the header
 * @param   rxFile: FILE ptr to the incoming file
 * @param   maxRxFileSize: max size of the file to receive
 * @param   timeouts: specifies connect and transfer timeouts for the connection
 * @param   ProgressCallback: function pointer that will be called every second during the connection
 *
 * @return  true for success, false for failure
 */
int libhttpcomm_getFile(CURLSH * shareCurlHandle, const char *url, const char *sslCertPath, const char *authToken,
                FILE *rxFile, int maxRxFileSize, http_timeout_t timeouts,
                int (*ProgressCallback) (void *clientp, double dltotal, double dlnow, double ultotal, double ulnow))
{
    CURL * curlHandle;
    CURLcode curlResult;
    char errorBuffer[CURL_ERROR_SIZE];
    struct curl_slist *slist = NULL;
    double connectDuration = 0.0;
    double transferDuration = 0.0;
    double nameResolvingDuration = 0.0;
    long curlErrno = 0;
    long httpResponseCode = 0;
    long httpConnectCode = 0;
    bool_t retVal = true;

    assert (rxFile);
    assert (url);

    pu_log(LL_DEBUG, "url: %s", url);

    curlHandle = curl_easy_init();
    if (curlHandle)
    {
        if (_libhttpcomm_configureHttp(curlHandle, shareCurlHandle, slist, CURLOPT_HTTPGET, url,
                sslCertPath, authToken, timeouts, ProgressCallback) == false)
        {
            retVal = false;
            goto out;
        }

        curlResult = curl_easy_setopt(curlHandle, CURLOPT_ERRORBUFFER, errorBuffer);
        if (curlResult != CURLE_OK)
        {
            pu_log(LL_ERROR, "%s CURLOPT_ERRORBUFFER", curl_easy_strerror(curlResult));
            retVal = false;
            goto out;
        }

        // sets maximum size of our internal buffer
        curlResult = curl_easy_setopt(curlHandle, CURLOPT_BUFFERSIZE, maxRxFileSize);
        if (curlResult != CURLE_OK)
        {
            pu_log(LL_ERROR, "%s CURLOPT_BUFFERSIZE", curl_easy_strerror(curlResult));
            retVal = false;
            goto out;
        }

        curlResult = curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, rxFile);
        if (curlResult != CURLE_OK)
        {
            pu_log(LL_ERROR, "%s CURLOPT_WRITEDATA", curl_easy_strerror(curlResult));
            retVal = false;
            goto out;
        }

        curlResult = curl_easy_perform(curlHandle);

        curl_easy_getinfo(curlHandle, CURLINFO_APPCONNECT_TIME, &connectDuration );
        curl_easy_getinfo(curlHandle, CURLINFO_NAMELOOKUP_TIME, &nameResolvingDuration );
        curl_easy_getinfo(curlHandle, CURLINFO_TOTAL_TIME, &transferDuration );
        curl_easy_getinfo(curlHandle, CURLINFO_RESPONSE_CODE, &httpResponseCode );
        curl_easy_getinfo(curlHandle, CURLINFO_HTTP_CONNECTCODE, &httpConnectCode );

        if (httpResponseCode >= 300 || httpConnectCode >= 300)
        {
            retVal = false;
            goto out;
        }

        if (nameResolvingDuration >= 2.0)
        {
            pu_log(LL_WARNING, "connectDuration=%.2lf, nameResolvingDuration=%.2lf, transferDuration=%.2lf",
                    connectDuration, nameResolvingDuration, transferDuration);
        }
        else
        {
            pu_log(LL_DEBUG, "connectDuration=%.2lf, nameResolvingDuration=%.2lf, transferDuration=%.2lf",
                    connectDuration, nameResolvingDuration, transferDuration);
        }

        if (curlResult != CURLE_OK)
        {
            if (curlResult != CURLE_ABORTED_BY_CALLBACK)
            {
                if (curl_easy_getinfo(curlHandle, CURLINFO_OS_ERRNO, &curlErrno) != CURLE_OK)
                {
                    curlErrno = 0;
                    pu_log(LL_ERROR, "curl_easy_getinfo");
                }
                pu_log(LL_ERROR, "curl_easy_perform: %s, %s for url %s",
                        curl_easy_strerror(curlResult), strerror((int)curlErrno), url);
            }else
            {
                pu_log(LL_DEBUG, "quitting curl transfer");
            }
            retVal = false;
            goto out;
        }
    }
    else
    {
        pu_log(LL_ERROR, "curl_easy_init failed");
        retVal = false;
    }

    out:
      _libhttpcomm_closeHttp(curlHandle, slist);
      return retVal;
}

/**
 * @brief   Sends a file through HTTP to a remote computer
 *
 * @param   url: url of the server (hostname + uri)
 * @param   sslCertPath: location of where the certificate is
 * @param   authToken: authentication token to be added in the header
 * @param   fileName: ptr to message to send. NULL if none
 * @param   rxBuffer: ptr for storing message received by the server -> must exist
 * @param   maxRxBufferSize: max size of rxBuffer in bytes.
 * @param   timeouts: specifies connect and transfer timeouts for the connection
 *
 * @return  true for success, false for failure
 */
int libhttpcomm_sendFile(const char *url, const char *sslCertPath, const char *authToken,
                char *fileName, char *rxBuffer, int maxRxBufferSize, http_timeout_t timeouts)
{
    CURL * curlHandle;
    CURLcode curlResult;
    FILE *file = NULL;
    char tempString[4097];
    char errorBuffer[CURL_ERROR_SIZE];
    int fileSize = 0;
    struct HttpIoInfo inBoundCommInfo;
    struct curl_slist *slist = NULL;
    struct stat fileStats;
    double connectDuration = 0.0;
    double transferDuration = 0.0;
    double nameResolvingDuration = 0.0;
    long httpResponseCode = 0;
    long httpConnectCode = 0;
    long curlErrno = 0;
    bool_t retVal = true;

    assert (url);
    assert (fileName);

    curlHandle = curl_easy_init();
    if (curlHandle)
    {
        //creating the curl object
        // TODO: not sure that setting a pointer to a pointer that is scoped elsewhere is correct.
        file = fopen(fileName, "r");
        if (file == NULL)
        {
            pu_log(LL_ERROR, "%s opening %s", strerror(errno), fileName);
            retVal = false;
            goto out;
        }

        if (stat (fileName, &fileStats) >= 0)
        {
            fileSize = (int)fileStats.st_size;
        }else
        {
            pu_log(LL_ERROR, "stats returned %s", strerror(errno));
        }

        slist = curl_slist_append(slist, "Content-Type: application/octet-stream");
        snprintf(tempString, sizeof(tempString), "Content-Length: %d", fileSize);
        slist = curl_slist_append(slist, tempString);

        pu_log(LL_DEBUG, "fileName: %s, url: %s, fileSize = %d", fileName, url, fileSize);

        if (_libhttpcomm_configureHttp(curlHandle, NULL, slist, CURLOPT_POST, url,
                sslCertPath, authToken, timeouts, NULL) == false)
        {
            retVal = false;
            goto out;
        }

        curlResult = curl_easy_setopt(curlHandle, CURLOPT_ERRORBUFFER, errorBuffer);
        if (curlResult != CURLE_OK)
        {
            pu_log(LL_ERROR, "%s CURLOPT_ERRORBUFFER", curl_easy_strerror(curlResult));
            retVal = false;
            goto out;
        }

        /* pointer to pass to our read function */
        // here you must put the file info
        curlResult = curl_easy_setopt(curlHandle, CURLOPT_READDATA, file);
        if (curlResult != CURLE_OK)
        {
            pu_log(LL_ERROR, "%s CURLOPT_READDATA", curl_easy_strerror(curlResult));
            retVal = false;
            goto out;
        }

        // sets maximum size of our internal buffer
        curlResult = curl_easy_setopt(curlHandle, CURLOPT_BUFFERSIZE, maxRxBufferSize);
        if (curlResult != CURLE_OK)
        {
            pu_log(LL_ERROR, "%s CURLOPT_BUFFERSIZE", curl_easy_strerror(curlResult));
            retVal = false;
            goto out;
        }

        // CURLOPT_WRITEFUNCTION and CURLOPT_WRITEDATA in this context refers to
        // data received from the server... so curl will write data to us.
        curlResult = curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, writer);
        if (curlResult != CURLE_OK)
        {
            pu_log(LL_ERROR, "%s CURLOPT_WRITEFUNCTION", curl_easy_strerror(curlResult));
            retVal = false;
            goto out;
        }

        inBoundCommInfo.buffer = rxBuffer;
        inBoundCommInfo.size = maxRxBufferSize;

        curlResult = curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &inBoundCommInfo);
        if (curlResult != CURLE_OK)
        {
            pu_log(LL_ERROR, "%s CURLOPT_WRITEDATA", curl_easy_strerror(curlResult));
            retVal = false;
            goto out;
        }

        curlResult = curl_easy_perform(curlHandle);

        curl_easy_getinfo(curlHandle, CURLINFO_APPCONNECT_TIME, &connectDuration );
        curl_easy_getinfo(curlHandle, CURLINFO_NAMELOOKUP_TIME, &nameResolvingDuration );
        curl_easy_getinfo(curlHandle, CURLINFO_TOTAL_TIME, &transferDuration );
        curl_easy_getinfo(curlHandle, CURLINFO_RESPONSE_CODE, &httpResponseCode );
        curl_easy_getinfo(curlHandle, CURLINFO_HTTP_CONNECTCODE, &httpConnectCode );

        if (nameResolvingDuration >= 2.0)
        {
            pu_log(LL_WARNING, "connectDuration=%.2lf, nameResolvingDuration=%.2lf, transferDuration=%.2lf, "
                    "httpConnectCode=%ld",
                    connectDuration, nameResolvingDuration, transferDuration, httpConnectCode);
        }
        else
        {
            pu_log(LL_DEBUG, "connectDuration=%.2lf, nameResolvingDuration=%.2lf, transferDuration=%.2lf, "
                    "httpConnectCode=%ld",
                    connectDuration, nameResolvingDuration, transferDuration, httpConnectCode);
        }

        if (httpResponseCode >= 300 || httpConnectCode >= 300)
        {
            pu_log(LL_ERROR, "HTTP error response code: %ld, connect code: %ld", httpResponseCode, httpConnectCode);
            retVal = false;
            goto out;
        }

        if (curlResult != CURLE_OK)
        {
            if (curlResult != CURLE_ABORTED_BY_CALLBACK)
            {
                if (curl_easy_getinfo(curlHandle, CURLINFO_OS_ERRNO, &curlErrno) != CURLE_OK)
                {
                    curlErrno = 0;
                    pu_log(LL_ERROR, "curl_easy_getinfo");
                }
                pu_log(LL_ERROR, "curl_easy_perform: %s, %s for url %s",
                        curl_easy_strerror(curlResult), strerror((int)curlErrno), url);
            }else
            {
                pu_log(LL_DEBUG, "quitting curl transfer");
            }
            retVal = false;
            goto out;
        }

        // the following is a special case - a time-out from the server is going to return a
        // string with 1 character in it ...
        if (strlen(rxBuffer) > 1)
        {
            /* put the result into the main buffer and return */
            pu_log(LL_DEBUG, "received msg length %d", strlen(rxBuffer));
        }else
        {
            if (strlen(rxBuffer) == 1)
            {
                pu_log(LL_DEBUG, "received time-out message from the server");
            }
            rxBuffer[0] = '\0';
            goto out;
        }
    }
    else
    {
        pu_log(LL_ERROR, "curl_easy_init failed");
        retVal = false;
    }

    out:
      fclose(file);
      _libhttpcomm_closeHttp(curlHandle, slist);
      return retVal;
}

/**
 * @brief   This function configures a HTTP connection with PPC standard parameters
 *
 * @param   curlHandle: curl handle to configure
 * @param   shareCurlHandle: curl handle shared across connections, used for DNS caching.
 * @param   slist: linked list that stores the HTTP Header
 * @param   httpMethod: HTTP RESTFUL method to use (GET, POST, DELETE PUT)
 * @param   url: url of the server (hostname + uri)
 * @param   sslCertPath: location of where the certificate is
 * @param   authToken: authentication token to be added in the header
 * @param   fileName: ptr to message to send. NULL if none
 * @param   rxBuffer: ptr for storing message received by the server -> must exist
 * @param   maxRxBufferSize: max size of rxBuffer in bytes.
 * @param   timeouts: specifies connect and transfer timeouts for the connection
 * @param   ProgressCallback: Function to be called periodically by curl every second
 *              while the transfer is underway
 *
 * @return  true for success, false for failure
 */
int _libhttpcomm_configureHttp(CURL * curlHandle, CURLSH * shareCurlHandle, struct curl_slist *slist, CURLoption httpMethod,
        const char *url, const char *sslCertPath, const char *authToken, http_timeout_t timeouts,
        int (*ProgressCallback) (void *clientp, double dltotal, double dlnow, double ultotal, double ulnow))
{
    int retVal = true;
    CURLcode curlResult;
    char tempString[256];

    if (shareCurlHandle != NULL)
    {
        curlResult = curl_easy_setopt(curlHandle, CURLOPT_SHARE, shareCurlHandle);
        if (curlResult != CURLE_OK)
        {
            pu_log(LL_ERROR, "%s CURLOPT_SHARE", curl_easy_strerror(curlResult));
            retVal = false;
            goto out;
        }
    }

    curlResult = curl_easy_setopt(curlHandle, CURLOPT_DNS_CACHE_TIMEOUT, HTTPCOMM_DEFAULT_DNS_CACHING_TIMEOUT_SEC);
    if (curlResult != CURLE_OK)
    {
        pu_log(LL_ERROR, "%s CURLOPT_FORBID_REUSE", curl_easy_strerror(curlResult));
        retVal = false;
        goto out;
    }

    // no signals when using c-ares
    curlResult = curl_easy_setopt(curlHandle, CURLOPT_NOSIGNAL, 1L);
    if (curlResult != CURLE_OK)
    {
        pu_log(LL_ERROR, "%s CURLOPT_NOSIGNAL", curl_easy_strerror(curlResult));
        retVal = false;
        goto out;
    }

    //making sure that this is a new connection
    curlResult = curl_easy_setopt(curlHandle, CURLOPT_FORBID_REUSE, false);
    if (curlResult != CURLE_OK)
    {
        pu_log(LL_ERROR, "%s CURLOPT_FORBID_REUSE", curl_easy_strerror(curlResult));
        retVal = false;
        goto out;
    }

#if 0
    //TODO:
    curlResult = curl_easy_setopt(curlHandle, CURLOPT_VERBOSE, true);
    if (curlResult != CURLE_OK)
    {
        pu_log(LL_ERROR, "%s CURLOPT_VERBOSE", curl_easy_strerror(curlResult));
        retVal = false;
        goto out;
    }
#endif

    curlResult = curl_easy_setopt(curlHandle, CURLOPT_FRESH_CONNECT, false);
    if (curlResult != CURLE_OK)
    {
        pu_log(LL_ERROR, "%s CURLOPT_FRESH_CONNECT", curl_easy_strerror(curlResult));
        retVal = false;
        goto out;
    }

    // for safe multi-threaded operation
    // now only used in one thread. to be able to disable signal -> c-ares library has be used
    // for name resolving
    curlResult = curl_easy_setopt(curlHandle, CURLOPT_NOSIGNAL, 1);
    if (curlResult != CURLE_OK)
    {
        pu_log(LL_ERROR, "%s CURLOPT_NOSIGNAL", curl_easy_strerror(curlResult));
        retVal = false;
        goto out;
    }

    // less than 30 seconds is not reliable
    if (timeouts.connectTimeout != 0)
    {
        curlResult = curl_easy_setopt(curlHandle, CURLOPT_CONNECTTIMEOUT, timeouts.connectTimeout);
        if (curlResult != CURLE_OK)
        {
            pu_log(LL_ERROR, "%s CURLOPT_CONNECTTIMEOUT", curl_easy_strerror(curlResult));
            retVal = false;
            goto out;
        }
    }

    if (timeouts.transferTimeout != 0)
    {
        curlResult = curl_easy_setopt(curlHandle, CURLOPT_TIMEOUT, timeouts.transferTimeout);
        if (curlResult != CURLE_OK)
        {
            pu_log(LL_ERROR, "%s CURLOPT_TIMEOUT", curl_easy_strerror(curlResult));
            retVal = false;
            goto out;
        }
    }

    // set progress meter so that we can read if the push pipe is getting full
    if(ProgressCallback != NULL)
    {
        curlResult = curl_easy_setopt(curlHandle, CURLOPT_NOPROGRESS, 0);
        if (curlResult != CURLE_OK)
        {
            pu_log(LL_ERROR, "%s CURLOPT_NOPROGRESS", curl_easy_strerror(curlResult));
            retVal = false;
            goto out;
        }

        curlResult = curl_easy_setopt(curlHandle, CURLOPT_PROGRESSFUNCTION, ProgressCallback);
        if (curlResult != CURLE_OK)
        {
            pu_log(LL_ERROR, "%s CURLOPT_PROGRESSFUNCTION", curl_easy_strerror(curlResult));
            retVal = false;
            goto out;
        }
    }

    if (sslCertPath != NULL)
    {
        curlResult = curl_easy_setopt(curlHandle, CURLOPT_SSL_VERIFYPEER, 0L);
        if (curlResult != CURLE_OK)
        {
            pu_log(LL_ERROR, "%s CURLOPT_SSL_VERIFYPEER", curl_easy_strerror(curlResult));
            retVal = false;
            goto out;
        }

        curlResult = curl_easy_setopt(curlHandle, CURLOPT_CAPATH, sslCertPath);
        if (curlResult != CURLE_OK)
        {
            pu_log(LL_ERROR, "%s CURLOPT_CAPATH", curl_easy_strerror(curlResult));
            retVal = false;
            goto out;
        }
    }

    curlResult = curl_easy_setopt(curlHandle, CURLOPT_URL, url);
    if (curlResult != CURLE_OK)
    {
        pu_log(LL_ERROR, "%s CURLOPT_URL", curl_easy_strerror(curlResult));
        retVal = false;
        goto out;
    }

    curlResult = curl_easy_setopt(curlHandle, httpMethod, 1L);
    if (curlResult != CURLE_OK)
    {
        pu_log(LL_ERROR, "%s CURLOPT_POST", curl_easy_strerror(curlResult));
        retVal = false;
        goto out;
    }

    // all is ready, so do it -> set the http header
    //generic http header
    slist = curl_slist_append(slist, "User-Agent: IOT Proxy");

    if (authToken != NULL)
    {
        snprintf(tempString, sizeof(tempString), "PPCAuthorization: esp token=%s", authToken);
        slist = curl_slist_append(slist, tempString);
    }

    //TODO: may want to add a paramter for an optional header line
    curlResult = curl_easy_setopt(curlHandle, CURLOPT_HTTPHEADER, slist);
    if (curlResult != CURLE_OK)
    {
        pu_log(LL_ERROR, "%s CURLOPT_HTTPHEADER", curl_easy_strerror(curlResult));
        retVal = false;
        goto out;
    }

    out:
        return retVal;
}

/**
 * @brief   This function cleans up connection parameters after a transfer has been made.
 *
 * @param   curlHandle: curl handle to close/clean
 * @param   slist: linked list used for the header to close/clean
 *
 * @return  None
 */
void _libhttpcomm_closeHttp(CURL * curlHandle, struct curl_slist *slist)
{
    curl_slist_free_all(slist); /* free the list again */
    curl_easy_cleanup(curlHandle);
}
