/***************************************************************************
 *   Copyright (C) 2003-2007 by Victor Julien                              *
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

/*  helper functions for building the GUI

*/

#include "main.h"

void
VrBusyWinCreate(const int debuglvl)
{
    int width = 20,
        height = 5;

    vr_busywin = VrNewWin(height, width, 0, 0, vccnf.color_win);

    mvwprintw(vr_busywin->w, 2, 4, gettext("Please Wait"));
}

void
VrBusyWinShow(void)
{
    if (vr_busywin != NULL && vr_busywin->p != NULL) {
        show_panel(vr_busywin->p);
    }
    update_panels();
    doupdate();
}

void
VrBusyWinHide(void)
{
    if (vr_busywin != NULL && vr_busywin->p != NULL) {
        hide_panel(vr_busywin->p);
    }
    update_panels();
    doupdate();
}

void
VrBusyWinDelete(const int debuglvl)
{
    VrDelWin(vr_busywin);
}


VrWin *
VrNewWin(int h, int w, int y, int x, chtype cp)
{
    VrWin   *win;
    int     maxx = 0,
            maxy = 0;

    /* max screen sizes */
    getmaxyx(stdscr, maxy, maxx);

    /*  if requested, place the window in the middle of the screen */
    if(y == 0)
    {
        y = (maxy - h)/2;
    }
    if(x == 0)
    {
        x = (maxx - w)/2;
    }

    win = malloc(sizeof(VrWin));
    if ( win == NULL )
    {
        // error
        return(NULL);
    }
    memset(win, 0, sizeof(VrWin));

    win->w = newwin(h, w, y, x);
    if( win->w == NULL)
    {
        // error
        // cleanup
        return(NULL);
    }
    win->p = new_panel(win->w);
    if( win->p == NULL)
    {
        // error
        // cleanup
        return(NULL);
    }

    box(win->w, 0, 0);
    wbkgd(win->w, cp);
    keypad(win->w, TRUE);

    win->height = h;
    win->width= w;
    win->y = y;
    win->x = x;

    return(win);
}


void
VrDelWin(VrWin *win)
{
    /* cleanup window and panel */
    nodelay(win->w, FALSE);
    del_panel(win->p);
    destroy_win(win->w);

    /* free memory */
    free(win);
}


int
VrWinSetTitle(VrWin *win, char *title)
{
    size_t  len = StrLen(title);
    size_t  printstart = 0;

    if((len - 4) > win->width)
    {
        return(0);
    }

    /* */
    printstart = (win->width - len - 2)/2;

    mvwprintw(win->w, 0, (int)printstart, " %s ", title);

    return(0);
}


int
VrWinGetch(VrWin *win)
{
    return(wgetch(win->w));
}


VrMenu *
VrNewMenu(int h, int w, int y, int x, unsigned int n, chtype bg, chtype fg)
{
    VrMenu *menu;

    menu = malloc(sizeof(VrMenu));
    if ( menu == NULL )
    {
        // error
        (void)vrprint.error(-1, VR_ERR, "malloc failed");
        return(NULL);
    }
    memset(menu, 0, sizeof(VrMenu));

    menu->h = h;
    menu->w = w;
    menu->y = y;
    menu->x = x;

    menu->i = calloc(sizeof(ITEM), n);
    if ( menu->i == NULL )
    {
        // error
        (void)vrprint.error(-1, VR_ERR, "calloc failed");
        return(NULL);
    }
    memset(menu->i, 0, (sizeof(ITEM) * n));
    menu->nitems = n;

    menu->free_name = NULL;
    menu->use_namelist = FALSE;

    menu->free_desc = NULL;
    menu->use_desclist = FALSE;

    menu->fg = fg;
    menu->bg = bg;

    return(menu);
}


void
VrMenuSetupNameList(const int debuglvl, VrMenu *menu)
{
    d_list_setup(debuglvl, &menu->name, menu->free_name);
    menu->use_namelist = TRUE;
}


void
VrMenuSetupDescList(const int debuglvl, VrMenu *menu)
{
    d_list_setup(debuglvl, &menu->desc, menu->free_desc);
    menu->use_desclist = TRUE;
}


void
VrMenuSetNameFreeFunc(VrMenu *menu, void (*free_func)(void *ptr))
{
    menu->free_name = free_func;
}


void
VrMenuSetDescFreeFunc(VrMenu *menu, void (*free_func)(void *ptr))
{
    menu->free_desc = free_func;
}


void
VrDelMenu(const int debuglvl, VrMenu *menu)
{
    int i = 0;

    if (menu->m) {
        unpost_menu(menu->m);
        free_menu(menu->m);
    }

    if (menu->dw)
        destroy_win(menu->dw);

    for (i = 0; i < menu->nitems; i++) {
        free_item(menu->i[i]);
    }
    /* free items */
    if(menu->i != NULL)
        free(menu->i);

    /* clear the lists if used */
    if(menu->use_namelist == TRUE)
        d_list_cleanup(debuglvl, &menu->name);
    if(menu->use_desclist == TRUE)
        d_list_cleanup(debuglvl, &menu->desc);

    /* free memory */
    free(menu);
}

