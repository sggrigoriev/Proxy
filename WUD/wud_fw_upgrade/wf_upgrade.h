//
// Created by gsg on 01/12/16.
//

#ifndef PRESTO_WF_UPGRADE_H
#define PRESTO_WF_UPGRADE_H

//Check downloaded file
//Return 1 if OK, 0 if error

#define WF_STATE_EMPTY 0
#define WF_STATE_BUZY 1


int wf_check_file(const char* check_sum, const char* path, const char* file_name);

void wf_set_download_state(int empty);
void wf_set_upgrade_state(int empty);

int wf_was_download_empty();
int wf_was_upgrade_empty();

#endif //PRESTO_WF_UPGRADE_H
