/***************************************************************************
 *   Copyright (C) 2002-2007 by Victor Julien                              *
 *   victor@vuurmuur.org                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "config.h"
#include "vuurmuur.h"

/*  rule_malloc

    Allocates memory for a rule, and inits all variables.

    Returns the address of the memory on succes and NULL on failure.
*/
void *
rule_malloc(void)
{
    struct RuleData_ *rule_ptr = NULL;

    rule_ptr = malloc(sizeof(struct RuleData_));
    if(rule_ptr == NULL)
    {
        return(NULL);
    }
    /* clear */
    memset(rule_ptr, 0, sizeof(struct RuleData_));

    rule_ptr->type = TYPE_RULE;

    return(rule_ptr);
}


void *
ruleoption_malloc(const int debuglvl)
{
    struct options  *opt_ptr = NULL;

    /* alloc the memory */
    opt_ptr = malloc(sizeof(struct options));
    if(opt_ptr == NULL)
    {
        return(NULL);
    }
    /* initialize the mem */
    memset(opt_ptr, 0, sizeof(struct options));

    /* setup the lists */
    (void)d_list_setup(debuglvl, &opt_ptr->RemoteportList, NULL);
    (void)d_list_setup(debuglvl, &opt_ptr->ListenportList, NULL);

    return(opt_ptr);
}


void *
zone_malloc(const int debuglvl)
{
    struct ZoneData_ *zone_ptr = NULL;

    /* alloc memory */
    zone_ptr = malloc(sizeof(struct ZoneData_));
    if(zone_ptr == NULL)
    {
        return(NULL);
    }
    /* initialize */
    memset(zone_ptr, 0, sizeof(struct ZoneData_));

#ifdef IPV6_ENABLED
    zone_ptr->ipv6.cidr6 = -1;
#endif

    zone_ptr->GroupList.len = 0;
    if(d_list_setup(debuglvl, &zone_ptr->GroupList, NULL) < 0)
        return(NULL);

    zone_ptr->InterfaceList.len = 0;
    if(d_list_setup(debuglvl, &zone_ptr->InterfaceList, NULL) < 0)
        return(NULL);

    zone_ptr->ProtectList.len = 0;
    if(d_list_setup(debuglvl, &zone_ptr->ProtectList, free) < 0)
        return(NULL);

    zone_ptr->type = TYPE_UNSET;

    /* done, return the zone */
    return(zone_ptr);
}


void
zone_free(const int debuglvl, struct ZoneData_ *zone_ptr)
{
    if(!zone_ptr)
        return;

    if(zone_ptr->type == TYPE_GROUP)
        (void)d_list_cleanup(debuglvl, &zone_ptr->GroupList);

    if(zone_ptr->type == TYPE_NETWORK)
    {
        (void)d_list_cleanup(debuglvl, &zone_ptr->InterfaceList);
        (void)d_list_cleanup(debuglvl, &zone_ptr->ProtectList);
    }

    free(zone_ptr);
}


void *
service_malloc(void)
{
    struct ServicesData_ *ser_ptr = NULL;

    /* alloc some mem */
    ser_ptr = malloc(sizeof(struct ServicesData_));
    if(ser_ptr == NULL)
    {
        return(NULL);
    }

    /* init */
    memset(ser_ptr, 0, sizeof(struct ServicesData_));

    ser_ptr->type = TYPE_SERVICE;

    return(ser_ptr);
}


/* returns a initialized interface memory area */
void *
interface_malloc(const int debuglvl)
{
    struct InterfaceData_ *iface_ptr = NULL;

    iface_ptr = malloc(sizeof(struct InterfaceData_));
    if(iface_ptr == NULL)
    {
        return(NULL);
    }

    memset(iface_ptr, 0, sizeof(struct InterfaceData_));
#ifdef IPV6_ENABLED
    iface_ptr->ipv6.cidr6 = -1;
#endif
    iface_ptr->type = TYPE_INTERFACE;

    iface_ptr->active = TRUE;

    if(d_list_setup(debuglvl, &iface_ptr->ProtectList, free) < 0)
        return(NULL);

    iface_ptr->cnt = NULL;

    return(iface_ptr);
}


