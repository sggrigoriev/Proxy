//
// Created by gsg on 30/11/16.
//
// Saves initial state of Download & Upload direcories to report later to the Cloud

#ifndef PRESTO_WF_DIRS_STATE_H
#define PRESTO_WF_DIRS_STATE_H

void wf_set_download_state(int empty);   
void wf_set_upgrade_state(int empty);

int wf_was_download_empty();
int wf_was_upgrade_empty();

#endif //PRESTO_WF_DIRS_STATE_H