int
VrMenuAddItem(const int debuglvl, VrMenu *menu, char *name, char *desc)
{
    if(menu->cur_item >= menu->nitems)
    {
        (void)vrprint.error(-1, VR_ERR, "menu full: all %u items already added", menu->nitems);
        return(-1);
    }

    if(menu->use_namelist == TRUE)
    {
        if(d_list_append(debuglvl, &menu->name, name) == NULL)
        {
            (void)vrprint.error(-1, VR_ERR, "d_list_append failed");
            return(-1);
        }
    }
    if(menu->use_desclist == TRUE)
    {
        if(d_list_append(debuglvl, &menu->desc, desc) == NULL)
        {
            (void)vrprint.error(-1, VR_ERR, "d_list_append failed");
            return(-1);
        }
    }

    menu->i[menu->cur_item] = new_item(name, desc);
    if(menu->i[menu->cur_item] == NULL)
    {
        (void)vrprint.error(-1, VR_ERR, "new_item failed");
        return(-1);
    }
    menu->cur_item++;

    return(0);
}

int
VrMenuAddSepItem(const int debuglvl, VrMenu *menu, char *desc)
{
    if(menu->cur_item >= menu->nitems)
    {
        (void)vrprint.error(-1, VR_ERR, "menu full: all %u items already added", menu->nitems);
        return(-1);
    }

    if(menu->use_desclist == TRUE)
    {
        if(d_list_append(debuglvl, &menu->desc, desc) == NULL)
        {
            (void)vrprint.error(-1, VR_ERR, "d_list_append failed");
            return(-1);
        }
    }

    menu->i[menu->cur_item] = new_item(" ", desc);
    if(menu->i[menu->cur_item] == NULL)
    {
        (void)vrprint.error(-1, VR_ERR, "new_item failed");
        return(-1);
    }
    item_opts_off(menu->i[menu->cur_item], O_SELECTABLE);

    menu->cur_item++;

    return(0);
}


int
VrMenuConnectToWin(const int debuglvl, VrMenu *menu, VrWin *win)
{
    int result;

    menu->m = new_menu((ITEM **)menu->i);
    if ( menu->m == NULL )
    {
        // error
        (void)vrprint.error(-1, VR_ERR, "new_menu failed");
        return(-1);
    }
    result = set_menu_win(menu->m, win->w);
    if(result != E_OK)
    {
        (void)vrprint.error(-1, VR_ERR, "set_menu_win failed");
        return(-1);
    }

    menu->dw = derwin(win->w, menu->h, menu->w, menu->y, menu->x);
    if(menu->dw == NULL)
    {
        (void)vrprint.error(-1, VR_ERR, "derwin failed");
        return(-1);
    }

    result = set_menu_sub(menu->m, menu->dw);
    if(result != E_OK)
    {
        (void)vrprint.error(-1, VR_ERR, "set_menu_sub failed");
        return(-1);
    }
    result = set_menu_format(menu->m, win->height - 2, 1);
    if(result != E_OK)
    {
        if(result == E_BAD_ARGUMENT)
        {
            (void)vrprint.error(-1, VR_ERR, "set_menu_format failed: E_BAD_ARGUMENT");
        }
        else if(result == E_SYSTEM_ERROR)
        {
            (void)vrprint.error(-1, VR_ERR, "set_menu_format failed: E_SYSTEM_ERROR");
        }
        else if(result == E_POSTED)
        {
            (void)vrprint.error(-1, VR_ERR, "set_menu_format failed: E_POSTED");
        }
        else
        {
            (void)vrprint.error(-1, VR_ERR, "set_menu_format failed: unknown error");
        }

        return(-1);
    }

    set_menu_back(menu->m, menu->bg);
    set_menu_grey(menu->m, menu->bg);
    set_menu_fore(menu->m, menu->fg);

    return(0);
}