/*

*/
int
LockSHM(int lock, int sem_id)
{
    int                     z;
    static struct sembuf    sops = { 0, -1, 0 };

    sops.sem_num = 0;
    sops.sem_op  = (short int)(lock ? -1 : 1);

    do
    {
        z = semop(sem_id, &sops, 1);
    }
    while (z == -1 && errno == EINTR);

    if(z == -1)
    {
        return(0);
    }

    return(1);
}

/* return a ptr to the lib version string */
char *libvuurmuur_get_version(void) {
    return LIBVUURMUUR_VERSION;
}

/*  range_strcpy

    src must be NULL-terminated.

    Returncodes:
        0: ok
        -1: error

*/
int
range_strcpy(char *dest, const char *src, const size_t start,
        const size_t end, size_t size)
{
    size_t  s = 0,
            d = 0,
            src_len = 0;

    if(!dest || !src)
        return(-1);

    src_len = strlen(src);

    if(start >= src_len || end > src_len)
        return(-1);

    for(s = start, d = 0; s < end && d < size && s < src_len; s++, d++)
    {
        dest[d] = src[s];
    }
    dest[d]='\0';

    return(0);
}



/* textfield regex

    General regular expression for filtering out problematic
    chars like '"' and %...
*/
//#define TEXTFIELD_REGEX   "^([a-zA-Z0-9_-][.] )$"


