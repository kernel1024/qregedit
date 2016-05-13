#ifndef CHNTPW_H
#define CHNTPW_H

#include "ntreg.h"
#include "sam.h"

int change_user_pw(struct hive *hdesc, char *buf, int rid, int vlen, int stat, char *newp, int pl);

#endif // CHNTPW_H
