#include <QString>
#include <QByteArray>
#include <sys/types.h>
#include <sys/stat.h>

#include "chntpw/ntreg.h"
#include "functions.h"

/* Extended compare for key names with encoding
 * s1 - raw src keyname, in utf8 or Latin1 encoding
 * s2 - dst key struct. may contains encoded or unencoded key names
 */

int qf_strncasecmp(const char *s1, struct nk_key *s2)
{
    int res = 0;
    QString qs1 = QString::fromUtf8(s1);
    QString qs2;
    if (s2->type & KEY_NORMAL)
        qs2 = QString::fromLatin1(s2->keyname,s2->len_name);
    else
        qs2 = QString::fromUtf16((char16_t*)s2->keyname,s2->len_name);

    res = QString::compare(qs1, qs2, Qt::CaseInsensitive);
    return res;
}

