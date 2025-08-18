/* Copyright (c) 2005 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* This is a prototype of an active comment.  It might be replaced with
   a new core object type, T_LINK (te_type bitfield would have to be
   extended then). */

#include <stdio.h>
#include <string.h>
#include "m_pd.h"
#include "m_imp.h"  /* FIXME need access to c_externdir... */
#include "s_stuff.h"
#include "g_canvas.h"

typedef t_rtext *(*t_glist_rtext_fn)(t_glist *, t_text *);
typedef void (*t_rtext_getrect_fn)(t_rtext *x, int *x1p, int *y1p, int *x2p, int *y2p);
typedef int (*t_rtext_dimen_fn)(t_rtext *x);
typedef void (*t_getrect_dimen_fn)(t_rtext *x, int *w, int *h);

static t_glist_rtext_fn _glist_getrtext;
static t_rtext_dimen_fn _rtext_width, _rtext_height;
static t_rtext_getrect_fn _rtext_getrect;
static t_getrect_dimen_fn _getrect_dimen;


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

static t_class *pddplink_class;
static t_class *pddplinkbox_class;

/* Code that might be merged back to g_text.c starts here: */

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

static void pddplink_getrect(t_gobj *z, t_glist *glist,
			     int *xp1, int *yp1, int *xp2, int *yp2)
{
    t_pddplink *x = (t_pddplink *)z;
    int width, height;
    float x1, y1, x2, y2;
    if (glist->gl_editor && glist->gl_editor->e_rtext)
    {
	if (x->x_rtextactive)
	{
	    t_rtext *y = _glist_getrtext(glist, (t_text *)x);
	    _getrect_dimen(y, &width, &height);
	    height -= 2;
	}
	else
	{
	    int font = glist_getfont(glist);
	    width = x->x_vislength * sys_fontwidth(font) + 2;
	    height = sys_fontheight(font) + 2;
	}
    }
    else width = height = 10;
    x1 = text_xpix((t_text *)x, glist);
    y1 = text_ypix((t_text *)x, glist);
    x2 = x1 + width;
    y2 = y1 + height;
    y1 += 1;
    *xp1 = x1;
    *yp1 = y1;
    *xp2 = x2;
    *yp2 = y2;
}

static void pddplink_displace(t_gobj *z, t_glist *glist, int dx, int dy)
{
    t_text *t = (t_text *)z;
    t->te_xpix += dx;
    t->te_ypix += dy;
    if (glist_isvisible(glist))
    {
        t_rtext *y = _glist_getrtext(glist, t);
        rtext_displace(y, dx, dy);
    }
}

static void pddplink_select(t_gobj *z, t_glist *glist, int state)
{
    t_pddplink *x = (t_pddplink *)z;
    t_rtext *y = _glist_getrtext(glist, (t_text *)x);
    rtext_select(y, state);
    if (state)
      pdgui_vmess(0, "crs rk", glist, "itemconfigure", rtext_gettag(y)
		  , "-fill", 0x0000FF
	);
    else
      pdgui_vmess(0, "crs rs rk rk", glist, "itemconfigure", rtext_gettag(y)
		  , "-text", x->x_vistext
		  , "-fill", 0x0000DD
		  , "-activefill", 0x700000
	);
}

static void pddplink_activate(t_gobj *z, t_glist *glist, int state)
{
    t_pddplink *x = (t_pddplink *)z;
    t_rtext *y = _glist_getrtext(glist, (t_text *)x);
    rtext_activate(y, state);
    x->x_rtextactive = state;
}

static void pddplink_vis(t_gobj *z, t_glist *glist, int vis)
{
    t_pddplink *x = (t_pddplink *)z;
    t_rtext *y = _glist_getrtext(glist, (t_text *)x);
    if (vis)
    {
        rtext_draw(y);
	pdgui_vmess(0, "crs rs rk rk", glist_getcanvas(glist), "itemconfigure", rtext_gettag(y)
		  , "-text", x->x_vistext
		  , "-fill", 0x0000DD
		  , "-activefill", 0x700000
	);
    }
    else
        rtext_erase(y);
}

static int pddplink_wbclick(t_gobj *z, t_glist *glist, int xpix, int ypix,
			    int shift, int alt, int dbl, int doit);