/*
    Actions:     1: setup
                 0: setdown

    Returncodes:
            0: ok
            -1: error

*/
int
setup_rgx(int action, struct rgx_ *reg)
{
    if(!reg)
        return(-1);

    if(action < 0 || action > 1 || reg == NULL)
    {
        (void)vrprint.error(-1, "Internal Error", "parameter problem "
                "(in: %s:%d).", __FUNC__, __LINE__);
        return(-1);
    }

    if(action == 1)
    {
        /* regex setup */
        if(!(reg->zonename = malloc(sizeof(regex_t))))
        {
            (void)vrprint.error(-1, "Internal Error", "malloc "
                    "failed: %s (in: %s:%d).",
                    strerror(errno), __FUNC__, __LINE__);
            return(-1);
        }

        if(regcomp(reg->zonename, ZONE_REGEX, REG_EXTENDED) != 0)
        {
            (void)vrprint.error(-1, "Internal Error", "regcomp() "
                    "failed (in: %s:%d).",
                    __FUNC__, __LINE__);
            return(-1);
        }

        /* regex setup */
        if(!(reg->zone_part = malloc(sizeof(regex_t))))
        {
            (void)vrprint.error(-1, "Internal Error", "malloc "
                    "failed: %s (in: %s:%d).",
                    strerror(errno), __FUNC__, __LINE__);
            return(-1);
        }

        if(regcomp(reg->zone_part, ZONE_REGEX_ZONEPART, REG_EXTENDED) != 0)
        {
            (void)vrprint.error(-1, "Internal Error", "regcomp() "
                    "failed (in: %s:%d).",
                    __FUNC__, __LINE__);
            return(-1);
        }

        /* regex setup */
        if(!(reg->network_part = malloc(sizeof(regex_t))))
        {
            (void)vrprint.error(-1, "Internal Error", "malloc "
                    "failed: %s (in: %s:%d).",
                    strerror(errno), __FUNC__, __LINE__);
            return(-1);
        }

        if(regcomp(reg->network_part, ZONE_REGEX_NETWORKPART, REG_EXTENDED) != 0)
        {
            (void)vrprint.error(-1, "Internal Error", "regcomp() "
                    "failed (in: %s:%d).",
                    __FUNC__, __LINE__);
            return(-1);
        }

        /* regex setup */
        if(!(reg->host_part = malloc(sizeof(regex_t))))
        {
            (void)vrprint.error(-1, "Internal Error", "malloc "
                    "failed: %s (in: %s:%d).",
                    strerror(errno), __FUNC__, __LINE__);
            return(-1);
        }

        if(regcomp(reg->host_part, ZONE_REGEX_HOSTPART, REG_EXTENDED) != 0)
        {
            (void)vrprint.error(-1, "Internal Error", "regcomp() "
                    "failed (in: %s:%d).",
                    __FUNC__, __LINE__);
            return(-1);
        }

        /* regex setup */
        if(!(reg->servicename = malloc(sizeof(regex_t))))
        {
            (void)vrprint.error(-1, "Internal Error", "malloc "
                    "failed: %s (in: %s:%d).",
                    strerror(errno), __FUNC__, __LINE__);
            return(-1);
        }

        if(regcomp(reg->servicename, SERV_REGEX, REG_EXTENDED) != 0)
        {
            (void)vrprint.error(-1, "Internal Error", "regcomp() "
                    "failed (in: %s:%d).",
                    __FUNC__, __LINE__);
            return(-1);
        }

        /* regex setup */
        if(!(reg->interfacename = malloc(sizeof(regex_t))))
        {
            (void)vrprint.error(-1, "Internal Error", "malloc "
                    "failed: %s (in: %s:%d).",
                    strerror(errno), __FUNC__, __LINE__);
            return(-1);
        }

        if(regcomp(reg->interfacename, IFAC_REGEX, REG_EXTENDED) != 0)
        {
            (void)vrprint.error(-1, "Internal Error", "regcomp() "
                    "failed (in: %s:%d).",
                    __FUNC__, __LINE__);
            return(-1);
        }

        /* regex setup */
        if(!(reg->macaddr = malloc(sizeof(regex_t))))
        {
            (void)vrprint.error(-1, "Internal Error", "malloc "
                    "failed: %s (in: %s:%d).",
                    strerror(errno), __FUNC__, __LINE__);
            return(-1);
        }

        if(regcomp(reg->macaddr, MAC_REGEX, REG_EXTENDED) != 0)
        {
            (void)vrprint.error(-1, "Internal Error", "regcomp() "
                    "failed (in: %s:%d).",
                    __FUNC__, __LINE__);
            return(-1);
        }

        /* regex setup */
        if(!(reg->configline = malloc(sizeof(regex_t))))
        {
            (void)vrprint.error(-1, "Internal Error", "malloc "
                    "failed: %s (in: %s:%d).",
                    strerror(errno), __FUNC__, __LINE__);
            return(-1);
        }

        if(regcomp(reg->configline, CONFIG_REGEX, REG_EXTENDED) != 0)
        {
            (void)vrprint.error(-1, "Internal Error", "regcomp() "
                    "failed (in: %s:%d).",
                    __FUNC__, __LINE__);
            return(-1);
        }

        /* regex setup */
/*        if(!(reg->comment = malloc(sizeof(regex_t))))
        {
            (void)vrprint.error(-1, "Internal Error", "malloc "
                    "failed: %s (in: %s:%d).",
                    strerror(errno), __FUNC__, __LINE__);
            return(-1);
        }

        if(regcomp(reg->comment, TEXTFIELD_REGEX, REG_EXTENDED) != 0)
        {
            (void)vrprint.error(-1, "Internal Error", "regcomp() "
                    "failed (in: %s:%d).",
                    __FUNC__, __LINE__);
            return(-1);
        }
*/
    }
    else
    {
        /* zone */
        regfree(reg->zonename);
        free(reg->zonename);
        regfree(reg->zone_part);
        free(reg->zone_part);
        regfree(reg->network_part);
        free(reg->network_part);
        regfree(reg->host_part);
        free(reg->host_part);

        /* service */
        regfree(reg->servicename);
        free(reg->servicename);

        /* interface */
        regfree(reg->interfacename);
        free(reg->interfacename);

        /* mac */
        regfree(reg->macaddr);
        free(reg->macaddr);

        /* config */
        regfree(reg->configline);
        free(reg->configline);

        /* comment */
//        regfree(reg->comment);
//        free(reg->comment);
    }

    return(0);
}