/*  default keys:
        up
        down
        pageup
        pagedown
        home
        end

    returns TRUE if the key matched, false if not
*/
char
VrMenuDefaultNavigation(const int debuglvl, VrMenu *menu, int key)
{
    char    match = FALSE;

    switch(key)
    {
        case KEY_DOWN:
            menu_driver(menu->m, REQ_DOWN_ITEM);
            match = TRUE;
            break;
        case KEY_UP:
            menu_driver(menu->m, REQ_UP_ITEM);
            match = TRUE;
            break;
        case KEY_NPAGE:
            if(menu_driver(menu->m, REQ_SCR_DPAGE) != E_OK)
            {
                while(menu_driver(menu->m, REQ_DOWN_ITEM) == E_OK);
            }
            match = TRUE;
            break;
        case KEY_PPAGE:
            if(menu_driver(menu->m, REQ_SCR_UPAGE) != E_OK)
            {
                while(menu_driver(menu->m, REQ_UP_ITEM) == E_OK);
            }
            match = TRUE;
            break;
        case KEY_HOME:
            menu_driver(menu->m, REQ_FIRST_ITEM);
            match = TRUE;
            break;
        case KEY_END:
            menu_driver(menu->m, REQ_LAST_ITEM);
            match = TRUE;
            break;
        case 9: /* TAB */
            menu_driver(menu->m, REQ_NEXT_ITEM);
            match = TRUE;
            break;
    }

    return(match);
}


int
VrMenuPost(const int debuglvl, VrMenu *menu)
{
    int result = 0;

    result = post_menu(menu->m);
    if(result != E_OK)
    {
        if(result == E_BAD_ARGUMENT)
        {
            (void)vrprint.error(-1, VR_ERR, "post_menu failed: E_BAD_ARGUMENT");
        }
        else if(result == E_SYSTEM_ERROR)
        {
            (void)vrprint.error(-1, VR_ERR, "post_menu failed: E_SYSTEM_ERROR");
        }
        else if(result == E_POSTED)
        {
            (void)vrprint.error(-1, VR_ERR, "post_menu failed: E_POSTED");
        }
        else if(result == E_BAD_STATE)
        {
            (void)vrprint.error(-1, VR_ERR, "post_menu failed: E_BAD_STATE");
        }
        else if(result == E_NO_ROOM)
        {
            (void)vrprint.error(-1, VR_ERR, "post_menu failed: E_NO_ROOM");
        }
        else if(result == E_NOT_POSTED)
        {
            (void)vrprint.error(-1, VR_ERR, "post_menu failed: E_NOT_POSTED");
        }
        else if(result == E_NOT_CONNECTED)
        {
            (void)vrprint.error(-1, VR_ERR, "post_menu failed: E_NOT_CONNECTED");
        }
        else
        {
            (void)vrprint.error(-1, VR_ERR, "post_menu failed: unknown error %d", result);
        }

        return(-1);
    }

    return(0);
}

int
VrMenuUnPost(const int debuglvl, VrMenu *menu)
{
    int result = 0;

    result = unpost_menu(menu->m);
    if(result != E_OK)
    {
        if(result == E_BAD_ARGUMENT)
        {
            (void)vrprint.error(-1, VR_ERR, "unpost_menu failed: E_BAD_ARGUMENT");
        }
        else if(result == E_SYSTEM_ERROR)
        {
            (void)vrprint.error(-1, VR_ERR, "unpost_menu failed: E_SYSTEM_ERROR");
        }
        else if(result == E_POSTED)
        {
            (void)vrprint.error(-1, VR_ERR, "unpost_menu failed: E_POSTED");
        }
        else if(result == E_BAD_STATE)
        {
            (void)vrprint.error(-1, VR_ERR, "unpost_menu failed: E_BAD_STATE");
        }
        else if(result == E_NO_ROOM)
        {
            (void)vrprint.error(-1, VR_ERR, "unpost_menu failed: E_NO_ROOM");
        }
        else if(result == E_NOT_POSTED)
        {
            (void)vrprint.error(-1, VR_ERR, "unpost_menu failed: E_NOT_POSTED");
        }
        else if(result == E_NOT_CONNECTED)
        {
            (void)vrprint.error(-1, VR_ERR, "unpost_menu failed: E_NOT_CONNECTED");
        }
        else
        {
            (void)vrprint.error(-1, VR_ERR, "unpost_menu failed: unknown error %d", result);
        }

        return(-1);
    }

    return(0);
}

VrForm *
VrNewForm(int h, int w, int y, int x, unsigned int n, chtype bg, chtype fg)
{
    VrForm *form;

    form = malloc(sizeof(VrForm));
    if ( form == NULL )
    {
        // error
        (void)vrprint.error(-1, VR_ERR, "malloc failed");
        return(NULL);
    }
    memset(form, 0, sizeof(VrForm));

    form->h = h;
    form->w = w;
    form->y = y;
    form->x = x;

    form->nfields = n + 2; /* for ok and cancel */
    form->fields = calloc(sizeof(FIELD), form->nfields + 1);
    if ( form->fields == NULL )
    {
        // error
        (void)vrprint.error(-1, VR_ERR, "calloc failed");
        return(NULL);
    }
    memset(form->fields, 0, (sizeof(FIELD) * form->nfields + 1));

    form->fg = fg;
    form->bg = bg;

    form->save = NULL;
    form->save_ctx = NULL;

    return(form);
}

