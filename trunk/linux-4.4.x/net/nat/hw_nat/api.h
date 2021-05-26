/*
    Module Name:
    frame_engine.h

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ----------------------------------------------
    Name        Date            Modification logs
*/

#ifndef _HWNAT_API
#define _HWNAT_API

int GetPpeEntryIdx(struct foe_pri_key *key, struct foe_entry *entry, int del);
int get_mib_entry_idx(struct foe_pri_key *key, struct foe_entry *entry);
#endif
