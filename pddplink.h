/* Copyright (c) 2005 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */


#ifndef _pddplink_h
#define _pddplink_h
#include "m_pd.h"
#include "g_canvas.h"

typedef t_rtext *(*t_glist_rtext_fn)(t_glist *, t_text *);
typedef void (*t_rtext_getrect_fn)(t_rtext *x, int *x1p, int *y1p, int *x2p, int *y2p);
typedef int (*t_rtext_dimen_fn)(t_rtext *x);
typedef void (*t_getrect_dimen_fn)(t_rtext *x, int *w, int *h);

static t_glist_rtext_fn _glist_getrtext;
static t_rtext_dimen_fn _rtext_width, _rtext_height;
static t_rtext_getrect_fn _rtext_getrect;
static t_getrect_dimen_fn _getrect_dimen;


static void _getrect_dummy (t_rtext *y, int *width, int *height) {
  (void)y;
  *width = 100;
  *height = 25;
}
static void _getrect_legacy (t_rtext *y, int *width, int *height) {
  *width = _rtext_width(y);
  *height = _rtext_height(y);
}
static void _getrect_056 (t_rtext *y, int *width, int *height) {
  int x1, y1, x2, y2;
  _rtext_getrect(y, &x1, &y1, &x2, &y2);
  *width = x2 - x1;
  *height = y2 - y1;
}

static int pddplink_compatfuns(void) {
    _glist_getrtext = (t_glist_rtext_fn)sys_getfunbyname("glist_getrtext");
    if (!_glist_getrtext)
      _glist_getrtext = (t_glist_rtext_fn)sys_getfunbyname("glist_findrtext");
    _rtext_width = (t_rtext_dimen_fn)sys_getfunbyname("rtext_width");
    _rtext_height = (t_rtext_dimen_fn)sys_getfunbyname("rtext_height");
    _rtext_getrect = (t_rtext_getrect_fn)sys_getfunbyname("rtext_getrect");

    if (!_glist_getrtext) {
      pd_error(0, "'glist_getrtext'/'glist_findrtext' missing from Pd");
      return 0;
    }
    if(_rtext_getrect)
      _getrect_dimen = _getrect_056;
    else if (_rtext_width && _rtext_height)
      _getrect_dimen = _getrect_legacy;
    else
      _getrect_dimen = _getrect_dummy;

    return 1;
}


typedef struct _pddplink
{
    t_object   x_ob;
    t_glist   *x_glist;
    int        x_isboxed;
    char      *x_vistext;
    int        x_vissize;
    int        x_vislength;
    int        x_rtextactive;
    t_symbol  *x_dirsym;
    t_symbol  *x_ulink;
    t_atom     x_openargs[2];
    int        x_linktype;
    int        x_ishit;
} t_pddplink;

#endif /* _pddplink_h */
