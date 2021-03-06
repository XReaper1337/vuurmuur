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

/* - determine_action -
 * In this function we translate the 'accept' or 'drop' from the 'rules.conf' file to the
 * values that iptables understands, like 'ACCEPT, DROP, REJECT'.
 *
 * The function is called with the action from the rulesfile 'query' and returns the iptables
 * action trough 'action'.
 *
 * Returncodes:
 *      0: ok, found
 *     -1: invalid query
 */
int
determine_action(const int debuglvl, char *query, char *action, size_t size, struct options *option)
{
    int action_type = 0;

    /* safety */
    if(query == NULL || action == NULL || option == NULL)
    {
        (void)vrprint.error(-1, "Internal Error", "parameter problem "
            "(in: %s:%d).", __FUNC__, __LINE__);
        return(-1);
    }

    action_type = rules_actiontoi(query);
    if(action_type <= AT_ERROR || action_type >= AT_TOO_BIG)
    {
        (void)vrprint.error(-1, "Error", "unknown action '%s' "
            "(in: %s:%d).", query, __FUNC__, __LINE__);
        return(-1);
    }

    if(action_type == AT_ACCEPT)
    {
        (void)strlcpy(action, "NEWACCEPT", size);
    }
    else if(action_type == AT_DROP)
    {
        (void)strlcpy(action, "DROP", size);
    }
    else if(action_type == AT_REJECT)
    {
        (void)strlcpy(action, "REJECT", size);
        if(option->reject_option == 1)
        {
            if(debuglvl >= MEDIUM)
                (void)vrprint.debug(__FUNC__, "reject option: "
                        "'%s'.", option->reject_type);

            if(strcmp(option->reject_type, "tcp-reset") == 0)
            {
                (void)strlcpy(action, "TCPRESET", size);
            }
            else
            {
                snprintf(action, size, "REJECT --reject-with "
                        "%s", option->reject_type);
            }
        }
    }
    else if(action_type == AT_CHAIN)
    {
        (void)strlcpy(action, option->chain, size);
    }
    else if(action_type == AT_REDIRECT)
    {
        (void)strlcpy(action, "REDIRECT", size);
        if(option->redirectport > 0)
        {
            if(debuglvl >= MEDIUM)
                (void)vrprint.debug(__FUNC__, "redirect "
                        "option: '%d'.", option->redirectport);

            snprintf(action, size, "REDIRECT --to-ports "
                    "%d", option->redirectport);
        }
        else
        {
            (void)vrprint.error(-1, "Error", "target REDIRECT "
                    "requires option 'redirectport'.");
            return(-1);
        }
    }
    else if(action_type == AT_LOG)
    {
        if(strcmp(conf.loglevel, "") == 0)
            (void)strlcpy(action, "LOG", size);
        else
            snprintf(action, size, "LOG --log-level %s",
                    conf.loglevel);

        /* when action is LOG, the log option must not be set */
        option->rule_log = FALSE;

        if(debuglvl >= MEDIUM)
            (void)vrprint.debug(__FUNC__, "set option->rule_log "
                    "to FALSE because action is LOG.");
    }
    else if(action_type == AT_MASQ)
    {
        (void)strlcpy(action, "MASQUERADE", size);
    }
    else if(action_type == AT_SNAT)
    {
        (void)strlcpy(action, "SNAT", size);
    }
    else if(action_type == AT_PORTFW ||
        action_type == AT_DNAT ||
        action_type == AT_BOUNCE)
    {
        (void)strlcpy(action, "DNAT", size);
    }
    else if(action_type == AT_QUEUE)
    {
        (void)strlcpy(action, "NEWQUEUE", size);
    }
    else if(action_type == AT_NFQUEUE)
    {
        (void)strlcpy(action, "NEWNFQUEUE", size);
    }
    else
    {
        (void)vrprint.error(-1, "Error", "unknown action '%s' "
            "(in: %s:%d).", query, __FUNC__, __LINE__);
        return(-1);
    }

    return(0);
}