void
VrDelForm(const int debuglvl, VrForm *form)
{
    int i = 0;

    if (form->f) {
        unpost_form(form->f);
        free_form(form->f);
    }

    if (form->dw)
        destroy_win(form->dw);

    for (i = 0; i < form->nfields; i++) {
        free_field(form->fields[i]);
    }
    /* free items */
    if(form->fields != NULL)
        free(form->fields);

    /* free memory */
    free(form);
}

int
VrFormPost(const int debuglvl, VrForm *form)
{
    int result = 0;

    result = post_form(form->f);
    if(result != E_OK)
    {
        if(result == E_BAD_ARGUMENT)
        {
            (void)vrprint.error(-1, VR_ERR, "post_form failed: E_BAD_ARGUMENT");
        }
        else if(result == E_SYSTEM_ERROR)
        {
            (void)vrprint.error(-1, VR_ERR, "post_form failed: E_SYSTEM_ERROR");
        }
        else if(result == E_POSTED)
        {
            (void)vrprint.error(-1, VR_ERR, "post_form failed: E_POSTED");
        }
        else if(result == E_BAD_STATE)
        {
            (void)vrprint.error(-1, VR_ERR, "post_form failed: E_BAD_STATE");
        }
        else if(result == E_NO_ROOM)
        {
            (void)vrprint.error(-1, VR_ERR, "post_form failed: E_NO_ROOM");
        }
        else if(result == E_NOT_POSTED)
        {
            (void)vrprint.error(-1, VR_ERR, "post_form failed: E_NOT_POSTED");
        }
        else if(result == E_NOT_CONNECTED)
        {
            (void)vrprint.error(-1, VR_ERR, "post_form failed: E_NOT_CONNECTED");
        }
        else
        {
            (void)vrprint.error(-1, VR_ERR, "post_form failed: unknown error %d", result);
        }

        return(-1);
    }

    return(0);
}

int
VrFormUnPost(const int debuglvl, VrForm *form)
{
    int result = 0;

    result = unpost_form(form->f);
    if(result != E_OK)
    {
        if(result == E_BAD_ARGUMENT)
        {
            (void)vrprint.error(-1, VR_ERR, "unpost_form failed: E_BAD_ARGUMENT");
        }
        else if(result == E_SYSTEM_ERROR)
        {
            (void)vrprint.error(-1, VR_ERR, "unpost_form failed: E_SYSTEM_ERROR");
        }
        else if(result == E_POSTED)
        {
            (void)vrprint.error(-1, VR_ERR, "unpost_form failed: E_POSTED");
        }
        else if(result == E_BAD_STATE)
        {
            (void)vrprint.error(-1, VR_ERR, "unpost_form failed: E_BAD_STATE");
        }
        else if(result == E_NO_ROOM)
        {
            (void)vrprint.error(-1, VR_ERR, "unpost_form failed: E_NO_ROOM");
        }
        else if(result == E_NOT_POSTED)
        {
            (void)vrprint.error(-1, VR_ERR, "unpost_form failed: E_NOT_POSTED");
        }
        else if(result == E_NOT_CONNECTED)
        {
            (void)vrprint.error(-1, VR_ERR, "unpost_form failed: E_NOT_CONNECTED");
        }
        else
        {
            (void)vrprint.error(-1, VR_ERR, "unpost_form failed: unknown error %d", result);
        }

        return(-1);
    }

    return(0);
}

int
VrFormAddTextField(const int debuglvl, VrForm *form, int height, int width, int toprow, int leftcol, chtype cp, char *name, char *value)
{
    int result;

    if (StrLen(name) > width) {
        (void)vrprint.error(-1, VR_ERR, "field name length (%u) is bigger than field length (%d)", StrLen(name), width);
        return(-1);
    }

    if(form->cur_field >= form->nfields)
    {
        (void)vrprint.error(-1, VR_ERR, "form full: all %u fields already added", form->nfields);
        return(-1);
    }

    form->fields[form->cur_field] = new_field_wrap(height, width, toprow, leftcol, 0, 2);
    if(form->fields[form->cur_field] == NULL)
    {
        (void)vrprint.error(-1, VR_ERR, "new_field failed");
        return(-1);
    }

    set_field_buffer_wrap(debuglvl, form->fields[form->cur_field], 0, value);
    set_field_buffer_wrap(debuglvl, form->fields[form->cur_field], 1, name);
    set_field_buffer_wrap(debuglvl, form->fields[form->cur_field], 2, "txt");

    result = set_field_back(form->fields[form->cur_field], cp);
    if(result != E_OK)
    {
        (void)vrprint.error(-1, VR_ERR, "set_field_back failed");
        return(-1);
    }

    form->cur_field++;

    return(0);
}

