/***************************************************************************
 *   Copyright (C) 2003-2008 by Victor Julien                              *
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
 
#ifndef __MAIN_H__
#define __MAIN_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/utsname.h>    /* for uname -> stat_sec */
#include <signal.h>
#include <string.h>
#include <sys/time.h>   /* for gettimeofday in stat_sec */
#include <sys/wait.h>   /* for WIFEXITED macro */
#include <ctype.h>  /* for isdigit() */

#include <vuurmuur.h>

#ifdef HAVE_NC_WIDE_HEADERS
#include <ncursesw/ncurses.h>
#include <ncursesw/menu.h>
#include <ncursesw/panel.h>
#include <ncursesw/form.h>
#else
#include <ncurses.h>
#include <menu.h>
#include <panel.h>
#include <form.h>
#endif /* HAVE_NC_WIDE_HEADERS */

#if !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif
#include <getopt.h>

#ifdef USE_WIDEC
#include <wchar.h>
#endif /* USE_WIDEC */

#include <regex.h>

#include <locale.h> /* for gettext() */
#include "gettext.h"

#include "strings.h"
#include "gui.h"

#ifndef LOCALEDIR
#define LOCALEDIR   "/usr/share/locale"
#endif /* LOCALEDIR */

#define NLINES  10
#define NCOLS   40

#define VUURMUURCONF_VERSION	"0.8rc1"

#define LOCK(x)             LockSHM(1, x)
#define UNLOCK(x)           LockSHM(0, x)

#define SILENT_LOCK(x)      LockSHM(1, x)
#define SILENT_UNLOCK(x)    LockSHM(0, x)

/* Initialize all the color pairs */
#define CP_WIN          1
#define CP_WIN_REV      2
#define CP_WIN_MARK     3
#define CP_WIN_FIELD    4

#define CP_WIN_RED      5
#define CP_WIN_RED_REV  6
#define CP_WIN_GREEN    7
#define CP_WIN_GREEN_REV 8
#define CP_WIN_YELLOW   9
#define CP_WIN_MAGENTA  10
#define CP_WIN_CYAN     11

#define CP_BGD          12
#define CP_BGD_REV      13
#define CP_BGD_RED      14
#define CP_BGD_GREEN    15
#define CP_BGD_YELLOW   16
#define CP_BGD_MAGENTA  17
#define CP_BGD_CYAN     18

#define CP_WIN_INIT     19
#define CP_WIN_WARN     20
#define CP_WIN_NOTE     21
#define CP_WIN_NOTE_REV 22

#define CP_RULE_BAR     23

/* the lists */
d_list  PluginList;

#ifdef SYSCONFDIR
#define VUURMUURCONF_CONFIGFILE SYSCONFDIR "/vuurmuur/vuurmuur_conf.conf"
#else
#define VUURMUURCONF_CONFIGFILE "/etc/vuurmuur/vuurmuur_conf.conf"
#endif


/* Vuurmuur_conf settings


*/
typedef struct
{
    char            configfile_location[128];

    char            helpfile_location[256];
    char            scripts_location[256];

    char            newrule_log;
    unsigned int    newrule_loglimit;
    unsigned int    newrule_logburst; /* set to 2x loglimit */

    unsigned int    logview_bufsize;

    char            advanced_mode;  /* is the interface in advanced mode ? */

    char            draw_status;    /* draw the status stuff in the main_menu? */

    char            iptrafvol_location[128];

    /*
        colors
    */
    int background; /* 0 blue, 1 black */

    /* windows */
    short           win_fore;
    short           win_back;

    chtype          color_win_init;
    chtype          color_win_warn;
    chtype          color_win_note;
    chtype          color_win_note_rev;

    chtype          color_win;
    chtype          color_win_mark;
    chtype          color_win_field;

    chtype          color_win_red;
    chtype          color_win_red_rev;
    chtype          color_win_green;
    chtype          color_win_green_rev;
    chtype          color_win_yellow;
    chtype          color_win_magenta;
    chtype          color_win_cyan;

    chtype          color_win_rev;
    chtype          color_win_rev_yellow;

    /* background */
    short           bgd_fore;
    short           bgd_back;

    chtype          color_bgd;
    chtype          color_bgd_hi;
    chtype          color_bgd_rev;

    chtype          color_bgd_red;
    chtype          color_bgd_green;
    chtype          color_bgd_yellow;
    chtype          color_bgd_magenta;
    chtype          color_bgd_cyan;

    chtype          color_rule_bar;
} vc_cnf;

vc_cnf vccnf;

/* setting defaults */
#define DEFAULT_NEWRULE_LOG         1
#define DEFAULT_NEWRULE_LOGLIMIT    20