int
get_ip_info(const int debuglvl, char *name, struct ZoneData_ *answer_ptr, struct rgx_ *reg)
{
    int retval = 0,
        result = 0;

    /* safety */
    if(name == NULL || answer_ptr == NULL || reg == NULL)
    {
        (void)vrprint.error(-1, "Internal Error", "parameter problem (in: %s:%d).", __FUNC__, __LINE__);
        return(-1);
    }

    if(debuglvl >= MEDIUM)
        (void)vrprint.debug(__FUNC__, "determining info for '%s'.", name);

    switch(answer_ptr->type)
    {
        case TYPE_HOST:

            /* ask the ipaddress for this host */
            result = zf->ask(debuglvl, zone_backend, name, "IPADDRESS", answer_ptr->ipv4.ipaddress, sizeof(answer_ptr->ipv4.ipaddress), TYPE_HOST, 0);
            if(result < 0)
            {
                (void)vrprint.error(-1, "Internal Error", "zf->ask() failed (in: %s:%d).",
                        __FUNC__, __LINE__);
                return(-1);
            }

            /* get the mac-address */
            answer_ptr->has_mac = get_mac_address(debuglvl, name, answer_ptr->mac, sizeof(answer_ptr->mac), reg->macaddr);
            if(debuglvl >= HIGH)
                (void)vrprint.debug(__FUNC__, "has_mac: %d", answer_ptr->has_mac);

            /*  for iptables, the netmask of a single host is always 255.255.255.255,
                we do this after check_ip because otherwise check_ip would not work */
            strcpy(answer_ptr->ipv4.netmask, "255.255.255.255");

#ifdef IPV6_ENABLED
            /* ask the ipaddress for this host */
            result = zf->ask(debuglvl, zone_backend, name, "IPV6ADDRESS", answer_ptr->ipv6.ip6, sizeof(answer_ptr->ipv6.ip6), TYPE_HOST, 0);
            if(result < 0)
            {
                (void)vrprint.error(-1, "Internal Error", "zf->ask() failed (in: %s:%d).",
                        __FUNC__, __LINE__);
                return(-1);
            }

            if (strcmp(answer_ptr->ipv6.ip6, "") != 0) {
                /*  for iptables, the netmask of a single host is always /128,
                    we do this after check_ip because otherwise check_ip would not work */
                answer_ptr->ipv6.cidr6 = 128;
            }
#endif
            break;

        case TYPE_NETWORK:

            /*
                get the network address
            */
            if(debuglvl >= HIGH)
                (void)vrprint.debug(__FUNC__, "get network_ip for '%s', max_size: %d.", name, sizeof(answer_ptr->ipv4.network));

            result = zf->ask(debuglvl, zone_backend, name, "NETWORK", answer_ptr->ipv4.network, sizeof(answer_ptr->ipv4.network), TYPE_NETWORK, 0);
            if(result < 0)
            {
                (void)vrprint.error(-1, "Internal Error", "zf->ask() failed (in: %s:%d).",
                        __FUNC__, __LINE__);
                return(-1);
            }

            /*
                netmask
            */
            result = zf->ask(debuglvl, zone_backend, name, "NETMASK", answer_ptr->ipv4.netmask, sizeof(answer_ptr->ipv4.netmask), TYPE_NETWORK, 0);
            if(result < 0)
            {
                (void)vrprint.error(-1, "Internal Error", "zf->ask() failed (in: %s:%d).",
                        __FUNC__, __LINE__);
                return(-1);
            }

            /* get the broadcast address for this network/netmask combination */
            if(strcmp(answer_ptr->ipv4.network, "") != 0 && strcmp(answer_ptr->ipv4.netmask, "") != 0)
            {
                result = create_broadcast_ip(debuglvl, answer_ptr->ipv4.network, answer_ptr->ipv4.netmask, answer_ptr->ipv4.broadcast, sizeof(answer_ptr->ipv4.broadcast));
                if(result != 0)
                {
                    (void)vrprint.error(-1, "Error", "creating broadcast ip for zone '%s' failed.", answer_ptr->name);
                    answer_ptr->active = 0;
                }
            }

#ifdef IPV6_ENABLED
            result = zf->ask(debuglvl, zone_backend, name, "IPV6NETWORK", answer_ptr->ipv6.net6, sizeof(answer_ptr->ipv6.net6), TYPE_NETWORK, 0);
            if(result < 0)
            {
                (void)vrprint.error(-1, "Internal Error", "zf->ask() failed (in: %s:%d).",
                        __FUNC__, __LINE__);
                return(-1);
            }

            char cidrstr[4] = "";
            result = zf->ask(debuglvl, zone_backend, name, "IPV6CIDR", cidrstr, sizeof(cidrstr), TYPE_NETWORK, 0);
            if(result < 0)
            {
                (void)vrprint.error(-1, "Internal Error", "zf->ask() failed (in: %s:%d).",
                        __FUNC__, __LINE__);
                return(-1);
            }

            int cidr = atoi(cidrstr);
            if (cidr >= 0 && cidr <= 128) {
                answer_ptr->ipv6.cidr6 = cidr;
            } else {
                (void)vrprint.error(-1, "Error", "invalid IPV6 CIDR for zone "
                        "'%s', must be in range 0-128.", answer_ptr->name);
                answer_ptr->active = 0;
            }
#endif
            break;

        default:
            (void)vrprint.error(-1, "Internal Error", "expected a host or a network, got a %d (in: %s:%d).",
                    answer_ptr->type, __FUNC__, __LINE__);
            retval = -1;
            break;
    }

    return(retval);
}


/* create_broadcast_ip

    For broadcasting protocols we need an ipaddress to broadcast to.
    This function creates this ipaddress.

    Returncodes:
     0: ok
    -1: error
*/
int
create_broadcast_ip(const int debuglvl, char *network, char *netmask, char *broadcast_ip, size_t size)
{
    int retval=0;

    struct in_addr net;     /* the network address against we want to check */
    struct in_addr mask;    /* the netmask of the network */
    struct in_addr broad;   /* the broadcast address of this network */

    unsigned long int netmaskvalue=0;
    //unsigned long int networkvalue=0;

    if(debuglvl >= MEDIUM)
        (void)vrprint.debug(__FUNC__, "network: %s, netmask: %s", network, netmask);

    if(inet_aton(netmask, &mask) == 0)
    {
        (void)vrprint.error(-1, "Error", "Invalid netmask: '%s' (in: create_broadcast_ip).", netmask);
        return(-1);
    }
    else
    {
        netmaskvalue=ntohl(mask.s_addr);

        if(debuglvl >= HIGH)
            (void)vrprint.debug(__FUNC__, "netmask = %s", inet_ntoa(mask));
    }

    if(inet_aton(network, &net) == 0)
    {
        (void)vrprint.error(-1, "Error", "Invalid network: '%s' (in: create_broadcast_ip).", network);
        return(-1);
    }
    else
    {
        //networkvalue=ntohl(net.s_addr);

        if(debuglvl >= HIGH)
            (void)vrprint.debug(__FUNC__, "network = %s", inet_ntoa(net));
    }

    broad=net;
    broad.s_addr|=~ntohl(netmaskvalue);

    if(strlcpy(broadcast_ip, inet_ntoa(broad), size) >= size)
    {
        (void)vrprint.error(-1, "Internal Error", "string overflow "
            "(in: %s:%d).", __FUNC__, __LINE__);
        return(-1);
    }

    if(debuglvl >= LOW)
        (void)vrprint.debug(__FUNC__, "broadcast-address for network %s with netmask %s is %s.", network, netmask, broadcast_ip);

    return(retval);
}

/*  get_group_info

    This function reads the groupfile.

    Returncodes:
         0: ok
        -1: error
 */