int
VrFormAddLabelField(const int debuglvl, VrForm *form, int height, int width, int toprow, int leftcol, chtype cp, char *value)
{
    int result;

    if(form->cur_field >= form->nfields)
    {
        (void)vrprint.error(-1, VR_ERR, "form full: all %u fields already added", form->nfields);
        return(-1);
    }

    form->fields[form->cur_field] = new_field_wrap(height, width, toprow, leftcol, 0, 2);
    if(form->fields[form->cur_field] == NULL)
    {
        (void)vrprint.error(-1, VR_ERR, "new_field failed");
        return(-1);
    }

    set_field_buffer_wrap(debuglvl, form->fields[form->cur_field], 0, value);
    set_field_buffer_wrap(debuglvl, form->fields[form->cur_field], 1, "lbl");
    set_field_buffer_wrap(debuglvl, form->fields[form->cur_field], 2, "lbl");

    field_opts_off(form->fields[form->cur_field], O_ACTIVE);

    result = set_field_back(form->fields[form->cur_field], cp);
    if(result != E_OK)
    {
        (void)vrprint.error(-1, VR_ERR, "set_field_back failed");
        return(-1);
    }

    form->cur_field++;

    return(0);
}

int
VrFormAddCheckboxField(const int debuglvl, VrForm *form, int toprow, int leftcol, chtype cp, char *name, char enabled)
{
    int result;
    int height = 1;
    int width = 1;
    char *value = enabled ? "X" : " ";

    if (StrLen(name) > width) {
        (void)vrprint.error(-1, VR_INTERR, "field name length (%u) is bigger than field length (%d)", StrLen(name), width);
        return(-1);
    }

    /* +1 because we create two fields */
    if(form->cur_field+1 >= form->nfields)
    {
        (void)vrprint.error(-1, VR_ERR, "form full: all %u fields already added", form->nfields);
        return(-1);
    }

    /* create the field that will contain the X or be empty */
    form->fields[form->cur_field] = new_field_wrap(height, width, toprow, leftcol+1, 0, 2);
    if(form->fields[form->cur_field] == NULL)
    {
        (void)vrprint.error(-1, VR_ERR, "new_field failed");
        return(-1);
    }

    set_field_buffer_wrap(debuglvl, form->fields[form->cur_field], 0, value);
    set_field_buffer_wrap(debuglvl, form->fields[form->cur_field], 1, name);
    set_field_buffer_wrap(debuglvl, form->fields[form->cur_field], 2, "C");

    result = set_field_back(form->fields[form->cur_field], cp);
    if(result != E_OK)
    {
        (void)vrprint.error(-1, VR_ERR, "set_field_back failed");
        return(-1);
    }

    form->cur_field++;

    /* create the label [ ] */
    form->fields[form->cur_field] = new_field_wrap(height, 3, toprow, leftcol, 0, 2);
    if(form->fields[form->cur_field] == NULL)
    {
        (void)vrprint.error(-1, VR_ERR, "new_field failed");
        return(-1);
    }

    set_field_buffer_wrap(debuglvl, form->fields[form->cur_field], 0, "[ ]");
    set_field_buffer_wrap(debuglvl, form->fields[form->cur_field], 1, "lbl");
    set_field_buffer_wrap(debuglvl, form->fields[form->cur_field], 2, "lbl");

    field_opts_off(form->fields[form->cur_field], O_EDIT);
    field_opts_off(form->fields[form->cur_field], O_ACTIVE);

    result = set_field_back(form->fields[form->cur_field], cp);
    if(result != E_OK)
    {
        (void)vrprint.error(-1, VR_ERR, "set_field_back failed");
        return(-1);
    }

    form->cur_field++;

    return(0);
}

static int
VrFormAddOKCancel(const int debuglvl, VrForm *form) {
    int result;

    form->fields[form->cur_field] = new_field_wrap(1, 10, form->h - 2, 2, 0, 2);
    if(form->fields[form->cur_field] == NULL)
    {
        (void)vrprint.error(-1, VR_ERR, "new_field failed");
        return(-1);
    }

    set_field_buffer_wrap(debuglvl, form->fields[form->cur_field], 0, gettext("    OK"));
    set_field_buffer_wrap(debuglvl, form->fields[form->cur_field], 1, "save");
    set_field_buffer_wrap(debuglvl, form->fields[form->cur_field], 2, "btn");

    result = set_field_back(form->fields[form->cur_field], vccnf.color_win_green_rev | A_BOLD);
    if(result != E_OK)
    {
        (void)vrprint.error(-1, VR_ERR, "set_field_back failed");
        return(-1);
    }
    field_opts_off(form->fields[form->cur_field], O_EDIT);

    form->cur_field++;

    form->fields[form->cur_field] = new_field_wrap(1, 10, form->h - 2, 16, 0, 2);
    if(form->fields[form->cur_field] == NULL)
    {
        (void)vrprint.error(-1, VR_ERR, "new_field failed");
        return(-1);
    }

    set_field_buffer_wrap(debuglvl, form->fields[form->cur_field], 0, gettext("  Cancel"));
    set_field_buffer_wrap(debuglvl, form->fields[form->cur_field], 1, "nosave");
    set_field_buffer_wrap(debuglvl, form->fields[form->cur_field], 2, "btn");

    result = set_field_back(form->fields[form->cur_field], vccnf.color_win_red_rev | A_BOLD);
    if(result != E_OK)
    {
        (void)vrprint.error(-1, VR_ERR, "set_field_back failed");
        return(-1);
    }
    field_opts_off(form->fields[form->cur_field], O_EDIT);

    form->cur_field++;

    return(0);
}

