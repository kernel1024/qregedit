/*
 * chntpw.c - Offline Password Edit Utility for Windows SAM database
 *
 * This program uses the "ntreg" library to load and access the registry,
 * the "libsam" library for user / group handling
 * it's main purpose is to reset password based information.
 * It can also call the registry editor etc
 
 * 2013-may: Added group add/remove in user edit (using new functions
 *           in sam library)
 * 2013-apr: Changed around a bit on some features, chntpw is now
 *           mainly used for interactive edits.
 *           For automatic/scripted functions, use new programs:
 *           sampasswd and samusrgrp !
 * 2011-apr: Command line options added for hive expansion safe mode
 * 2010-jun: Syskey not visible in menu, but is selectable (2)
 * 2010-apr: Interactive menu adapts to show most relevant
 *           selections based on what is loaded
 * 2008-mar: Minor other tweaks
 * 2008-mar: Interactive reg ed moved out of this file, into edlib.c
 * 2008-mar: 64 bit compatible patch by Mike Doty, via Alon Bar-Lev
 *           http://bugs.gentoo.org/show_bug.cgi?id=185411
 * 2007-sep: Group handling extended, promotion now public
 * 2007-sep: User edit menu, some changes to user info edit
 * 2007-apr-may: Get and display users group memberships
 * 2007-apr: GNU license. Some bugfixes. Cleaned up some output.
 * 2004-aug: More stuff in regedit. Stringinput bugfixes.
 * 2004-jan: Changed some of the verbose/debug stuff
 * 2003-jan: Changed to use more of struct based V + some small stuff
 * 2003-jan: Support in ntreg for adding keys etc. Editor updated.
 * 2002-dec: New option: Specify user using RID
 * 2002-dec: New option: blank the pass (zero hash lengths).
 * 2001-jul: extra blank password logic (when NT or LANMAN hash missing)
 * 2001-jan: patched & changed to use OpenSSL. Thanks to Denis Ducamp
 * 2000-jun: changing passwords regardless of syskey.
 * 2000-jun: syskey disable works on NT4. Not properly on NT5.
 * 2000-jan: Attempt to detect and disable syskey
 * 1999-feb: Now able to browse registry hives. (write support to come)
 * See HISTORY.txt for more detailed info on history.
 *
 *****
 *
 * Copyright (c) 1997-2014 Petter Nordahl-Hagen.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * See file GPL.txt for the full license.
 *
 *****
 *
 * Information and ideas taken from pwdump by Jeremy Allison.
 *
 * More info from NTCrack by Jonathan Wilkins.
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <inttypes.h>

/* Define DOCRYPTO in makefile to include cryptostuff to be able to change passwords to
 * a new one.
 * Changing passwords is seems not to be working reliably on XP and newer anyway.
 * When not defined, only reset (nulling) of passwords available.
 */

#ifdef DOCRYPTO
#include <openssl/des.h>
#include <openssl/md4.h>
#endif

#define uchar u_char
#define MD4Init MD4_Init
#define MD4Update MD4_Update
#define MD4Final MD4_Final

#include "chntpw.h"

#include "../functions.h"

/* Global verbosity */
int gverbose = 0;

int syskeyreset = 0;

/* ============================================================== */


#ifdef DOCRYPTO

/* Crypto-stuff & support for what we'll do in the V-value */

/* Zero out string for lanman passwd, then uppercase
 * the supplied password and put it in here */

void make_lanmpw(char *p, char *lm, int len)
{
    int i;

    for (i=0; i < 15; i++) lm[i] = 0;
    for (i=0; i < len; i++) lm[i] = toupper(p[i]);
}

/*
 * Convert a 7 byte array into an 8 byte des key with odd parity.
 */

void str_to_key(unsigned char *str,unsigned char *key)
{
    int i;

    key[0] = str[0]>>1;
    key[1] = ((str[0]&0x01)<<6) | (str[1]>>2);
    key[2] = ((str[1]&0x03)<<5) | (str[2]>>3);
    key[3] = ((str[2]&0x07)<<4) | (str[3]>>4);
    key[4] = ((str[3]&0x0F)<<3) | (str[4]>>5);
    key[5] = ((str[4]&0x1F)<<2) | (str[5]>>6);
    key[6] = ((str[5]&0x3F)<<1) | (str[6]>>7);
    key[7] = str[6]&0x7F;
    for (i=0;i<8;i++) {
        key[i] = (key[i]<<1);
    }
    DES_set_odd_parity((des_cblock *)key);
}

/*
 * Function to convert the RID to the first decrypt key.
 */