int
get_group_info(const int debuglvl, Zones *zones, char *groupname, struct ZoneData_ *answer_ptr)
{
    int                 result = 0;
    char                total_zone[MAX_HOST_NET_ZONE] = "",
                        cur_mem[MAX_HOST] = "";
    struct ZoneData_    *zone_ptr = NULL;


    /* safety */
    if(groupname == NULL || answer_ptr == NULL || zones == NULL)
    {
        (void)vrprint.error(-1, "Internal Error", "parameter problem (in: %s:%d).",
                __FUNC__, __LINE__);
        return(-1);
    }
    if(answer_ptr->type != TYPE_GROUP)
    {
        (void)vrprint.error(-1, "Internal Error", "expected a group, but got a %d (in: %s:%d).",
                answer_ptr->type, __FUNC__, __LINE__);
        return(-1);
    }

    /* setup the list (allready done in zone_malloc?) */
    d_list_setup(debuglvl, &answer_ptr->GroupList, NULL);
    answer_ptr->group_member_count = 0;

    /* get the members */
    while((result = zf->ask(debuglvl, zone_backend, groupname, "MEMBER", cur_mem, sizeof(cur_mem), TYPE_GROUP, 1)) == 1)
    {
        answer_ptr->group_member_count++;

        snprintf(total_zone, sizeof(total_zone), "%s.%s.%s", cur_mem, answer_ptr->network_name, answer_ptr->zone_name);

        zone_ptr = search_zonedata(debuglvl, zones, total_zone);
        if(zone_ptr == NULL)
        {
            (void)vrprint.debug(__FUNC__, "the member '%s' of group '%s' was not found in memory.",
                    total_zone, groupname);
            answer_ptr->group_member_count--;
        }
        else
        {
            if(zone_ptr->type == TYPE_GROUP)
            {
                (void)vrprint.debug(__FUNC__, "only hosts can be groupmembers. Member '%s' of '%s' is a group.",
                        zone_ptr->name, groupname);
                answer_ptr->group_member_count--;
            }
            else
            {
                /* increase the refcnt of the host */
                zone_ptr->refcnt_group++;

                if(zone_ptr->active == 0)
                {
                    if(debuglvl >= LOW)
                        (void)vrprint.debug(__FUNC__, "member %s is not active", zone_ptr->name);
                }

                if(d_list_append(debuglvl, &answer_ptr->GroupList, zone_ptr) == NULL)
                {
                    (void)vrprint.error(-1, "Internal Error", "d_list_append() failed (in: %s:%d).",
                            __FUNC__, __LINE__);
                    return(-1);
                }

                if(debuglvl >= HIGH)
                    (void)vrprint.debug(__FUNC__, "refcnt_group of host '%s' is now '%u'.", zone_ptr->name, zone_ptr->refcnt_group);
            }
        }
    }
    if(result == -1)
    {
        (void)vrprint.error(-1, "Internal Error", "zf->ask() failed (in: %s:%d).",
                __FUNC__, __LINE__);
        return(-1);
    }

    return(0);
}


/*  returns a string with a list of ports.
    if option_name == NULL, then we just print the ports
    else we print it in the format for use in the rules languague

    returns NULL on error
*/
char *
list_to_portopts(const int debuglvl, d_list *dlist, /*@null@*/char *option_name)
{
    d_list_node     *d_node = NULL;
    char            options[MAX_OPTIONS_LENGTH] = "",
                    oneport[32] = "",
                    *return_ptr = NULL;
    struct portdata *portrange_ptr = NULL;
    size_t          size = 0;

    if(option_name != NULL)
    {
        /* option name + starting trema */
        snprintf(options, sizeof(options), "%s=\"", option_name);
    }

    /* loop trough the list and strcat */
    for(d_node = dlist->top; d_node; d_node = d_node->next)
    {
        portrange_ptr = d_node->data;

        /* single port */
        if(portrange_ptr->dst_high == -1)
        {
            snprintf(oneport, sizeof(oneport), "%d,",
                portrange_ptr->dst_low);

            if(strlcat(options, oneport, sizeof(options)) >= sizeof(options))
            {
                (void)vrprint.error(-1, "Internal Error",
                        "string overflow (in: %s:%d).",
                        __FUNC__, __LINE__);
                return(NULL);
            }
        }
        /* range */
        else
        {
            snprintf(oneport, sizeof(oneport), "%d:%d,",
                    portrange_ptr->dst_low, portrange_ptr->dst_high);

            if(strlcat(options, oneport, sizeof(options)) >= sizeof(options))
            {
                (void)vrprint.error(-1, "Internal Error",
                        "string overflow (in: %s:%d).",
                        __FUNC__, __LINE__);
                return(NULL);
            }
        }
    }
    /* overwrite the last comma */
    options[strlen(options)-1] = '\0';

    if(option_name != NULL)
    {
        /* the trailing trema */
        if(strlcat(options, "\"", sizeof(options)) >= sizeof(options))
        {
            (void)vrprint.error(-1, "Internal Error",
                    "string overflow (in: %s:%d).",
                    __FUNC__, __LINE__);
            return(NULL);
        }
    }

    size = strlen(options) + 1;

    if(!(return_ptr = malloc(size)))
    {
        (void)vrprint.error(-1, "Error", "malloc failed: %s "
            "(in: %s:%d).", strerror(errno), __FUNC__, __LINE__);
        return(NULL);
    }

    if(strlcpy(return_ptr, options, size) >= size)
    {
        (void)vrprint.error(-1, "Internal Error", "string overflow "
            "(in: %s:%d).", __FUNC__, __LINE__);
        return(NULL);
    }

    if(debuglvl >= MEDIUM)
        (void)vrprint.debug(__FUNC__, "options: '%s'.", return_ptr);

    return(return_ptr);
}