int
VrFormConnectToWin(const int debuglvl, VrForm *form, VrWin *win)
{
    int result;
    int rows, cols;

    /* add OK and Cancel fields */
    VrFormAddOKCancel(debuglvl, form);

    /* we are done adding fields, so reset counter */
    form->cur_field = 0;

    form->f = new_form((FIELD **)form->fields);
    if ( form->f == NULL )
    {
        // error
        (void)vrprint.error(-1, VR_ERR, "new_form failed");
        return(-1);
    }

    result = scale_form(form->f, &rows, &cols);
    if(result != E_OK)
    {
        (void)vrprint.error(-1, VR_ERR, "scale_form failed");
        return(-1);
    }
    //(void)vrprint.debug(__FUNC__, "rows %d, cols %d", rows, cols);

    result = set_form_win(form->f, win->w);
    if(result != E_OK)
    {
        (void)vrprint.error(-1, VR_ERR, "set_form_win failed");
        return(-1);
    }

    form->dw = derwin(win->w, rows, cols, form->y, form->x);
    if(form->dw == NULL)
    {
        (void)vrprint.error(-1, VR_ERR, "derwin failed");
        return(-1);
    }

    result = set_form_sub(form->f, form->dw);
    if(result != E_OK)
    {
        (void)vrprint.error(-1, VR_ERR, "set_form_sub failed");
        return(-1);
    }

    return(0);
}

char
VrFormTextNavigation(const int debuglvl, VrForm *form, FIELD *fld, int key)
{
    char    match = FALSE;

    //(void)vrprint.info(VR_INFO, "key %d", key);

    switch(key)
    {
        case 127:
        case KEY_BACKSPACE:
            form_driver(form->f, REQ_PREV_CHAR);
            form_driver(form->f, REQ_DEL_CHAR);
            form_driver(form->f, REQ_END_LINE);
            break;
        case KEY_DC:
            form_driver(form->f, REQ_PREV_CHAR);
            form_driver(form->f, REQ_DEL_CHAR);
            form_driver(form->f, REQ_END_LINE);
            break;
        case KEY_RIGHT:
            form_driver(form->f, REQ_NEXT_CHAR);
            match = TRUE;
            break;
        case KEY_LEFT:
            form_driver(form->f, REQ_PREV_CHAR);
            match = TRUE;
            break;
        case KEY_DOWN:
            form_driver(form->f, REQ_NEXT_FIELD);
            match = TRUE;
            break;
        case KEY_UP:
            form_driver(form->f, REQ_PREV_FIELD);
            match = TRUE;
            break;
        case KEY_NPAGE:
            if(form_driver(form->f, REQ_NEXT_PAGE) != E_OK)
            {
                while(form_driver(form->f, REQ_NEXT_FIELD) == E_OK);
            }
            match = TRUE;
            break;
        case KEY_PPAGE:
            if(form_driver(form->f, REQ_PREV_PAGE) != E_OK)
            {
                while(form_driver(form->f, REQ_PREV_FIELD) == E_OK);
            }
            match = TRUE;
            break;
/* TODO last pos in field */
        case KEY_HOME:
            form_driver(form->f, REQ_BEG_LINE);
            match = TRUE;
            break;
        case KEY_END:
            form_driver(form->f, REQ_END_LINE);
            match = TRUE;
            break;
        case 9: /* TAB */
            form_driver(form->f, REQ_NEXT_FIELD);
            match = TRUE;
            break;
        default:
            //(void)vrprint.info(VR_INFO, "default");
            if (isprint(key)) {
                //(void)vrprint.info(VR_INFO, "default %d", key);
                form_driver(form->f, key);
                match = TRUE;
            }
            break;
    }

    return(match);
}

