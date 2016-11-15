How to use these scripts
========================

The scripts in these folder are for testing the firmware upgrade functions, and OpenWrt package management.
Most scripts are usging the Ensemble App API to access cloud to communicate with gateway for sending command and
checking the result.


update.sh
---------

The update.sh script can send the firmware upgrade command to the target gateway using the sbox account.

1. Login
2. Get Device List
3. Select Gateway
4. Send Firmware Upgrade command

> $ ./update.sh

> Username[]: user@email.com

> Password[]: mypassword

> Login Success!!

> Device List:

> 001C4216F35F-32-354

> Send Firmware Upgrade Command to which Device ?[001C4216F35F-32-354]:

> .......

> Send Success!!




Todo List
---------
1. Firmware upgrade progress verify
1. Package testgin script
2. Firmware verify script
3. False state testing script