int
portopts_to_list(const int debuglvl, const char *opt, d_list *dlist)
{
    int             done=0,
                    range=0,
                    o=0,
                    p=0;
    char            option_string[MAX_OPTIONS_LENGTH] = "";
    struct portdata *portrange_ptr = NULL;

    if(debuglvl >= MEDIUM)
        (void)vrprint.debug(__FUNC__, "opt: '%s'", opt);

    /* if the first char is a whitespace, bail out */
    if(opt[0] == ' ')
    {
        if(debuglvl >= HIGH)
            (void)vrprint.debug(__FUNC__, "first char of 'opt' is a whitespace, so we bail out quietly (and without error).");

        return(0);
    }

    for(o=0, p=0; o <= (int)strlen(opt); o++)
    {
        if(opt[o] != '"')
        {
            if(opt[o] == ':')
                range = 1;

            if(opt[o] == ',')
            {
                option_string[p]='\0';
                done = 1;
                p=0;
            }
            else if(opt[o] == '"' || opt[o] == '\0')
            {
                option_string[p]='\0';
                done = 1;
            }
            else
            {
                option_string[p] = opt[o];
                p++;
            }

            if(done == 1)
            {
                if(debuglvl >= HIGH)
                    (void)vrprint.debug(__FUNC__, "now trying to insert: %s", option_string);

                if(!(portrange_ptr = malloc(sizeof(struct portdata))))
                {
                    (void)vrprint.error(-1, "Error", "malloc failed: %s (in: %s:%d).", strerror(errno), __FUNC__, __LINE__);
                    return(-1);
                }
                portrange_ptr->protocol = -1;
                portrange_ptr->src_low = -1;
                portrange_ptr->src_high = -1;
                portrange_ptr->dst_low = -1;
                portrange_ptr->dst_high = -1;

                if(range == 0)
                {
                    portrange_ptr->dst_low = atoi(option_string);
                    if(portrange_ptr->dst_low <= 0 || portrange_ptr->dst_low > 65535)
                    {
                        (void)vrprint.error(-1, "Error", "listenport must be 1-65535.");

                        free(portrange_ptr);
                        return(-1);
                    }
                }
                else
                {
                    /* split it! */
                    if(split_portrange(option_string, &portrange_ptr->dst_low, &portrange_ptr->dst_high) < 0)
                    {
                        free(portrange_ptr);
                        return(-1);
                    }

                    /* it is not a range after all */
                    if(portrange_ptr->dst_high == 0)
                        portrange_ptr->dst_high = -1;

                    if(debuglvl >= HIGH)
                        (void)vrprint.debug(__FUNC__, "listen: %d, %d", portrange_ptr->dst_low, portrange_ptr->dst_high);
                }

                /* append to the list */
                if(d_list_append(debuglvl, dlist, portrange_ptr) == NULL)
                {
                    (void)vrprint.error(-1, "Internal Error", "appending to list failed (in: %s:%d).", __FUNC__, __LINE__);

                    free(portrange_ptr);
                    return(-1);
                }
                done = 0;
            }
        }
    }

    return(0);
}


/*  check_active

    Checks if the supplied zoneinfo is active. It does this by calling ask_backend with
    the question 'ACTIVE'.

    return codes:
    -1: error
     0: not active
     1: active
 */
int
check_active(const int debuglvl, char *name, int type)
{
    int     result = 0;
    char    active[4] = "";

    /* safety */
    if(name == NULL)
    {
        (void)vrprint.error(-1, "Internal Error", "parameter problem (in: %s:%d).",
                __FUNC__, __LINE__);
        return(-1);
    }

    if(debuglvl >= MEDIUM)
        (void)vrprint.debug(__FUNC__, "type: %d, name = '%s'.", type, name);

    if(type >= TYPE_TOO_BIG)
    {
        (void)vrprint.error(-1, "Internal Error", "type is out of range (in: %s:%d).",
                __FUNC__, __LINE__);
        return(-1);
    }

    /* fw active */
    if(strcasecmp(name, "firewall") == 0 || strncasecmp(name, "firewall", 8) == 0)
    {
        if(debuglvl >= MEDIUM)
            (void)vrprint.debug(__FUNC__, "'firewall' is always active.");

        return(1);
    }

    /* service */
    if(type == TYPE_SERVICE || type == TYPE_SERVICEGRP)
    {
        result = sf->ask(debuglvl, serv_backend, name, "ACTIVE", active, sizeof(active), type, 0);
    }
    /* interface */
    else if(type == TYPE_INTERFACE)
    {
        result = af->ask(debuglvl, ifac_backend, name, "ACTIVE", active, sizeof(active), type, 0);
    }
    /* zone, network, host, group */
    else if(type == TYPE_ZONE || type == TYPE_NETWORK || type == TYPE_HOST || type == TYPE_GROUP)
    {
        result = zf->ask(debuglvl, zone_backend, name, "ACTIVE", active, sizeof(active), type, 0);
    }
    else
    {
        (void)vrprint.error(-1, "Internal Error", "type '%d' is unsupported (in: %s:%d).",
                type, __FUNC__, __LINE__);
        return(-1);
    }

    if(debuglvl >= HIGH)
        (void)vrprint.debug(__FUNC__, "'%s' (result: %d).", active, result);

    /* if we have an anwser, check it out */
    if(result == 1)
    {
        if(strncasecmp(active, "yes", 3) == 0)
        {
            if(debuglvl >= MEDIUM)
                (void)vrprint.debug(__FUNC__, "'%s' is active.", name);

            return(1);
        }
        else
        {
            if(debuglvl >= LOW)
                (void)vrprint.debug(__FUNC__, "'%s' is not active.", name);

            return(0);
        }
    }
    else if(result == 0)
    {
        if(debuglvl >= LOW)
            (void)vrprint.debug(__FUNC__, "keyword ACTIVE not found in '%s', assuming inactive.", name);

        return(0);
    }
    else
    {
        (void)vrprint.error(-1, "Error", "ask_backend returned error (in: %s:%d).", __FUNC__, __LINE__);
        return(-1);
    }
}


