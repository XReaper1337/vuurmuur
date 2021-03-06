/***************************************************************************
 *   Copyright (C) 2005-2008 by Victor Julien                              *
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

#include "vuurmuur_script.h"

int
script_list(const int debuglvl, VuurmuurScript *vr_script)
{
    char    back_zone[MAX_ZONE] = "",
            back_net[MAX_NETWORK] = "",
            back_host[MAX_HOST] = "";

    if(vr_script->type == TYPE_ZONE)
    {
        while(zf->list(debuglvl, zone_backend, vr_script->bdat, &vr_script->zonetype, CAT_ZONES) != NULL)
        {
            if(vr_script->zonetype == vr_script->type)
            {
                printf("%s\n", vr_script->bdat);
            }
        }
    }
    else if(vr_script->type == TYPE_NETWORK)
    {
        while(zf->list(debuglvl, zone_backend, vr_script->bdat, &vr_script->zonetype, CAT_ZONES) != NULL)
        {
            if(vr_script->zonetype == vr_script->type)
            {
                if(strcmp(vr_script->name,"any") != 0)
                {
                    /* validate and split the new name */
                    if(validate_zonename(debuglvl, vr_script->bdat, 0, back_zone, back_net, back_host, vr_script->reg.zonename, VALNAME_VERBOSE) != 0)
                    {
                        (void)vrprint.error(VRS_ERR_INTERNAL, VR_INTERR, "invalid name '%s' returned from backend (in: %s:%d).", vr_script->name, __FUNC__, __LINE__);
                        return(VRS_ERR_INTERNAL);
                    }
                    if(debuglvl >= HIGH)
                        (void)vrprint.debug(__FUNC__, "name: '%s': host/group '%s', net '%s', zone '%s'.",
                                                vr_script->bdat, back_host, back_net, back_zone);

                    if(strcmp(back_zone, vr_script->name_zone) == 0)
                    {
                        printf("%s\n", vr_script->bdat);
                    }
                }
                else
                {
                    printf("%s\n", vr_script->bdat);
                }
            }
        }
    }
    else if(vr_script->type == TYPE_HOST || vr_script->type == TYPE_GROUP)
    {
        while(zf->list(debuglvl, zone_backend, vr_script->bdat, &vr_script->zonetype, CAT_ZONES) != NULL)
        {
            if(vr_script->zonetype == vr_script->type)
            {
                if(strcmp(vr_script->name,"any") != 0)
                {
                    /* validate and split the new name */
                    if(validate_zonename(debuglvl, vr_script->bdat, 0, back_zone, back_net, back_host, vr_script->reg.zonename, VALNAME_VERBOSE) != 0)
                    {
                        (void)vrprint.error(VRS_ERR_INTERNAL, VR_INTERR, "invalid name '%s' returned from backend (in: %s:%d).", vr_script->name, __FUNC__, __LINE__);
                        return(VRS_ERR_INTERNAL);
                    }
                    if(debuglvl >= HIGH)
                        (void)vrprint.debug(__FUNC__, "name: '%s': host/group '%s', net '%s', zone '%s'.",
                                                vr_script->bdat, back_host, back_net, back_zone);

                    if( strcmp(back_zone, vr_script->name_zone) == 0 &&
                        (vr_script->name_net[0] == '\0' || strcmp(back_net, vr_script->name_net) == 0))
                    {
                        printf("%s\n", vr_script->bdat);
                    }
                }
                else
                {
                    printf("%s\n", vr_script->bdat);
                }
            }
        }
    }
    else if(vr_script->type == TYPE_SERVICE)
    {
        while(sf->list(debuglvl, serv_backend, vr_script->bdat, &vr_script->zonetype, CAT_SERVICES) != NULL)
        {
            printf("%s\n", vr_script->bdat);
        }
    }
    else if(vr_script->type == TYPE_INTERFACE)
    {
        while(af->list(debuglvl, ifac_backend, vr_script->bdat, &vr_script->zonetype, CAT_INTERFACES) != NULL)
        {
            printf("%s\n", vr_script->bdat);
        }
    }
    else if(vr_script->type == TYPE_RULE)
    {
        while(rf->list(debuglvl, rule_backend, vr_script->bdat, &vr_script->zonetype, CAT_RULES) != NULL)
        {
            printf("%s\n", vr_script->bdat);
        }
    }
    else
    {
        (void)vrprint.error(VRS_ERR_INTERNAL, VR_INTERR, "unknown type %d.", vr_script->type);
        return(VRS_ERR_INTERNAL);
    }

    return(0);
}
