#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#ifdef __cplusplus
extern "C" {
#endif

int qf_strncasecmp(const char *s1, struct nk_key *s2);
void qf_printf( const char* format, ... );

#ifdef __cplusplus
}
#endif

#endif // FUNCTIONS_H