char
VrFormCheckboxNavigation(const int debuglvl, VrForm *form, FIELD *fld, int key)
{
    char    match = FALSE;

    //(void)vrprint.info(VR_INFO, "key %d", key);

    switch(key)
    {
        case KEY_DOWN:
            form_driver(form->f, REQ_NEXT_FIELD);
            match = TRUE;
            break;
        case KEY_UP:
            form_driver(form->f, REQ_PREV_FIELD);
            match = TRUE;
            break;
        case KEY_NPAGE:
            if(form_driver(form->f, REQ_NEXT_PAGE) != E_OK)
            {
                while(form_driver(form->f, REQ_NEXT_FIELD) == E_OK);
            }
            match = TRUE;
            break;
        case KEY_PPAGE:
            if(form_driver(form->f, REQ_PREV_PAGE) != E_OK)
            {
                while(form_driver(form->f, REQ_PREV_FIELD) == E_OK);
            }
            match = TRUE;
            break;
        case 9: /* TAB */
            form_driver(form->f, REQ_NEXT_FIELD);
            match = TRUE;
            break;
        case 32: /* SPACE */
        {
            char *buf = field_buffer(fld, 0);
            if (strncmp(buf, "X", 1) == 0) {
                set_field_buffer_wrap(debuglvl, fld, 0, " ");
            } else {
                set_field_buffer_wrap(debuglvl, fld, 0, "X");
            }

            match = TRUE;
            break;
        }
    }

    return(match);
}

/*  default keys:
        up
        down
        pageup
        pagedown
        home
        end

    returns TRUE if the key matched, false if not
*/
char
VrFormDefaultNavigation(const int debuglvl, VrForm *form, int key)
{
    char    match = FALSE;
    FIELD   *fld = NULL;
    char    *buf = NULL;

    //(void)vrprint.debug(__FUNC__, "key %d", key);

    fld = current_field(form->f);
    if (fld == NULL)
        return(FALSE);

    buf = field_buffer(fld, 2);
    if (buf == NULL)
        return(FALSE);

    if (strncmp(buf, "txt", 3) == 0)
        return(VrFormTextNavigation(debuglvl, form, fld, key));
    else if (strncmp(buf, "C", 1) == 0)
        return(VrFormCheckboxNavigation(debuglvl, form, fld, key));

    switch(key)
    {
        case KEY_RIGHT:
            form_driver(form->f, REQ_NEXT_CHAR);
            match = TRUE;
            break;
        case KEY_LEFT:
            form_driver(form->f, REQ_PREV_CHAR);
            match = TRUE;
            break;
        case KEY_DOWN:
            form_driver(form->f, REQ_NEXT_FIELD);
            match = TRUE;
            break;
        case KEY_UP:
            form_driver(form->f, REQ_PREV_FIELD);
            match = TRUE;
            break;
        case KEY_NPAGE:
            if(form_driver(form->f, REQ_NEXT_PAGE) != E_OK)
            {
                while(form_driver(form->f, REQ_NEXT_FIELD) == E_OK);
            }
            match = TRUE;
            break;
        case KEY_PPAGE:
            if(form_driver(form->f, REQ_PREV_PAGE) != E_OK)
            {
                while(form_driver(form->f, REQ_PREV_FIELD) == E_OK);
            }
            match = TRUE;
            break;
/* TODO last pos in field */
        case KEY_HOME:
            form_driver(form->f, REQ_FIRST_FIELD);
            match = TRUE;
            break;
        case KEY_END:
            form_driver(form->f, REQ_LAST_FIELD);
            match = TRUE;
            break;
        case 9: /* TAB */
            form_driver(form->f, REQ_NEXT_FIELD);
            match = TRUE;
            break;
    }

    return(match);
}

int
VrFormGetFields(const int debuglvl, VrForm *form, char *name, size_t nlen, char *value, size_t vlen)
{
    FIELD *field = NULL;
    char *n = NULL, *v = NULL;

    if (form->cur_field >= form->nfields) {
        form->cur_field = 0;
        return(0);
    }

    /* skip past non active (label) fields,
     * but not past non visible fields */
    while (form->cur_field < form->nfields) {
        int opts = field_opts(form->fields[form->cur_field]);
        char *buf = field_buffer(form->fields[form->cur_field], 2);

        if (buf != NULL && (strncmp(buf, "btn", 3) == 0)) {
            //(void)vrprint.info(VR_INFO, "button field");
        } else if(!(opts & O_ACTIVE)) {
            //(void)vrprint.info(VR_INFO, "inactive field");
        } else {
            break;
        }

        form->cur_field++;
    }
    if (form->cur_field >= form->nfields)
        return(0);

    field = form->fields[form->cur_field];
    form->cur_field++;

    n = field_buffer(field, 1);
    copy_field2buf(name, n, nlen);
    v = field_buffer(field, 0);
    copy_field2buf(value, v, vlen);

    //(void)vrprint.info(VR_INFO, "name %s value %s", name, value);
    return(1);
}