#define DEFAULT_LOGVIEW_BUFFERSIZE  500

/* default not in advanced mode */
#define DEFAULT_ADVANCED_MODE       0

/* default print mainmenu_status */
#define DEFAULT_MAINMENU_STATUS     1

#define DEFAULT_IPTRAFVOL_LOCATION  "/usr/bin/iptrafvol.pl"

struct VuurmuurStatus_
{
    d_list  StatusList;

    int     vuurmuur;
    int     vuurmuur_log;
    
    int     zones;
    int     services;
    int     interfaces;
    int     rules;

    /* connections with vuurmuur and vuurmuur_log */
    int     shm;
    /* backend data */
    int     backend;
    /* vuurmuur config */
    int     config;
    /* vuurmuur_conf settings */
    int     settings;
    /* system stuff */
    int     system;

    /* this one is checked for the header */
    int     overall;

    char    have_shape_rules;
    char    have_shape_ifaces;
} VuurmuurStatus;


/* TODO remove this */
WINDOW *status_frame_win, *status_win, *top_win, *main_win, *mainlog_win;


/*
    shared memory id and semaphore id
*/

/* vuurmuur */
int                 vuurmuur_shmid;
int                 vuurmuur_semid;
/*@null@*/
struct SHM_TABLE    *vuurmuur_shmtable;
char                *vuurmuur_shmp;
pid_t               vuurmuur_pid;

/* vuurmuur_log */
int                 vuurmuurlog_shmid;
int                 vuurmuurlog_semid;
char                *vuurmuurlog_shmp;
/*@null@*/
struct SHM_TABLE    *vuurmuurlog_shmtable;
pid_t               vuurmuurlog_pid;

char                version_string[128];

VR_user_t           user_data;

int                 utf8_mode;





/*
 *
 * FUNCTION PROTOTYPES
 *
 */


/*
    main
*/
void print_in_middle(WINDOW *win, int starty, int startx, int width, char *string, chtype color);
WINDOW *create_newwin(int height, int width, int starty, int startx, /*@null@*/ char *title, chtype ch);
void destroy_win(WINDOW *local_win);
int startup_screen(const int, Rules *, Zones *, Services *, Interfaces *, BlockList *, struct rgx_ *);
void draw_field_active_mark(const FIELD *cur, const FIELD *prev, WINDOW *formwin, FORM *form, chtype ch);
int copy_field2buf(char *buf, char *fieldbuf, size_t bufsize);
int protectrule_loaded(const int, d_list *, char *, char *, char *);


/*
    topmenu
*/
void draw_top_menu(const int, WINDOW *, char *, int, char **, int, char **);


/*
    services section
*/
void services_section(const int, Services *, Rules *, struct rgx_ *);


/*
    zones section
*/
int zones_section(const int, Zones *, Interfaces *, Rules *, BlockList *, struct rgx_ *);
int zones_blocklist(const int, BlockList *, Zones *, struct rgx_ *);
int zones_blocklist_add_one(const int, BlockList *, Zones *);


/*
    rules_section
*/
int rules_form(const int, Rules *, Zones *, Interfaces *, Services *, struct rgx_ *);
int delete_rule(const int, Rules *, unsigned int, int);
int insert_new_rule(const int, Rules *, unsigned int, const char *);
int edit_rule(const int, Rules *, Zones *, Interfaces *, Services *, unsigned int, struct rgx_ *);
int edit_rule_normal(const int, Zones *, Interfaces *, Services *, struct RuleData_ *, unsigned int, struct rgx_ *);
char *VrShapeUnitMenu(const int, char *, int, int, char);


/*
    io
*/
FILE *vuurmuur_rulesfile_open(const char *path, const char *mode, int caller);
int vuurmuur_rulesfile_close(FILE *stream, const char *path);
int write_rulesfile(const int, char *, Rules *);


/*
    templates
*/
int confirm(char *title, char *text, chtype forecolor, chtype backcolor, int def);
char *input_box(size_t length, char *title, char *description);
int vuumuurconf_print_error(int error_no, char *title, char *fmt, ...);
int vuumuurconf_print_warning(char *title, char *fmt, ...);
int vuumuurconf_print_info(char *title, char *fmt, ...);
char *selectbox(char *title, char *text, size_t n_choices, char **choices, unsigned int cols, char *);
int status_print(WINDOW *local_win, /*@null@*/ char *fmt, ...);
int check_box(int status, char *title, char *description);
/* fixes */
void set_field_buffer_wrap(const int, FIELD *, int, const char *);
FIELD *new_field_wrap(int rows, int cols, int frow, int fcol, int nrow, int nbuf);

//void filter_setup(const int, vr_filter *);
//void filter_cleanup(const int, vr_filter *);
int filter_input_box(const int, VR_filter *);