/*  get_dynamic_ip

    partly ripped from Net-tools 1.60 (c) Phil Blundell philb@gnu.org and
    Bernd Eckenfels net-tools@lina.inka.de

    Returncodes:
        1: ok
        0: not found
        -1: error
 */
int
get_dynamic_ip(const int debuglvl, char *device, char *answer_ptr, size_t size)
{
    int                 numreqs = 30;
    struct ifconf       ifc;
    struct ifreq        *ifr_ptr = NULL,
                        ifr_struct;
    int                 n;
    int                 sockfd = 0;
    char                ipaddress[16] = "";
    struct sockaddr     *sa = NULL;
    struct sockaddr_in  *sin = NULL;


    /* safety */
    if(!device || !answer_ptr || size == 0)
    {
        (void)vrprint.error(-1, "Internal Error", "parameter problem (in: %s:%d).", __FUNC__, __LINE__);
        return(-1);
    }

    /* open a socket for ioctl */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd == -1)
    {
        (void)vrprint.error(-1, "Error", "couldn't open socket: %s (in: %s:%d).", strerror(errno), __FUNC__, __LINE__);
        return(-1);
    }

    /* initialize the buf otherwise realloc will freak out (read segv) */
    ifc.ifc_buf = NULL;
    for (;;)
    {
        ifc.ifc_len = (int)(sizeof(struct ifreq) * numreqs);
        /* get some mem */
        if(!(ifc.ifc_buf = realloc(ifc.ifc_buf, (size_t)ifc.ifc_len)))
        {
            (void)vrprint.error(-1, "Error", "realloc failed: %s (in: %s:%d).", strerror(errno), __FUNC__, __LINE__);
            (void)close(sockfd);

            return(-1);
        }

        /* get the interfaces from the system */
        if(ioctl(sockfd, SIOCGIFCONF, &ifc) < 0)
        {
            (void)vrprint.error(-1, "Error", "ioctl(SIOCGIFCONF) failed: %s (in: %s:%d).", strerror(errno), __FUNC__, __LINE__);
            free(ifc.ifc_buf);
            (void)close(sockfd);

            return(-1);
        }
        if(ifc.ifc_len == (int)(sizeof(struct ifreq) * numreqs))
        {
            /* assume it overflowed and try again */
            numreqs += 10;
            continue;
        }
        break;
    }

    ifr_ptr = ifc.ifc_req;
    for(n = 0; n < ifc.ifc_len; n += sizeof(struct ifreq))
    {
        if(debuglvl >= HIGH)
            (void)vrprint.debug(__FUNC__, "ifr_ptr->ifr_name: '%s'.", ifr_ptr->ifr_name);

        if(strcmp(device, ifr_ptr->ifr_name) == 0)
        {
            if(strlcpy(ifr_struct.ifr_name, ifr_ptr->ifr_name, sizeof(ifr_struct.ifr_name)) >= sizeof(ifr_struct.ifr_name))
            {
                (void)vrprint.error(-1, "Error", "buffer overflow (in: %s:%d).", __FUNC__, __LINE__);
                (void)close(sockfd);
                free(ifc.ifc_buf);
                return(-1);
            }
            /* we only care about IPv4 */
            ifr_struct.ifr_addr.sa_family = AF_INET;

            /* cast to a socketaddr */
            sa = &ifr_struct.ifr_addr;

            if(ioctl(sockfd, SIOCGIFADDR, &ifr_struct) == 0)
            {
                sin = (struct sockaddr_in *)sa;

                /* get the ipaddress into a string */
                if(inet_ntop(AF_INET, &sin->sin_addr, ipaddress, (socklen_t)sizeof(ipaddress)) == NULL)
                {
                    (void)vrprint.error(-1, "Error", "getting ipaddress for device '%s' failed: %s (in: %s:%d).", device, strerror(errno), __FUNC__, __LINE__);
                    (void)close(sockfd);
                    free(ifc.ifc_buf);

                    return(-1);
                }

                if(debuglvl >= LOW)
                    (void)vrprint.debug(__FUNC__, ", device: '%s', ipaddress: '%s'.", device, ipaddress);

                /* copy back to the caller */
                if(strlcpy(answer_ptr, ipaddress, size) >= size)
                {
                    (void)vrprint.error(-1, "Error", "copying ipaddress for device '%s' failed: destination buffer too small (in: %s:%d).", device, __FUNC__, __LINE__);
                    (void)close(sockfd);
                    free(ifc.ifc_buf);

                    return(-1);
                }

                /* found! */
                (void)close(sockfd);
                free(ifc.ifc_buf);
                return(1);
            }
        }
        ifr_ptr++;
    }

    /* not found */
    if(debuglvl >= LOW)
        (void)vrprint.debug(__FUNC__, "device '%s' not found.", device);

    close(sockfd);
    free(ifc.ifc_buf);
    return(0);
}


