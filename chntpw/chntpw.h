#ifndef CHNTPW_H
#define CHNTPW_H

#include "ntreg.h"
#include "sam.h"

int promote_user(struct hive *hdesc, int rid);
int change_user_pw(struct hive *hdesc, char *buf, int rid, int vlen, int stat, char *newp, int pl);
int get_user_rid(struct hive *hdesc, char *username);
struct keyval* get_user_v_ptr(struct hive *hdesc, int rid);


#endif // CHNTPW_H