int
VrFormCheckOKCancel(const int debuglvl, VrForm *form, int key)
{
    FIELD *fld = NULL;
    char *buf = NULL;

    //(void)vrprint.debug(__FUNC__, "key %d", key);

    fld = current_field(form->f);
    if (fld == NULL)
        return(-1);

    buf = field_buffer(fld, 1);
    if (buf == NULL)
        return(-1);

    if(key == 27 || key == KEY_F(10) || (key == 10 && strncmp(buf,"save", 4) == 0))
    {
        if (form->save != NULL && form->save_ctx != NULL) {
            char name[32] = "", value[32] = "";

            /* save */
            while((VrFormGetFields(debuglvl, form, name, sizeof(name), value, sizeof(value)) == 1)) {
                form->save(debuglvl, form->save_ctx, name, value);
            }
        }

        return(1);

    }
    else if(key == 10 && strncmp(buf,"nosave", 6) == 0)
    {
        return(1);
    }

    return(0);
}

void
VrFormDrawMarker(const int debuglvl, VrWin *win, VrForm *form) {
    int pos_x,
        pos_y,
        x,
        y,
        off_row,
        wrk_buff;
    int ch = vccnf.color_win_mark|A_BOLD;

    form->prev = form->cur;
    form->cur  = current_field(form->f);

    /* if supplied we remove the previous marking */
    if(form->prev)
    {
        if(field_info(form->prev, &y, &x, &pos_y, &pos_x, &off_row, &wrk_buff) < 0)
            return;

        mvwprintw(win->w, pos_y + 1, pos_x - 1, " ");
        mvwprintw(win->w, pos_y + 1, pos_x + x + 2, " ");
    }

    /* draw our marking */
    if(field_info(form->cur, &y, &x, &pos_y, &pos_x, &off_row, &wrk_buff) < 0)
        return;

    wattron(win->w, ch);
    mvwprintw(win->w, pos_y + 1, pos_x - 1, ">");
    mvwprintw(win->w, pos_y + 1, pos_x + x + 2, "<");
    wattroff(win->w, ch);
    wrefresh(win->w);

    /* restore cursor position */
    pos_form_cursor(form->f);
    return;
}

int
VrFormSetSaveFunc(const int debuglvl, VrForm *form, int (*save)(const int debuglvl, void *ctx, char *name, char *value), void *ctx) {
    form->save_ctx = ctx;
    form->save = save;
    return(0);
}

struct cnf_ {
    char file[64];
};

int
form_test_save(const int debuglvl, void *ctx, char *name, char *value)
{
    struct cnf_ *c = (struct cnf_ *)ctx;

    (void)vrprint.info(VR_INFO, "c %p", c);

    if (strcmp(name,"test1") == 0) {
        (void)vrprint.info(VR_INFO, "%s:%s", name, value);
    } else if (strcmp(name,"test2") == 0) {
        (void)vrprint.info(VR_INFO, "%s:%s", name, value);
    }

    return(0);
}

void form_test (const int debuglvl) {
    VrWin   *win = NULL;
    VrForm  *form = NULL;
    int     ch = 0, result = 0;
    struct cnf_ config;

    strlcpy(config.file, "/tmp", sizeof(config.file));

    /* create the window and put it in the middle of the screen */
    win = VrNewWin(30,80,0,0,vccnf.color_win);
    if(win == NULL)
    {
        (void)vrprint.error(-1, VR_ERR, "VrNewWin failed");
        return;
    }
    VrWinSetTitle(win, "title");

    form = VrNewForm(20, 60, 1, 1, 4, vccnf.color_win, vccnf.color_win_rev | A_BOLD);

    VrFormSetSaveFunc(debuglvl, form, form_test_save, &config);

    VrFormAddLabelField(debuglvl, form, 1, 6, 1, 1, vccnf.color_win, "Label1");
    VrFormAddTextField(debuglvl, form, 1, 10, 1, 9, vccnf.color_win_rev | A_BOLD, "test1", "test1");
    VrFormAddLabelField(debuglvl, form, 1, 6, 3, 1, vccnf.color_win, "Label2");
    VrFormAddTextField(debuglvl, form, 1, 10, 3, 9, vccnf.color_win_rev | A_BOLD, "test2", "test2");

    VrFormConnectToWin(debuglvl, form, win);

    VrFormPost(debuglvl, form);

    update_panels();
    doupdate();

    /* user input */
    char quit = FALSE;
    while(quit == FALSE)
    {
        ch = VrWinGetch(win);

        /* check OK/Cancel buttons */
        result = VrFormCheckOKCancel(debuglvl, form, ch);
        if (result == -1 || result == 1) {
            break;
        }

        if (VrFormDefaultNavigation(debuglvl, form, ch) == FALSE) {
            switch(ch)
            {
                case KEY_DOWN:
                case 10: // enter
                    form_driver(form->f, REQ_NEXT_FIELD);
                    form_driver(form->f, REQ_BEG_LINE);
                    break;
                case 27:
                case 'q':
                case 'Q':
                case KEY_F(10):
                    quit = TRUE;
                    break;
                case KEY_F(12):
                case 'h':
                case 'H':
                case '?':
                    print_help(debuglvl, ":[VUURMUUR:INTERFACES:SHAPE]:");
                    break;
            }
        }
    }
}

