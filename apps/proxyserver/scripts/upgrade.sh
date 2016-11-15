#!/bin/bash

USERNAME=yihua1218@gmail.com
PASSWORD=********

if [ ! -e .key ] ; then
    echo -n "Username[$USERNAME]: "
    read NEW_USERNAME
    echo -n "Password[$PASSWORD]: "
    read NEW_PASSWORD

    HAS_NEW_USERNAME=`echo $NEW_USERNAME | wc -w`
    HAS_NEW_PASSWORD=`echo $NEW_PASSWORD | wc -w`

    if [ $HAS_NEW_USERNAME -ne 0 ] ; then
	USERNAME=$NEW_USERNAME
    fi

    if [ $HAS_NEW_PASSWORD -ne 0 ] ; then
	PASSWORD=$NEW_PASSWORD
    fi

    curl \
	--header "Content-Type: application/json" \
	--header "PASSWORD: $PASSWORD" \
	'https://sbox.presencepro.com/cloud/json/login?username='$USERNAME > .login.out 2> .login.err.out

    RESULT_CODE=`jq '.resultCode' .login.out`

    if [ $RESULT_CODE -eq 0 ] ; then
	echo Login Success!!
    else
	echo Login Fail!!
	exit 1
    fi

    KEY=`jq '.key' .login.out`
    echo $KEY > .key
else
    KEY=`cat .key | cut -b 2-65`
fi

curl \
    --header "API_KEY:$KEY" \
    'https://sbox.presencepro.com/cloud/json/devices' > .devices.out 2> .devices.err.out

RESULT_CODE=`jq '.resultCode' .devices.out`

if [ $RESULT_CODE -ne 0 ] ; then
    echo Get Device List Fail!!
    exit 1
fi

LENGTH=`jq '.devices | length' .devices.out`

for ((i=0;i<$LENGTH;i++)) ; do
    TYPE=`jq .devices[$i].type .devices.out`
    if [ $TYPE -eq 32 ] ; then
	DEVICE_ID=`jq .devices[$i].id .devices.out | sed 's/^.//' |sed 's/.$//' `
	echo $DEVICE_ID
	if [ `echo $FIRST_DEVICE_ID | wc -w` -eq 0 ] ; then
	    FIRST_DEVICE_ID=$DEVICE_ID
	fi
    fi
done

echo -n "Send Firmware Upgrade Command to which Device ? [$FIRST_DEVICE_ID]: "
read DEVICE_ID
HAS_NEW_DEVICE_ID=`echo $DEVICE_ID | wc -w`

if [ $HAS_NEW_DEVICE_ID -eq 0 ] ; then
    DEVICE_ID=$FIRST_DEVICE_ID
else
    DEVICE_ID=$DEVICE_ID
fi

echo deviceId:"$DEVICE_ID"


# http://nchc.dl.sourceforge.net/project/openwrt-presto/Firmware/openwrt-ramips-mt7688-LinkIt7688-squashfs-sysupgrade.bin
# https://sourceforge.net/projects/openwrt-presto/files/Firmware/openwrt-ramips-mt7688-LinkIt7688-squashfs-sysupgrade.bin

curl \
    --request PUT \
    --header "Content-Type: application/json" \
    --header "API_KEY: $KEY" \
    --data-binary "{
	\"parameters\": {
	    \"param\": [
		{
		    \"name\": \"ppm.action\",
		    \"content\": \"FIRMWARE_UPGRADE\"
		},
		{
		    \"name\": \"ppm.url\",
		    \"content\": \"http://nchc.dl.sourceforge.net/project/openwrt-presto/Firmware/openwrt-ramips-mt7688-LinkIt7688-squashfs-sysupgrade.bin\"
		},
		{
		    \"name\": \"ppm.size\",
		    \"content\": \"22020100\"
		},
		{
		    \"name\": \"ppm.md5\",
		    \"content\": \"0c5d11db72298b8d227f4a52d92dee84\"
		}
	    ]
	  }
	}" \
	"https://sbox.presencepro.com/cloud/json/devices/$DEVICE_ID/parameters"