void sid_to_key1(uint32_t sid,unsigned char deskey[8])
{
    unsigned char s[7];

    s[0] = (unsigned char)(sid & 0xFF);
    s[1] = (unsigned char)((sid>>8) & 0xFF);
    s[2] = (unsigned char)((sid>>16) & 0xFF);
    s[3] = (unsigned char)((sid>>24) & 0xFF);
    s[4] = s[0];
    s[5] = s[1];
    s[6] = s[2];

    str_to_key(s,deskey);
}

/*
 * Function to convert the RID to the second decrypt key.
 */

void sid_to_key2(uint32_t sid,unsigned char deskey[8])
{
    unsigned char s[7];

    s[0] = (unsigned char)((sid>>24) & 0xFF);
    s[1] = (unsigned char)(sid & 0xFF);
    s[2] = (unsigned char)((sid>>8) & 0xFF);
    s[3] = (unsigned char)((sid>>16) & 0xFF);
    s[4] = s[0];
    s[5] = s[1];
    s[6] = s[2];

    str_to_key(s,deskey);
}

/* DES encrypt, for LANMAN */

void E1(uchar *k, uchar *d, uchar *out)
{
    des_key_schedule ks;
    des_cblock deskey;

    str_to_key(k,(uchar *)deskey);
#ifdef __FreeBSD__
    des_set_key(&deskey,ks);
#else /* __FreeBsd__ */
    des_set_key((des_cblock *)deskey,ks);
#endif /* __FreeBsd__ */
    des_ecb_encrypt((des_cblock *)d,(des_cblock *)out, ks, DES_ENCRYPT);
}

#endif   /* DOCRYPTO */

/* Decode the V-struct, and change the password
 * vofs - offset into SAM buffer, start of V struct
 * rid - the users RID, required for the DES decrypt stage
 * newp - new password in UTF16, NULL for reset
 * pl - password length, bytes
 *
 * Some of this is ripped & modified from pwdump by Jeremy Allison
 *
 */