/*
    config
*/
int edit_genconfig(const int debuglvl);
int edit_logconfig(const int debuglvl);
int config_menu(const int debuglvl);
int edit_sysopt(int debuglvl);


/*
    logview section
*/
int logview_section(const int, struct vuurmuur_config *, Zones *, BlockList *, Interfaces *, Services *, /*@null@*/ char *);


/*
    interfaces section
*/
void interfaces_section(const int, Interfaces *, Zones *, Rules *, struct rgx_ *reg);


/*
    navigation
*/
int nav_field_comment(const int, FORM *, int);
int nav_field_simpletext(const int, FORM *, int);
int nav_field_yesno(const int, FORM *, int);
int nav_field_toggleX(const int, FORM *, int);
int validate_commentfield(const int, char *, regex_t *);


/*
    status section
*/
int status_section(const int, struct vuurmuur_config *, Zones *, Interfaces *, Services *);


/*
    connections
*/
int connections_section(const int, struct vuurmuur_config *, Zones *, Interfaces *, Services *, BlockList *);


/*
    help/status
*/
void print_help(const int debuglvl, char *part);
void print_status(const int debuglvl);
int read_helpline(const int debuglvl, d_list *help_list, char *line);
int setup_statuslist(const int debuglvl);


/*
    config
*/
int init_vcconfig(const int debuglvl, char *configfile_location, vc_cnf *cnf);
int write_vcconfigfile(const int debuglvl, char *file_location, vc_cnf *cnf);
int edit_vcconfig(const int debuglvl);
int vcconfig_use_defaults(const int debuglvl, vc_cnf *cnf);


/*
    main menu
*/
int main_menu(const int, Rules *,  Zones *, Interfaces *, Services *, BlockList *, struct rgx_ *);
void mm_status_checkall(int, d_list *, Rules *, Zones *, Interfaces *, Services *);
int vc_apply_changes(const int debuglvl);

/*
    bandwidth
*/
int trafvol_section(const int, Zones *, Interfaces *, Services *);

/*
    about
*/
void print_about(const int debuglvl);


/* statevent */
#define STATEVENTTYPE_LOG   1
#define STATEVENTTYPE_CONN  2
typedef struct LogRule_
{
    char filtered;

    char month[4];
    char date[3];
    char time[10];

    char action[16];

    char service[MAX_SERVICE];

    char from[MAX_HOST_NET_ZONE];
    char to[MAX_HOST_NET_ZONE];

    char prefix[32];

    char details[256];
} LogRule;

typedef struct ct_
{
    /* hashes for the vuurmuur names */
    Hash                    zone_hash,
                            service_hash;

    d_list                  network_list;

    d_list                  conn_list;

    struct ConntrackStats_  conn_stats;

    unsigned int            prev_list_size;
} Conntrack;

int kill_connections_by_ip(const int debuglvl, struct vuurmuur_config *cnf, Conntrack *ct, char *srcip, char *dstip, char *sername, char connect_status);
int block_and_kill(const int debuglvl, Conntrack *ct, Zones *zones, BlockList *blocklist, Interfaces *interfaces, char *ip);
int kill_connection(const int debuglvl, char *cmd, char *srcip, char *dstip, int proto, int sp, int dp);
int kill_connections_by_name(const int debuglvl, struct vuurmuur_config *cnf, Conntrack *ct, char *srcname, char *dstname, char *sername, char connect_status);

Conntrack *conn_init_ct(const int debuglvl, Zones *zones, Interfaces *interfaces, Services *services, BlockList *blocklist );
void conn_free_ct(const int debuglvl, Conntrack **ct, Zones *zones);
int conn_ct_get_connections(const int, struct vuurmuur_config *, Conntrack *, VR_ConntrackRequest *);
void conn_ct_clear_connections(const int debuglvl, Conntrack *ct);

void statevent(const int, struct vuurmuur_config *, int, d_list *, Conntrack *, VR_ConntrackRequest *, Zones *, BlockList *, Interfaces *, Services *);


/* length in chars (be it wide chars or normal chars) */
static inline size_t
StrLen(const char *s)
{
    return(mbstowcs(NULL,s,0));
}

/* length in mem regardless of wide/non-wide */
static inline size_t
StrMemLen(const char *s)
{
    return(strlen(s));
}

void fix_wide_menu(const int, MENU *, ITEM **);

void form_test (const int debuglvl);

void VrShapeRule(const int debuglvl, struct options *opt);
void VrShapeIface(const int debuglvl, struct InterfaceData_ *iface_ptr);

#ifdef USE_WIDEC
#define wsizeof(s)  sizeof(s)/sizeof(wchar_t)
#endif /* USE_WIDEC */


#endif