static t_widgetbehavior pddplink_widgetbehavior =
{
    pddplink_getrect,
    pddplink_displace,
    pddplink_select,
    pddplink_activate,
    0,
    pddplink_vis,
    pddplink_wbclick,
};

/* Code that might be merged back to g_text.c ends here. */

/* FIXME need access to glob_pdobject... */
static t_pd *pddplink_pdtarget(t_pddplink *x)
{
    t_pd *pdtarget = gensym("pd")->s_thing;
    if (pdtarget && !strcmp(class_getname(*pdtarget), "pd"))
	return (pdtarget);
    else
	return ((t_pd *)x);  /* internal error */
}

static void pddplink_anything(t_pddplink *x, t_symbol *s, int ac, t_atom *av)
{
    if (x->x_ishit)
    {
	startpost("pddplink: internal error (%s", (s ? s->s_name : ""));
	postatom(ac, av);
	post(")");
    }
}

static void pddplink_click(t_pddplink *x, t_floatarg xpos, t_floatarg ypos,
			   t_floatarg shift, t_floatarg ctrl, t_floatarg alt)
{
    (void)xpos;(void)ypos;(void)shift;(void)ctrl;(void)alt;

    x->x_ishit = 1;
    pdgui_vmess("pddplink_open", "ss"
		, x->x_ulink->s_name
		, x->x_dirsym->s_name
      );
    x->x_ishit = 0;
}

static void pddplink_bang(t_pddplink *x)
{
  pddplink_click(x, 0, 0, 0, 0, 0);
}

static int pddplink_wbclick(t_gobj *z, t_glist *glist, int xpix, int ypix,
			    int shift, int alt, int dbl, int doit)
{
    (void)glist;(void)dbl;
    t_pddplink *x = (t_pddplink *)z;
    if (doit)
    {
        pddplink_click(x, (t_floatarg)xpix, (t_floatarg)ypix,
                       (t_floatarg)shift, 0, (t_floatarg)alt);
	return (1);
    }
    else
        return (0);
}

static int pddplink_isoption(const char *name)
{
    if (*name == '-')
    {
	char c = name[1];
	return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
    }
    else return (0);
}

static t_symbol *pddplink_nextsymbol(int ac, t_atom *av, int opt, int *skipp)
{
    int ndx;
    for (ndx = 0; ndx < ac; ndx++, av++)
    {
	if (av->a_type == A_SYMBOL &&
	    (!opt || pddplink_isoption(av->a_w.w_symbol->s_name)))
	{
	    *skipp = ++ndx;
	    return (av->a_w.w_symbol);
	}
    }
    return (0);
}

static int pddplink_dooptext(char *dst, int maxsize, int ac, t_atom *av)
{
    int i, sz, sep, len;
    char buf[32];
    const char *src;
    for (i = 0, sz = 0, sep = 0; i < ac; i++, av++)
    {
	if (sep)
	{
	    sz++;
	    if (sz >= maxsize)
		break;
	    else if (dst)
	    {
		*dst++ = ' ';
		*dst = 0;
	    }
	}
	else sep = 1;
	if (av->a_type == A_SYMBOL)
	    src = av->a_w.w_symbol->s_name;
	else if (av->a_type == A_FLOAT)
	{
	    src = buf;
	    sprintf(buf, "%g", av->a_w.w_float);
	}
	else
	{
	    sep = 0;
	    continue;
	}
	len = strlen(src);
	sz += len;
	if (sz >= maxsize)
	    break;
	else if (dst)
	{
	    strcpy(dst, src);
	    dst += len;
	}
    }
    return (sz);
}

static char *pddplink_optext(int *sizep, int ac, t_atom *av)
{
    char *result;
    int sz = pddplink_dooptext(0, MAXPDSTRING, ac, av);
    *sizep = sz + (sz >= MAXPDSTRING ? 4 : 1);
    result = getbytes(*sizep);
    pddplink_dooptext(result, sz + 1, ac, av);
    if (sz >= MAXPDSTRING)
    {
	sz = strlen(result);
	strcpy(result + sz, "...");
    }
    return (result);
}

static void pddplink_free(t_pddplink *x)
{
    if (x->x_vistext)
	freebytes(x->x_vistext, x->x_vissize);
}