/*  check_ipv4address

    Checks if a ipaddress is valid, in two steps
        1: check if the ipaddress itself is valid
        2: check if it belong to the network/netmask combo supplied

    network and netmask may be NULL. If this is the case 'ipaddress' will
    only be checked for validity, not for belonging in a network.

    if 'quiet' is 1, we don't print errors, just return the code.

    Returncodes:
         1: valid
         0: error: not in network/netmask
        -1: error: no valid ip/net/mask
*/
int
check_ipv4address(const int debuglvl, char *network, char *netmask, char *ipaddress, char quiet)
{
    int                 retval = 0;

    struct in_addr      ip;      /* the ipaddress we want to check */
    struct in_addr      net;     /* the network address against we want to check */
    struct in_addr      mask;    /* the netmask of the network */
    struct in_addr      broad;   /* the broadcast address of this network */

    unsigned long int   netmaskvalue = 0;
    //unsigned long int   ipaddressvalue = 0;
    //unsigned long int   networkvalue = 0;

    unsigned long int   high;
    unsigned long int   low;
    unsigned long int   current;


    /* safety */
    if(!ipaddress)
    {
        (void)vrprint.error(-1, "Internal Error", "parameter problem (in: %s:%d).", __FUNC__, __LINE__);
        return(-1);
    }

    /* first check if the ipaddress itself is valid */
    if(inet_aton(ipaddress, &ip) == 0)
    {
        if(!quiet)
        {
            (void)vrprint.error(-1, "Error", "invalid ipaddress: '%s' (in: %s).", ipaddress, __FUNC__);
        }

        return(-1);
    }
    else
    {
        //ipaddressvalue = ntohl(ip.s_addr);

        if(debuglvl >= HIGH)
            (void)vrprint.debug(__FUNC__, "ipaddress = %s", inet_ntoa(ip));

        /* if were only checking ipaddress we are happy now. */
        if(!network && !netmask)
            return(1);
    }

    /* check if the networkadress is valid */
    if(inet_aton(network, &net) == 0)
    {
        (void)vrprint.error(-1, "Error", "invalid network: '%s' (in: %s).", network, __FUNC__);
        return(-1);
    }
    else
    {
        //networkvalue = ntohl(net.s_addr);

        if(debuglvl >= HIGH)
            (void)vrprint.debug(__FUNC__, "network = %s", inet_ntoa(net));
    }

    /* check if the netmask is valid */
    if(inet_aton(netmask, &mask) == 0)
    {
        (void)vrprint.error(-1, "Error", "invalid netmask: '%s' (in: %s).", netmask, __FUNC__);
        return(-1);
    }
    else
    {
        netmaskvalue = ntohl(mask.s_addr);
        if(debuglvl >= HIGH)
            (void)vrprint.debug(__FUNC__, "netmask = %s", inet_ntoa(mask));
    }


    broad = net;
    broad.s_addr|=~ntohl(netmaskvalue);

    if(debuglvl >= HIGH)
        (void)vrprint.debug(__FUNC__, "broad = %s", inet_ntoa(broad));

    /* get the lowest possible ip in this network/netmask combi */
    low = ntohl(net.s_addr);
    /* get the highest possible ip in this network/netmask combi */
    high = ntohl(broad.s_addr);
    /* get our ip */
    current = ntohl(ip.s_addr);

    /* finally check if the ipaddress fits in the range */
    if(current > low && current < high)
    {
        if(debuglvl >= HIGH)
            (void)vrprint.debug(__FUNC__, "ipaddress %s belongs to network %s with netmask %s", ipaddress, network, netmask);

        retval = 1;
    }

    return(retval);
}


/*  get_mac_address

    Gets the mac address of a host from the backend and checks it.

    Returncodes:
         0: ok
        -1: error
*/
int
get_mac_address(const int debuglvl, char *hostname, char *answer_ptr, size_t size, regex_t *mac_rgx)
{
    int retval = 0,
        result = 0;

    /* safety */
    if(hostname == NULL || mac_rgx == NULL)
    {
        (void)vrprint.error(-1, "Internal Error", "parameter problem (in: %s:%d).", __FUNC__, __LINE__);
        return(-1);
    }

    /* ask the backend */
    result = zf->ask(debuglvl, zone_backend, hostname, "MAC", answer_ptr, size, TYPE_HOST, 0);
    if(result == 1)
    {
        if(debuglvl >= HIGH)
            (void)vrprint.debug(__FUNC__, "found!");

        retval = 1;

        if(strcmp(answer_ptr, "none") == 0)
        {
            retval = 0;
        }
        else
        {
            /* test against the regex */
            if(regexec(mac_rgx, answer_ptr, 0, NULL, 0) != 0)
            {
                (void)vrprint.error(-1, "Error", "MAC '%s' for host '%s' is invalid.", answer_ptr, hostname);
                retval=-1;
            }
        }
    }
    else if(result == 0)
    {
        if(debuglvl >= HIGH)
            (void)vrprint.debug(__FUNC__, "not found");
    }
    else
    {
        (void)vrprint.error(-1, "Error", "getting macaddress for %s failed (in: get_mac_address).", hostname);
        retval=-1;
    }

    return(retval);
}


