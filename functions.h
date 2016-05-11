#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#ifdef __cplusplus
extern "C" {
#endif

int qf_strncasecmp(const char *s1, struct nk_key *s2);
void qf_printf( const char* format, ... );
int ucs2utf8(char *src, char *dest, int l);

#ifdef __cplusplus
}
#endif

#endif // FUNCTIONS_H
