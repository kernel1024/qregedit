#include <QString>
#include <QByteArray>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstdarg>
#include <QDebug>
#include <QRegularExpression>

#include "chntpw/ntreg.h"
#include "functions.h"
#include "global.h"
#include "regutils.h"

/* Extended compare for key names with encoding
 * s1 - raw src keyname, in utf8 or Latin1 encoding
 * s2 - dst key struct. may contains encoded or unencoded key names
 */

int qf_strncasecmp(const char *s1, struct nk_key *s2)
{
    int res = 0;
    const QString qs1 = QString::fromUtf8(s1);
    QString qs2;

    if ((s2->type & KEY_NORMAL) != 0) {
        qs2 = QString::fromLatin1(s2->keyname, s2->len_name);
    } else {
        qs2 = QString::fromUtf16(reinterpret_cast<char16_t *>(s2->keyname), s2->len_name);
    }

    res = QString::compare(qs1, qs2, Qt::CaseInsensitive);
    return res;
}

void qf_printf( const char *format, ... )
{
    va_list args;
    va_start( args, format );
    QString msg = QString::vasprintf(format, args);
    va_end( args );

    msg.remove(QRegularExpression("[\\x00-\\x1f]"));
    QMessageLogger(nullptr, 0, nullptr, "ntreg").debug() << msg;
}

int ucs2utf8(char *src, char *dest, int l)
{
    QByteArray ba(src, l);
    ba = fromUtf16(ba).toUtf8();

    if (dest == nullptr)
        return ba.size();

    memcpy(dest, ba.data(), ba.size());
    return ba.size();
}