/*  TODO: in 0.8 we need to rethink this. Hardcoding this makes no sense.

    returncodes
         0: ok
        -1: error
*/
int
get_danger_info(const int debuglvl, char *danger, char *source, struct danger_info *danger_struct)
{
    /* safety */
    if(danger == NULL || source == NULL || danger_struct == NULL)
    {
        (void)vrprint.error(-1, "Internal Error", "parameter problem "
            "(in: %s:%d).", __FUNC__, __LINE__);
        return(-1);
    }

    /* spoofing dangers */
    if(strcasecmp(danger, "spoofing") == 0)
    {
        if(strlcpy(danger_struct->type, "spoof", sizeof(danger_struct->type)) >= sizeof(danger_struct->type))
        {
            (void)vrprint.error(-1, "Internal Error", "string "
                    "overflow (in: %s:%d).", __FUNC__, __LINE__);
            return(-1);
        }

        if(strlcpy(danger_struct->source, source, sizeof(danger_struct->source)) >= sizeof(danger_struct->source))
        {
            (void)vrprint.error(-1, "Internal Error", "string "
                    "overflow (in: %s:%d).", __FUNC__, __LINE__);
            return(-1);
        }

        if(strcasecmp(source, "loopback") == 0)
        {
            danger_struct->solution = PROT_IPTABLES;
            strcpy(danger_struct->source_ip.ipaddress, "127.0.0.0");
            strcpy(danger_struct->source_ip.netmask, "255.0.0.0");
        }
        else if(strcasecmp(source, "class-a") == 0)
        {
            danger_struct->solution = PROT_IPTABLES;
            strcpy(danger_struct->source_ip.ipaddress, "10.0.0.0");
            strcpy(danger_struct->source_ip.netmask, "255.0.0.0");
        }
        else if(strcasecmp(source, "class-b") == 0)
        {
            danger_struct->solution = PROT_IPTABLES;
            strcpy(danger_struct->source_ip.ipaddress, "172.16.0.0");
            strcpy(danger_struct->source_ip.netmask, "255.240.0.0");
        }
        else if(strcasecmp(source, "class-c") == 0)
        {
            danger_struct->solution = PROT_IPTABLES;
            strcpy(danger_struct->source_ip.ipaddress, "192.168.0.0");
            strcpy(danger_struct->source_ip.netmask, "255.255.0.0");
        }
        else if(strcasecmp(source, "class-d") == 0)
        {
            danger_struct->solution = PROT_IPTABLES;
            strcpy(danger_struct->source_ip.ipaddress, "224.0.0.0");
            strcpy(danger_struct->source_ip.netmask, "240.0.0.0");
        }
        else if(strcasecmp(source, "class-e") == 0)
        {
            danger_struct->solution = PROT_IPTABLES;
            strcpy(danger_struct->source_ip.ipaddress, "240.0.0.0");
            strcpy(danger_struct->source_ip.netmask, "248.0.0.0");
        }
        else if(strcasecmp(source, "test-net") == 0)
        {
            danger_struct->solution = PROT_IPTABLES;
            strcpy(danger_struct->source_ip.ipaddress, "192.0.2.0");
            strcpy(danger_struct->source_ip.netmask, "255.255.255.0");
        }
        else if(strcasecmp(source, "lnk-loc-net") == 0)
        {
            danger_struct->solution = PROT_IPTABLES;
            strcpy(danger_struct->source_ip.ipaddress, "169.254.0.0");
            strcpy(danger_struct->source_ip.netmask, "255.255.0.0");
        }
        else if(strcasecmp(source, "iana-0/8") == 0)
        {
            danger_struct->solution = PROT_IPTABLES;
            strcpy(danger_struct->source_ip.ipaddress, "0.0.0.0");
            strcpy(danger_struct->source_ip.netmask, "255.0.0.0");
        }
        else if(strcasecmp(source, "brdcst-src") == 0)
        {
            danger_struct->solution = PROT_IPTABLES;
            strcpy(danger_struct->source_ip.ipaddress, "0.0.0.0");
            strcpy(danger_struct->source_ip.netmask, "255.255.255.255");
        }
        else if(strcasecmp(source, "brdcst-dst") == 0)
        {
            danger_struct->solution = PROT_IPTABLES;
            strcpy(danger_struct->source_ip.ipaddress, "255.255.255.255");
            strcpy(danger_struct->source_ip.netmask, "255.255.255.255");
        }
        else
        {
            (void)vrprint.error(-1, "Internal Error", "unknown "
                    "source: '%s' (in: %s:%d).",
                    source, __FUNC__, __LINE__);
            return(-1);
        }

    }

    /* system dangers */
    else if(strcasecmp(danger, "syn-flood") == 0)
    {
        danger_struct->solution = PROT_PROC_SYS;
        (void)strlcpy(danger_struct->proc_entry,
            "/proc/sys/net/ipv4/tcp_syncookies",
            sizeof(danger_struct->proc_entry));
        danger_struct->proc_set_on = 1;
        danger_struct->proc_set_off = 0;
    }
    else if(strcasecmp(danger, "echo-broadcast") == 0)
    {
        danger_struct->solution = PROT_PROC_SYS;
        (void)strlcpy(danger_struct->proc_entry,
            "/proc/sys/net/ipv4/icmp_echo_ignore_broadcasts",
            sizeof(danger_struct->proc_entry));
        danger_struct->proc_set_on = 1;
        danger_struct->proc_set_off = 0;
    }

    /* interface dangers */
    else if(strcasecmp(danger, "source-routed-packets") == 0)
    {
        danger_struct->solution = PROT_PROC_INT;
        (void)strlcpy(danger_struct->proc_entry, "/proc/sys/net/ipv4/conf/*/accept_source_route",
            sizeof(danger_struct->proc_entry));
        danger_struct->proc_set_on = 0;
        danger_struct->proc_set_off = 1;
    }
    else if(strcasecmp(danger, "icmp-redirect") == 0)
    {
        danger_struct->solution = PROT_PROC_INT;
        (void)strlcpy(danger_struct->proc_entry, "/proc/sys/net/ipv4/conf/*/accept_redirects",
            sizeof(danger_struct->proc_entry));
        danger_struct->proc_set_on = 0;
        danger_struct->proc_set_off = 1;
    }
    else if(strcasecmp(danger, "send-redirect") == 0)
    {
        danger_struct->solution = PROT_PROC_INT;
        (void)strlcpy(danger_struct->proc_entry, "/proc/sys/net/ipv4/conf/*/send_redirects",
            sizeof(danger_struct->proc_entry));
        danger_struct->proc_set_on = 0;
        danger_struct->proc_set_off = 1;
    }
    else if(strcasecmp(danger, "rp-filter") == 0)
    {
        danger_struct->solution = PROT_PROC_INT;
        (void)strlcpy(danger_struct->proc_entry, "/proc/sys/net/ipv4/conf/*/rp_filter",
            sizeof(danger_struct->proc_entry));
        danger_struct->proc_set_on = 1;
        danger_struct->proc_set_off = 0;
    }
    else if(strcasecmp(danger, "log-martians") == 0)
    {
        danger_struct->solution = PROT_PROC_INT;
        (void)strlcpy(danger_struct->proc_entry, "/proc/sys/net/ipv4/conf/*/log_martians",
            sizeof(danger_struct->proc_entry));
        danger_struct->proc_set_on = 1;
        danger_struct->proc_set_off = 0;
    }

    /* default case */
    else
    {
        (void)vrprint.error(-1, "Internal Error", "unknown danger: "
            "'%s' (in: %s:%d).", source, __FUNC__, __LINE__);
        return(-1);
    }

    return(0);
}


