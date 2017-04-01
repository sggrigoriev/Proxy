/*
* Created by gsg on 31/03/17.
 *
 * notify the cloud by old URL if the main loud URL was changed
 *
*/

#ifndef PRESTO_PT_CHANGE_CLOUD_NOTIFICATION_H
#define PRESTO_PT_CHANGE_CLOUD_NOTIFICATION_H

/**********************************************************************
 * Start thread. If the thread was started before it will stop it and restart
 * @return  - 1 if OK, 0 if not
 */
int pt_start_change_cloud_notification();

#endif /* PRESTO_PT_CHANGE_CLOUD_NOTIFICATION_H */