int change_user_pw(struct hive *hdesc, char *buf, int rid, int vlen, int stat, char *newp, int pl)
{
    char *vp;
    int ntpw_len,lmpw_len,ntpw_offs,lmpw_offs;
    struct user_V *v;

#ifdef DOCRYPTO
    int dontchange = 0;
    int i;
    char md4[32],lanman[32];
    char despw[20], newlanpw[16], newlandes[20];
    des_key_schedule ks1, ks2;
    des_cblock deskey1, deskey2;
    MD4_CTX context;
    unsigned char digest[16];
    uchar x1[] = {0x4B,0x47,0x53,0x21,0x40,0x23,0x24,0x25};
#endif

    v = (struct user_V *)buf;
    vp = buf;

    lmpw_offs       = v->lmpw_ofs;
    lmpw_len        = v->lmpw_len;
    ntpw_offs       = v->ntpw_ofs;
    ntpw_len        = v->ntpw_len;

    if (!rid) {
        qf_printf("No RID given. Unable to change passwords..\n");
        return(0);
    }

    if (gverbose) {
        qf_printf("lmpw_offs: 0x%x, lmpw_len: %d (0x%x)\n",lmpw_offs,lmpw_len,lmpw_len);
        qf_printf("ntpw_offs: 0x%x, ntpw_len: %d (0x%x)\n",ntpw_offs,ntpw_len,ntpw_len);
    }

    if(lmpw_offs < 0 || lmpw_offs >= vlen)
    {
        if (stat != 1) qf_printf("change_pw: Not a legal V struct? (negative struct lengths)\n");
        return(0);
    }

    /* Offsets in top of struct is relative to end of pointers, adjust */
    ntpw_offs += 0xCC;
    lmpw_offs += 0xCC;

    if (lmpw_len < 16 && gverbose) {
        qf_printf("** LANMAN password not set. User MAY have a blank password.\n** Usually safe to continue. Normal in Vista\n");
    }

    if (ntpw_len < 16) {
        qf_printf("** No NT MD4 hash found. This user probably has a BLANK password!\n");
        if (lmpw_len < 16) {
            qf_printf("** No LANMAN hash found either. Try login with no password!\n");
#ifdef DOCRYPTO
            dontchange = 1;
#endif
        } else {
            qf_printf("** LANMAN password IS however set. Will now install new password as NT pass instead.\n");
            qf_printf("** NOTE: Continue at own risk!\n");
            ntpw_offs = lmpw_offs;
            *(vp+0xa8) = ntpw_offs - 0xcc;
            ntpw_len = 16;
            lmpw_len = 0;
        }
    }

    if (gverbose) {
        hexprnt("Crypted NT pw: ",(unsigned char *)(vp+ntpw_offs),16);
        hexprnt("Crypted LM pw: ",(unsigned char *)(vp+lmpw_offs),16);
    }

#ifdef DOCRYPTO
    /* Get the two decrpt keys. */
    sid_to_key1(rid,(unsigned char *)deskey1);
    des_set_key((des_cblock *)deskey1,ks1);
    sid_to_key2(rid,(unsigned char *)deskey2);
    des_set_key((des_cblock *)deskey2,ks2);

    /* Decrypt the NT md4 password hash as two 8 byte blocks. */
    des_ecb_encrypt((des_cblock *)(vp+ntpw_offs ),
                    (des_cblock *)md4, ks1, DES_DECRYPT);
    des_ecb_encrypt((des_cblock *)(vp+ntpw_offs + 8),
                    (des_cblock *)&md4[8], ks2, DES_DECRYPT);

    /* Decrypt the lanman password hash as two 8 byte blocks. */
    des_ecb_encrypt((des_cblock *)(vp+lmpw_offs),
                    (des_cblock *)lanman, ks1, DES_DECRYPT);
    des_ecb_encrypt((des_cblock *)(vp+lmpw_offs + 8),
                    (des_cblock *)&lanman[8], ks2, DES_DECRYPT);

    if (gverbose) {
        hexprnt("MD4 hash     : ",(unsigned char *)md4,16);
        hexprnt("LANMAN hash  : ",(unsigned char *)lanman,16);
    }

    if (newp != NULL) {   /* Set new password */

        if (dontchange) {
            qf_printf("Sorry, unable to edit since password seems blank already (thus no space for it)\n");
            return(0);
        }

        //cheap_ascii2uni(newp,newunipw,pl);

        make_lanmpw(newp,newlanpw,pl);

        /*   printf("Ucase Lanman: %s\n",newlanpw); */

        MD4Init (&context);
        MD4Update (&context, newp, pl<<1);
        MD4Final (digest, &context);

        if (gverbose) hexprnt("\nNEW MD4 hash    : ",digest,16);

        E1((uchar *)newlanpw,   x1, (uchar *)lanman);
        E1((uchar *)newlanpw+7, x1, (uchar *)lanman+8);

        if (gverbose) hexprnt("NEW LANMAN hash : ",(unsigned char *)lanman,16);

        /* Encrypt the NT md4 password hash as two 8 byte blocks. */
        des_ecb_encrypt((des_cblock *)digest,
                        (des_cblock *)despw, ks1, DES_ENCRYPT);
        des_ecb_encrypt((des_cblock *)(digest+8),
                        (des_cblock *)&despw[8], ks2, DES_ENCRYPT);

        des_ecb_encrypt((des_cblock *)lanman,
                        (des_cblock *)newlandes, ks1, DES_ENCRYPT);
        des_ecb_encrypt((des_cblock *)(lanman+8),
                        (des_cblock *)&newlandes[8], ks2, DES_ENCRYPT);

        if (gverbose) {
            hexprnt("NEW DES crypt   : ",(unsigned char *)despw,16);
            hexprnt("NEW LANMAN crypt: ",(unsigned char *)newlandes,16);
        }

        /* Reset hash length to 16 if syskey enabled, this will cause
      * a conversion to syskey-hashes upon next boot */
        if (syskeyreset && ntpw_len > 16) {
            ntpw_len = 16;
            lmpw_len = 16;
            ntpw_offs -= 4;
            *(vp+0xa8) = (unsigned int)(ntpw_offs - 0xcc);
            *(vp + 0xa0) = 16;
            *(vp + 0xac) = 16;
        }

        for (i = 0; i < 16; i++) {
            *(vp+ntpw_offs+i) = (unsigned char)despw[i];
            if (lmpw_len >= 16) *(vp+lmpw_offs+i) = (unsigned char)newlandes[i];
        }

        hdesc->state |= HMODE_DIRTY;

        qf_printf("Password changed!\n");


    } else { /* new password */
#endif /* DOCRYPT */
        /* Setting hash lengths to zero seems to make NT think it is blank
      * However, since we cant cut the previous hash bytes out of the V value
      * due to missing resize-support of values, it may leak about 40 bytes
      * each time we do this.
      */
        v->ntpw_len = 0;
        v->lmpw_len = 0;

        hdesc->state |= HMODE_DIRTY;

        qf_printf("Password cleared!\n");
#ifdef DOCRYPTO
    }
#endif

    return(1);
}