/*  get_network_for_ipv4

    This functions checks to which network a ipv4 address belongs. It does
    this by looping trough the zoneslist and checking if the ipaddress
    fits in a network it returns a pointer to the name of the network,
    or else NULL.

    CAUTION!!! -> there is one major problem with this aproach. Its can be
    so slooooowwwwww!

    A solution is providing a list of only networks...
*/
char
*get_network_for_ipv4(const int debuglvl, const char *ipaddress, d_list *zonelist)
{
    struct in_addr      ip;      /* the ipaddress we want to check */
    struct in_addr      net;     /* the network address against we want to check */
    struct in_addr      mask;    /* the netmask of the network */
    struct in_addr      broad;   /* the broadcast address of this network */

    unsigned long int   netmaskvalue = 0;
    //unsigned long int   ipaddressvalue = 0;
    //unsigned long int   networkvalue = 0;

    unsigned long int   high = 0;
    unsigned long int   low = 0;
    unsigned long int   current = 0;

    unsigned long int   best_so_far = 0;

    struct ZoneData_    *zone_ptr = NULL,
                        *best_so_far_ptr = NULL;

    char                *result_ptr = NULL;
    d_list_node         *d_node = NULL;

    /*
        safety
    */
    if(!ipaddress || !zonelist)
    {
        (void)vrprint.error(-1, "Internal Error", "parameter problem (in: %s:%d).", __FUNC__, __LINE__);
        return(NULL);
    }


    /* we don't want the local loopback */
    if(strncmp(ipaddress, "127.", 4) == 0)
        return(NULL);

    /* first check if the ipaddress itself is valid */
    if(inet_aton(ipaddress, &ip) == 0)
        return(NULL);

    //ipaddressvalue = ntohl(ip.s_addr);

    /* get our ip */
    current = ntohl(ip.s_addr);

    /*
        now loop trough the zonelist in search of networks
    */
    for(d_node = zonelist->top; d_node; d_node = d_node->next)
    {
        if(!(zone_ptr = d_node->data))
        {
            (void)vrprint.error(-1, "Internal Error", "NULL pointer (in: %s:%d).", __FUNC__, __LINE__);
            return(NULL);
        }

        if(zone_ptr->type == TYPE_NETWORK)
        {
            /* check if the networkadress is valid */
            if(inet_aton(zone_ptr->ipv4.network, &net) != 0)
            {
                //networkvalue = ntohl(net.s_addr);

                /* check if the netmask is valid */
                if(inet_aton(zone_ptr->ipv4.netmask, &mask) != 0)
                {
                    netmaskvalue = ntohl(mask.s_addr);

                    /* create broadcast */
                    broad = net;
                    broad.s_addr|=~ntohl(netmaskvalue);

                    /* get the lowest possible ip in this network/netmask combi */
                    low = ntohl(net.s_addr);

                    /* get the highest possible ip in this network/netmask combi */
                    high = ntohl(broad.s_addr);

                    /* include the broadcastaddress, so current <= high */
                    if((current > low) && (current <= high))
                    {
                        if(best_so_far == 0 || (high - low) < best_so_far)
                        {
                            best_so_far = high - low;
                            best_so_far_ptr = zone_ptr;
                        }
                    }
                }
            }
        }
    }

    if(best_so_far_ptr != NULL)
    {
        if(!(result_ptr = (char *)malloc(sizeof(best_so_far_ptr->name))))
        {
            (void)vrprint.error(-1, "Error", "malloc failed: %s (in: %s).", strerror(errno), __FUNC__);
            return(NULL);
        }

        if(strlcpy(result_ptr, best_so_far_ptr->name, sizeof(best_so_far_ptr->name)) > sizeof(best_so_far_ptr->name))
        {
            (void)vrprint.error(-1, "Internal Error", "string overflow (in: %s:d).", __FUNC__, __LINE__);
            free(result_ptr);

            return(NULL);
        }
    }

    return(result_ptr);
}



/**
 * 
 * @param debuglvl 
 * @param user - pointer to user data
 * @return  0: ok
 *          -1: error
 */
int
get_user_info(const int debuglvl, VR_user_t *user)
{
    char            *proc_self_fd_0 = "/proc/self/fd/0",
                    term_path[256] = "";
    int             n = 0;
    struct stat     stat_buf;
    struct passwd   *pwd = NULL;

    /* safety */
    if(user == NULL)
    {
        (void)vrprint.error(-1, "Internal Error", "parameter problem "
            "(in: %s:%d).", __FUNC__, __LINE__);
        return(-1);
    }

    /* clear the memory */
    memset(user, 0, sizeof(VR_user_t));

    /* get the effective user id */
    user->user = getuid();
    user->group = getgid();

    pwd = getpwuid(user->user);
    if(pwd != NULL)
    {
        (void)strlcpy(user->username, pwd->pw_name, sizeof(user->username));

        /* see where the procfile links to */
        n = readlink(proc_self_fd_0, term_path, sizeof(term_path) - 1);
        if(n > 0)
        {
            /* terminate the string */
            term_path[n] = '\0';

            /* stat the damn thing */
            if(lstat(term_path, &stat_buf) != -1)
            {
                /* get the owner of the 'file' */
                user->realuser = stat_buf.st_uid;

                /* get the name of this user */
                pwd = getpwuid(user->realuser);
                if(pwd != NULL)
                {
                    (void)strlcpy(user->realusername,
                            pwd->pw_name,
                            sizeof(user->realusername));

                    return(0);
                }
            }
        }
    }

    /* something went wrong */
    (void)strlcpy(user->username, "unknown", sizeof(user->username));
    (void)strlcpy(user->groupname, "unknown", sizeof(user->groupname));
    (void)strlcpy(user->realusername, "unknown", sizeof(user->realusername));

    return(-1);
}