static void *pddplink_new(t_symbol *s, int ac, t_atom *av)
{
    (void)s;
    t_pddplink xgen, *x;
    int skip;
    xgen.x_isboxed = 0;
    xgen.x_vistext = 0;
    xgen.x_vissize = 0;
    if ((xgen.x_ulink = pddplink_nextsymbol(ac, av, 0, &skip)))
    {
	t_symbol *opt;
	ac -= skip;
	av += skip;
	while ((opt = pddplink_nextsymbol(ac, av, 1, &skip)))
	{
	    ac -= skip;
	    av += skip;
	    if (opt == gensym("-box"))
		xgen.x_isboxed = 1;
	    else if (opt == gensym("-text"))
	    {
		t_symbol *nextsym = pddplink_nextsymbol(ac, av, 1, &skip);
		int natoms = (nextsym ? skip - 1 : ac);
		if (natoms)
		    xgen.x_vistext =
			pddplink_optext(&xgen.x_vissize, natoms, av);
	    }
	}
    }
    x = (t_pddplink *)
	pd_new(xgen.x_isboxed ? pddplinkbox_class : pddplink_class);
    x->x_glist = canvas_getcurrent();
    x->x_dirsym = canvas_getdir(x->x_glist);  /* FIXME */

    x->x_isboxed = xgen.x_isboxed;
    x->x_vistext = xgen.x_vistext;
    x->x_vissize = xgen.x_vissize;
    x->x_vislength = (x->x_vistext ? strlen(x->x_vistext) : 0);
    x->x_rtextactive = 0;
    if (xgen.x_ulink)
        x->x_ulink = xgen.x_ulink;
    else
        x->x_ulink = gensym("Untitled");
    SETSYMBOL(&x->x_openargs[0], x->x_ulink);
    SETSYMBOL(&x->x_openargs[1], x->x_dirsym);
    x->x_ishit = 0;
    if (x->x_isboxed)
	outlet_new((t_object *)x, &s_anything);
    else
    {
	/* do we need to set ((t_text *)x)->te_type = T_TEXT; ? */
	if (!x->x_vistext)
	{
	    x->x_vislength = strlen(x->x_ulink->s_name);
	    x->x_vissize = x->x_vislength + 1;
	    x->x_vistext = getbytes(x->x_vissize);
	    strcpy(x->x_vistext, x->x_ulink->s_name);
	}
    }
    return (x);
}

void pddplink_setup(void)
{
    _glist_getrtext = (t_glist_rtext_fn)sys_getfunbyname("glist_getrtext");
    if (!_glist_getrtext)
      _glist_getrtext = (t_glist_rtext_fn)sys_getfunbyname("glist_findrtext");
    _rtext_width = (t_rtext_dimen_fn)sys_getfunbyname("rtext_width");
    _rtext_height = (t_rtext_dimen_fn)sys_getfunbyname("rtext_height");
    _rtext_getrect = (t_rtext_getrect_fn)sys_getfunbyname("rtext_getrect");

    if (!_glist_getrtext) {
      pd_error(0, "'glist_getrtext'/'glist_findrtext' missing from Pd");
      return;
    }
    if(_rtext_getrect)
      _getrect_dimen = _getrect_056;
    else if (_rtext_width && _rtext_height)
      _getrect_dimen = _getrect_legacy;
    else
      _getrect_dimen = _getrect_dummy;



    pddplink_class = class_new(gensym("pddplink"),
			       (t_newmethod)pddplink_new,
			       (t_method)pddplink_free,
			       sizeof(t_pddplink),
			       CLASS_NOINLET | CLASS_PATCHABLE,
			       A_GIMME, 0);
    class_addanything(pddplink_class, pddplink_anything);
    class_setwidget(pddplink_class, &pddplink_widgetbehavior);

    pddplinkbox_class = class_new(gensym("pddplink"), 0,
				  (t_method)pddplink_free,
				  sizeof(t_pddplink), 0, A_GIMME, 0);
    class_addbang(pddplinkbox_class, pddplink_bang);
    class_addanything(pddplinkbox_class, pddplink_anything);
    class_addmethod(pddplinkbox_class, (t_method)pddplink_click,
		    gensym("click"),
		    A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);

    t_symbol* dirsym = pddplink_class->c_externdir;  /* FIXME */
    char tclsourcefile[MAXPDSTRING];
    pd_snprintf(tclsourcefile, MAXPDSTRING, "%s/pddplink.tcl", dirsym->s_name);
    tclsourcefile[MAXPDSTRING-1] = 0;
    pdgui_vmess("source", "s", tclsourcefile);
}
