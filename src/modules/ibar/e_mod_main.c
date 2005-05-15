/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"
#include <math.h>

/* TODO List:
 *
 * * Listen to change of main e_app!
 *
 * * Create separate config for each bar
 * * Fix menu
 *
 * * icon labels & label tooltips supported for the name of the app
 * * use part list to know how many icons & where to put in the overlay of an icon
 * * description bubbles/tooltips for icons
 * * support dynamic iconsize change on the fly
 * * app subdirs - need to somehow handle these...
 * * use overlay object and repeat events for doing auto hide/show
 * * emit signals on hide/show due to autohide/show
 * * virtualise autoshow/hide to later allow for key bindings, mouse events elsewhere, ipc and other singals to show/hide
 *
 */

static int bar_count;
static E_Config_DD *conf_edd;
static E_Config_DD *conf_bar_edd;

static int drag, drag_start;
static int drag_x, drag_y;

/* const strings */
static const char *_ibar_main_orientation[] =
{"left", "right", "top", "bottom"};

/* module private routines */
static IBar   *_ibar_new();
static void    _ibar_free(IBar *ib);
static void    _ibar_app_change(void *data, E_App *a, E_App_Change ch);
static void    _ibar_config_menu_new(IBar *ib);

static IBar_Bar *_ibar_bar_new(IBar *ib, E_Container *con);
static void    _ibar_bar_free(IBar_Bar *ibb);
static void    _ibar_bar_menu_new(IBar_Bar *ibb);
static void    _ibar_bar_enable(IBar_Bar *ibb);
static void    _ibar_bar_disable(IBar_Bar *ibb);
static void    _ibar_bar_frame_resize(IBar_Bar *ibb);
static void    _ibar_bar_edge_change(IBar_Bar *ibb, int edge);
static void    _ibar_bar_update_policy(IBar_Bar *ibb);
static void    _ibar_bar_motion_handle(IBar_Bar *ibb, Evas_Coord mx, Evas_Coord my);
static void    _ibar_bar_timer_handle(IBar_Bar *ibb);
static void    _ibar_bar_follower_reset(IBar_Bar *ibb);

static IBar_Icon *_ibar_icon_new(IBar_Bar *ibb, E_App *a);
static void    _ibar_icon_free(IBar_Icon *ic);
static IBar_Icon *_ibar_icon_find(IBar_Bar *ibb, E_App *a);
static void    _ibar_icon_reorder_after(IBar_Icon *ic, IBar_Icon *after);

static void    _ibar_bar_cb_gmc_change(void *data, E_Gadman_Client *gmc, E_Gadman_Change change);
static void    _ibar_bar_cb_intercept_move(void *data, Evas_Object *o, Evas_Coord x, Evas_Coord y);
static void    _ibar_bar_cb_intercept_resize(void *data, Evas_Object *o, Evas_Coord w, Evas_Coord h);
static void    _ibar_bar_cb_mouse_in(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void    _ibar_bar_cb_mouse_out(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void    _ibar_bar_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void    _ibar_bar_cb_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void    _ibar_bar_cb_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info);
static int     _ibar_bar_cb_timer(void *data);
static int     _ibar_bar_cb_animator(void *data);

static void    _ibar_bar_cb_enter(void *data, const char *type, void *event);
static void    _ibar_bar_cb_move(void *data, const char *type, void *event);
static void    _ibar_bar_cb_leave(void *data, const char *type, void *event);
static void    _ibar_bar_cb_drop(void *data, const char *type, void *event);
static void    _ibar_bar_cb_finished(E_Drag *drag, int dropped);

static void    _ibar_icon_cb_intercept_move(void *data, Evas_Object *o, Evas_Coord x, Evas_Coord y);
static void    _ibar_icon_cb_intercept_resize(void *data, Evas_Object *o, Evas_Coord w, Evas_Coord h);
static void    _ibar_icon_cb_mouse_in(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void    _ibar_icon_cb_mouse_out(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void    _ibar_icon_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void    _ibar_icon_cb_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void    _ibar_icon_cb_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info);

static void    _ibar_bar_cb_width_auto(void *data, E_Menu *m, E_Menu_Item *mi);
#if 0
static void    _ibar_icon_reorder_before(IBar_Icon *ic, IBar_Icon *before);
#endif
static void    _ibar_bar_iconsize_change(IBar_Bar *ibb);
static void    _ibar_bar_cb_iconsize_microscopic(void *data, E_Menu *m, E_Menu_Item *mi);
static void    _ibar_bar_cb_iconsize_tiny(void *data, E_Menu *m, E_Menu_Item *mi);
static void    _ibar_bar_cb_iconsize_very_small(void *data, E_Menu *m, E_Menu_Item *mi);
static void    _ibar_bar_cb_iconsize_small(void *data, E_Menu *m, E_Menu_Item *mi);
static void    _ibar_bar_cb_iconsize_medium(void *data, E_Menu *m, E_Menu_Item *mi);
static void    _ibar_bar_cb_iconsize_large(void *data, E_Menu *m, E_Menu_Item *mi);
static void    _ibar_bar_cb_iconsize_very_large(void *data, E_Menu *m, E_Menu_Item *mi);
static void    _ibar_bar_cb_iconsize_extremely_large(void *data, E_Menu *m, E_Menu_Item *mi);
static void    _ibar_bar_cb_iconsize_huge(void *data, E_Menu *m, E_Menu_Item *mi);
static void    _ibar_bar_cb_iconsize_enormous(void *data, E_Menu *m, E_Menu_Item *mi);
static void    _ibar_bar_cb_iconsize_gigantic(void *data, E_Menu *m, E_Menu_Item *mi);

static void    _ibar_bar_cb_menu_enabled(void *data, E_Menu *m, E_Menu_Item *mi);
static void    _ibar_bar_cb_menu_edit(void *data, E_Menu *m, E_Menu_Item *mi);

/* public module routines. all modules must have these */
void *
e_modapi_init(E_Module *m)
{
   IBar *ib;

   /* check module api version */
   if (m->api->version < E_MODULE_API_VERSION)
     {
	e_error_dialog_show(_("Module API Error"),
			    _("Error initializing Module: IBar\n"
			      "It requires a minimum module API version of: %i.\n"
			      "The module API advertized by Enlightenment is: %i.\n"
			      "Aborting module."),
			    E_MODULE_API_VERSION,
			    m->api->version);
	return NULL;
     }
   /* actually init ibar */
   ib = _ibar_new();
   m->config_menu = ib->config_menu;
   return ib;
}

int
e_modapi_shutdown(E_Module *m)
{
   IBar *ib;

   if (m->config_menu)
     m->config_menu = NULL;

   ib = m->data;
   if (ib)
     _ibar_free(ib);
   return 1;
}

int
e_modapi_save(E_Module *m)
{
   IBar *ib;

   ib = m->data;
   e_config_domain_save("module.ibar", conf_edd, ib->conf);
   return 1;
}

int
e_modapi_info(E_Module *m)
{
   char buf[4096];

   m->label = strdup(_("IBar"));
   snprintf(buf, sizeof(buf), "%s/module_icon.png", e_module_dir_get(m));
   m->icon_file = strdup(buf);
   return 1;
}

int
e_modapi_about(E_Module *m)
{
   e_error_dialog_show(_("Enlightenment IBar Module"),
		       _("This is the IBar Application Launcher bar module for Enlightenment.\n"
			 "It is a first example module and is being used to flesh out several\n"
			 "Interfaces in Enlightenment 0.17.0. It is under heavy development,\n"
			 "so expect it to break often and change as it improves."));
   return 1;
}

/* module private routines */
static IBar *
_ibar_new()
{
   IBar *ib;
   char buf[4096];
   Evas_List *managers, *l, *l2, *cl;

   bar_count = 0;
   ib = E_NEW(IBar, 1);
   if (!ib) return NULL;

   conf_bar_edd = E_CONFIG_DD_NEW("IBar_Config_Bar", Config_Bar);
#undef T
#undef D
#define T Config_Bar
#define D conf_bar_edd
   E_CONFIG_VAL(D, T, enabled, UCHAR);

   conf_edd = E_CONFIG_DD_NEW("IBar_Config", Config);
#undef T
#undef D
#define T Config
#define D conf_edd
   E_CONFIG_VAL(D, T, appdir, STR);
   E_CONFIG_VAL(D, T, follow_speed, DOUBLE);
   E_CONFIG_VAL(D, T, autoscroll_speed, DOUBLE);
   E_CONFIG_VAL(D, T, iconsize, INT);
   E_CONFIG_VAL(D, T, width, INT);
   E_CONFIG_LIST(D, T, bars, conf_bar_edd);

   ib->conf = e_config_domain_load("module.ibar", conf_edd);
   if (!ib->conf)
     {
	ib->conf = E_NEW(Config, 1);
	ib->conf->appdir = strdup("bar");
	ib->conf->follow_speed = 0.9;
	ib->conf->autoscroll_speed = 0.95;
	ib->conf->iconsize = 24;
	ib->conf->width = IBAR_WIDTH_AUTO;
     }
   E_CONFIG_LIMIT(ib->conf->follow_speed, 0.01, 1.0);
   E_CONFIG_LIMIT(ib->conf->autoscroll_speed, 0.01, 1.0);
   E_CONFIG_LIMIT(ib->conf->iconsize, 2, 400);
   E_CONFIG_LIMIT(ib->conf->width, -2, -1);
   
   _ibar_config_menu_new(ib);

   if (ib->conf->appdir[0] != '/')
     {
	char *homedir;

	homedir = e_user_homedir_get();
	if (homedir)
	  {
	     snprintf(buf, sizeof(buf), "%s/.e/e/applications/%s", homedir, ib->conf->appdir);
	     free(homedir);
	  }
     }
   else
     strcpy(buf, ib->conf->appdir);

   ib->apps = e_app_new(buf, 0);
   if (ib->apps) e_app_subdir_scan(ib->apps, 0);
   e_app_change_callback_add(_ibar_app_change, ib);

   managers = e_manager_list();
   cl = ib->conf->bars;
   for (l = managers; l; l = l->next)
     {
	E_Manager *man;

	man = l->data;
	for (l2 = man->containers; l2; l2 = l2->next)
	  {
	     E_Container *con;
	     IBar_Bar *ibb;
	     /* Config */
	     con = l2->data;
	     ibb = _ibar_bar_new(ib, con);
	     if (ibb)
	       {
		  E_Menu_Item *mi;

		  if (!cl)
		    {
		       ibb->conf = E_NEW(Config_Bar, 1);
		       ibb->conf->enabled = 1;
		       ib->conf->bars = evas_list_append(ib->conf->bars, ibb->conf);
		    }
		  else
		    {
		       ibb->conf = cl->data;
		       cl = cl->next;
		    }
		  /* Menu */
		  _ibar_bar_menu_new(ibb);

		  /* Add main menu to bar menu */
		  mi = e_menu_item_new(ibb->menu);
		  e_menu_item_label_set(mi, "Options");
		  e_menu_item_submenu_set(mi, ib->config_menu_options);

		  mi = e_menu_item_new(ibb->menu);
		  e_menu_item_label_set(mi, "Size");
		  e_menu_item_submenu_set(mi, ib->config_menu_size);

		  mi = e_menu_item_new(ib->config_menu);
		  e_menu_item_label_set(mi, con->name);
		  e_menu_item_submenu_set(mi, ibb->menu);

		  /* Setup */
		  if (!ibb->conf->enabled)
		    _ibar_bar_disable(ibb);

	       }
	  }
     }
   return ib;
}

static void
_ibar_free(IBar *ib)
{
   E_CONFIG_DD_FREE(conf_edd);
   E_CONFIG_DD_FREE(conf_bar_edd);

   while (ib->bars)
     _ibar_bar_free(ib->bars->data);
   if (ib->apps)
     e_object_unref(E_OBJECT(ib->apps));

   E_FREE(ib->conf->appdir);
   e_app_change_callback_del(_ibar_app_change, ib);
   e_object_del(E_OBJECT(ib->config_menu_options));
   e_object_del(E_OBJECT(ib->config_menu_size));
   e_object_del(E_OBJECT(ib->config_menu));
   evas_list_free(ib->conf->bars);
   free(ib->conf);
   free(ib);
}

static void
_ibar_app_change(void *data, E_App *a, E_App_Change ch)
{
   IBar *ib;
   Evas_List *l, *ll;

   ib = data;
   for (l = ib->bars; l; l = l->next)
     {
	IBar_Bar *ibb;

	ibb = l->data;
	switch (ch)
	  {
	   case E_APP_ADD:
	     if (e_app_is_parent(ib->apps, a))
	       {
		  IBar_Icon *ic;

		  ic = _ibar_icon_new(ibb, a);
		  if (ic)
		    {
		       for (ll = ib->apps->subapps; ll; ll = ll->next)
			 {
			    E_App *a2;

			    a2 = ll->data;
			    ic = _ibar_icon_find(ibb, a2);
			    if (ic) _ibar_icon_reorder_after(ic, NULL);
			 }
		       _ibar_bar_frame_resize(ibb);
		    }
	       }
	     break;
	   case E_APP_DEL:
	     if (e_app_is_parent(ib->apps, a))
	       {
		  IBar_Icon *ic;

		  ic = _ibar_icon_find(ibb, a);
		  if (ic) _ibar_icon_free(ic);
		  _ibar_bar_frame_resize(ibb);
	       }
	     break;
	   case E_APP_CHANGE:
	     if (e_app_is_parent(ib->apps, a))
	       {
		  IBar_Icon *ic;

		  ic = _ibar_icon_find(ibb, a);
		  if (ic) _ibar_icon_free(ic);
		  evas_image_cache_flush(ibb->evas);
		  evas_image_cache_reload(ibb->evas);
		  ic = _ibar_icon_new(ibb, a);
		  if (ic)
		    {
		       for (ll = ib->apps->subapps; ll; ll = ll->next)
			 {
			    E_App *a2;

			    a2 = ll->data;
			    ic = _ibar_icon_find(ibb, a2);
			    if (ic) _ibar_icon_reorder_after(ic, NULL);
			 }
		       _ibar_bar_frame_resize(ibb);
		    }
	       }
	     break;
	   case E_APP_ORDER:
	     if (a == ib->apps)
	       {
		  for (ll = ib->apps->subapps; ll; ll = ll->next)
		    {
		       IBar_Icon *ic;
		       E_App *a2;

		       a2 = ll->data;
		       ic = _ibar_icon_find(ibb, a2);
		       if (ic) _ibar_icon_reorder_after(ic, NULL);
		    }
	       }
	     break;
	   case E_APP_EXEC:
	     break;
	   case E_APP_READY:
	     break;
	   case E_APP_EXIT:
	     break;
	   default:
	     break;
	  }
     }
}

static IBar_Bar *
_ibar_bar_new(IBar *ib, E_Container *con)
{
   IBar_Bar *ibb;
   Evas_List *l;
   Evas_Object *o;
   E_Gadman_Policy policy;
   Evas_Coord x, y, w, h;

   ibb = E_NEW(IBar_Bar, 1);
   if (!ibb) return NULL;
   ibb->ibar = ib;
   ib->bars = evas_list_append(ib->bars, ibb);

   ibb->con = con;
   e_object_ref(E_OBJECT(con));
   ibb->evas = con->bg_evas;

   ibb->x = ibb->y = ibb->w = ibb->h = -1;

   evas_event_freeze(ibb->evas);
   o = edje_object_add(ibb->evas);
   ibb->bar_object = o;
   e_theme_edje_object_set(o, "base/theme/modules/ibar",
			   "modules/ibar/main");
   evas_object_show(o);

   o = edje_object_add(ibb->evas);
   ibb->overlay_object = o;
   evas_object_layer_set(o, 1);
   e_theme_edje_object_set(o, "base/theme/modules/ibar",
			   "modules/ibar/follower");
   evas_object_show(o);

   o = evas_object_rectangle_add(ibb->evas);
   ibb->event_object = o;
   evas_object_layer_set(o, 2);
   evas_object_repeat_events_set(o, 1);
   evas_object_color_set(o, 0, 0, 0, 0);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_IN,  _ibar_bar_cb_mouse_in,  ibb);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_OUT, _ibar_bar_cb_mouse_out, ibb);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _ibar_bar_cb_mouse_down, ibb);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_UP, _ibar_bar_cb_mouse_up, ibb);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_MOVE, _ibar_bar_cb_mouse_move, ibb);
   evas_object_show(o);

   o = e_box_add(ibb->evas);
   ibb->box_object = o;
   evas_object_intercept_move_callback_add(o, _ibar_bar_cb_intercept_move, ibb);
   evas_object_intercept_resize_callback_add(o, _ibar_bar_cb_intercept_resize, ibb);
   e_box_freeze(o);
   edje_object_part_swallow(ibb->bar_object, "items", o);
   evas_object_show(o);

   if (ibb->ibar->apps)
     {
	for (l = ibb->ibar->apps->subapps; l; l = l->next)
	  {
	     E_App *a;
	     IBar_Icon *ic;

	     a = l->data;
	     ic = _ibar_icon_new(ibb, a);
	  }
     }
   ibb->align_req = 0.5;
   ibb->align = 0.5;
   e_box_align_set(ibb->box_object, 0.5, 0.5);

   e_box_thaw(ibb->box_object);

   evas_object_resize(ibb->bar_object, 1000, 1000);
   edje_object_calc_force(ibb->bar_object);
   edje_object_part_geometry_get(ibb->bar_object, "items", &x, &y, &w, &h);
   ibb->inset.l = x;
   ibb->inset.r = 1000 - (x + w);
   ibb->inset.t = y;
   ibb->inset.b = 1000 - (y + h);

   ibb->drop_handler = e_drop_handler_add(ibb,
					  _ibar_bar_cb_enter, _ibar_bar_cb_move,
					  _ibar_bar_cb_leave, _ibar_bar_cb_drop,
					  "enlightenment/eapp",
					  ibb->x + ibb->inset.l, ibb->y + ibb->inset.t,
					  ibb->w - (ibb->inset.l + ibb->inset.r),
					  ibb->h - (ibb->inset.t + ibb->inset.b));

   ibb->gmc = e_gadman_client_new(ibb->con->gadman);
   e_gadman_client_domain_set(ibb->gmc, "module.ibar", bar_count++);
   policy = E_GADMAN_POLICY_EDGES | E_GADMAN_POLICY_HMOVE | E_GADMAN_POLICY_VMOVE;
   if (ibb->ibar->conf->width == IBAR_WIDTH_FIXED)
     policy |= E_GADMAN_POLICY_HSIZE;
   e_gadman_client_policy_set(ibb->gmc, policy);
   e_gadman_client_min_size_set(ibb->gmc, 8, 8);
   e_gadman_client_max_size_set(ibb->gmc, 3200, 3200);
   e_gadman_client_auto_size_set(ibb->gmc, -1, -1);
   e_gadman_client_align_set(ibb->gmc, 0.5, 1.0);
   e_gadman_client_resize(ibb->gmc, 400, 32 + ibb->inset.t + ibb->inset.b);
   e_gadman_client_change_func_set(ibb->gmc, _ibar_bar_cb_gmc_change, ibb);
   e_gadman_client_edge_set(ibb->gmc, E_GADMAN_EDGE_BOTTOM);
   e_gadman_client_load(ibb->gmc);

   evas_event_thaw(ibb->evas);

   /* We need to resize, if the width is auto and the number
    * of apps has changed since last startup */
   _ibar_bar_frame_resize(ibb);

   /*
   edje_object_signal_emit(ibb->bar_object, "passive", "");
   edje_object_signal_emit(ibb->overlay_object, "passive", "");
   */

   return ibb;
}

static void
_ibar_bar_free(IBar_Bar *ibb)
{
   e_object_unref(E_OBJECT(ibb->con));

   e_object_del(E_OBJECT(ibb->menu));

   while (ibb->icons)
     _ibar_icon_free(ibb->icons->data);

   if (ibb->timer) ecore_timer_del(ibb->timer);
   if (ibb->animator) ecore_animator_del(ibb->animator);
   evas_object_del(ibb->bar_object);
   evas_object_del(ibb->overlay_object);
   evas_object_del(ibb->box_object);
   evas_object_del(ibb->event_object);

   e_gadman_client_save(ibb->gmc);
   e_object_del(E_OBJECT(ibb->gmc));

   ibb->ibar->bars = evas_list_remove(ibb->ibar->bars, ibb);

   free(ibb->conf);
   free(ibb);
   bar_count--;
}

static void
_ibar_bar_menu_new(IBar_Bar *ibb)
{
   E_Menu *mn;
   E_Menu_Item *mi;

   mn = e_menu_new();
   ibb->menu = mn;

   /* Enabled */
   /*
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Enabled");
   e_menu_item_check_set(mi, 1);
   if (ibb->conf->enabled) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ibar_bar_cb_menu_enabled, ibb);
    */
   
   /* Edit */
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Edit Mode"));
   e_menu_item_callback_set(mi, _ibar_bar_cb_menu_edit, ibb);
}

static void
_ibar_bar_enable(IBar_Bar *ibb)
{
   ibb->conf->enabled = 1;
   evas_object_show(ibb->bar_object);
   evas_object_show(ibb->overlay_object);
   evas_object_show(ibb->box_object);
   evas_object_show(ibb->event_object);
   e_config_save_queue();
}

static void
_ibar_bar_disable(IBar_Bar *ibb)
{
   ibb->conf->enabled = 0;
   evas_object_hide(ibb->bar_object);
   evas_object_hide(ibb->overlay_object);
   evas_object_hide(ibb->box_object);
   evas_object_hide(ibb->event_object);
   e_config_save_queue();
}

static IBar_Icon *
_ibar_icon_new(IBar_Bar *ibb, E_App *a)
{
   IBar_Icon *ic;
   char *str;
   Evas_Object *o;
   Evas_Coord bw, bh;

   ic = E_NEW(IBar_Icon, 1);
   if (!ic) return NULL;
   ic->ibb = ibb;
   ic->app = a;
   e_object_ref(E_OBJECT(a));
   ibb->icons = evas_list_append(ibb->icons, ic);

   o = evas_object_rectangle_add(ibb->evas);
   ic->event_object = o;
   evas_object_layer_set(o, 1);
   evas_object_clip_set(o, evas_object_clip_get(ibb->box_object));
   evas_object_color_set(o, 0, 0, 0, 0);
   evas_object_repeat_events_set(o, 1);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_IN,  _ibar_icon_cb_mouse_in,  ic);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_OUT, _ibar_icon_cb_mouse_out, ic);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _ibar_icon_cb_mouse_down, ic);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_UP, _ibar_icon_cb_mouse_up, ic);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_MOVE, _ibar_icon_cb_mouse_move, ic);
   evas_object_show(o);

   o = edje_object_add(ibb->evas);
   ic->bg_object = o;
   evas_object_intercept_move_callback_add(o, _ibar_icon_cb_intercept_move, ic);
   evas_object_intercept_resize_callback_add(o, _ibar_icon_cb_intercept_resize, ic);
   e_theme_edje_object_set(o, "base/theme/modules/ibar",
			   "modules/ibar/icon");
   evas_object_show(o);

   o = edje_object_add(ibb->evas);
   ic->icon_object = o;
   edje_object_file_set(o, ic->app->path, "icon");
   edje_extern_object_min_size_set(o, ibb->ibar->conf->iconsize, ibb->ibar->conf->iconsize);
   edje_object_part_swallow(ic->bg_object, "item", o);
   edje_object_size_min_calc(ic->bg_object, &bw, &bh);
   evas_object_pass_events_set(o, 1);
   evas_object_show(o);

   o = edje_object_add(ibb->evas);
   ic->overlay_object = o;
   evas_object_intercept_move_callback_add(o, _ibar_icon_cb_intercept_move, ic);
   evas_object_intercept_resize_callback_add(o, _ibar_icon_cb_intercept_resize, ic);
   e_theme_edje_object_set(o, "base/theme/modules/ibar",
			   "modules/ibar/icon_overlay");
   evas_object_show(o);

   o = edje_object_add(ibb->evas);
   ic->extra_icons = evas_list_append(ic->extra_icons, o);
   edje_object_file_set(o, ic->app->path, "icon");
   edje_object_part_swallow(ic->overlay_object, "item", o);
   evas_object_pass_events_set(o, 1);
   evas_object_show(o);

   evas_object_raise(ic->event_object);

   e_box_pack_end(ibb->box_object, ic->bg_object);
   e_box_pack_options_set(ic->bg_object,
			  1, 1, /* fill */
			  0, 0, /* expand */
			  0.5, 0.5, /* align */
			  bw, bh, /* min */
			  bw, bh /* max */
			  );

   str = (char *)edje_object_data_get(ic->icon_object, "raise_on_hilight");
   if (str)
     {
	if (atoi(str) == 1) ic->raise_on_hilight = 1;
     }

   edje_object_signal_emit(ic->bg_object, "passive", "");
   edje_object_signal_emit(ic->overlay_object, "passive", "");
   return ic;
}

static void
_ibar_icon_free(IBar_Icon *ic)
{
   ic->ibb->icons = evas_list_remove(ic->ibb->icons, ic);
   if (ic->bg_object) evas_object_del(ic->bg_object);
   if (ic->overlay_object) evas_object_del(ic->overlay_object);
   if (ic->icon_object) evas_object_del(ic->icon_object);
   if (ic->event_object) evas_object_del(ic->event_object);
   while (ic->extra_icons)
     {
	Evas_Object *o;

	o = ic->extra_icons->data;
	ic->extra_icons = evas_list_remove_list(ic->extra_icons, ic->extra_icons);
	evas_object_del(o);
     }
   e_object_unref(E_OBJECT(ic->app));
   free(ic);
}

static IBar_Icon *
_ibar_icon_find(IBar_Bar *ibb, E_App *a)
{
   Evas_List *l;

   for (l = ibb->icons; l; l = l->next)
     {
	IBar_Icon *ic;

	ic = l->data;
	if (ic->app == a) return ic;
     }
   return NULL;
}

void
_ibar_config_menu_new(IBar *ib)
{
   E_Menu *mn;
   E_Menu_Item *mi;

   mn = e_menu_new();
   ib->config_menu = mn;

   mn = e_menu_new();
   ib->config_menu_options = mn;

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Auto fit icons"));
   e_menu_item_check_set(mi, 1);
   if (ib->conf->width == IBAR_WIDTH_AUTO) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ibar_bar_cb_width_auto, ib);

   mn = e_menu_new();
   ib->config_menu_size = mn;

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Microscopic"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (ib->conf->iconsize == 8) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ibar_bar_cb_iconsize_microscopic, ib);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Tiny"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (ib->conf->iconsize == 12) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ibar_bar_cb_iconsize_tiny, ib);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Very Small"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (ib->conf->iconsize == 16) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ibar_bar_cb_iconsize_very_small, ib);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Small"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (ib->conf->iconsize == 24) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ibar_bar_cb_iconsize_small, ib);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Medium"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (ib->conf->iconsize == 32) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ibar_bar_cb_iconsize_medium, ib);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Large"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (ib->conf->iconsize == 40) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ibar_bar_cb_iconsize_large, ib);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Very Large"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (ib->conf->iconsize == 48) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ibar_bar_cb_iconsize_very_large, ib);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Extremely Large"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (ib->conf->iconsize == 56) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ibar_bar_cb_iconsize_extremely_large, ib);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Huge"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (ib->conf->iconsize == 64) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ibar_bar_cb_iconsize_huge, ib);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Enormous"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (ib->conf->iconsize == 96) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ibar_bar_cb_iconsize_enormous, ib);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Gigantic"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (ib->conf->iconsize == 128) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ibar_bar_cb_iconsize_gigantic, ib);

   /* Submenus */
   mi = e_menu_item_new(ib->config_menu);
   e_menu_item_label_set(mi, _("Options"));
   e_menu_item_submenu_set(mi, ib->config_menu_options);

   mi = e_menu_item_new(ib->config_menu);
   e_menu_item_label_set(mi, _("Size"));
   e_menu_item_submenu_set(mi, ib->config_menu_size);
}

#if 0
static void
_ibar_icon_reorder_before(IBar_Icon *ic, IBar_Icon *before)
{
   Evas_Coord bw, bh;

   e_box_freeze(ic->ibb->box_object);
   e_box_unpack(ic->bg_object);
   ic->ibb->icons = evas_list_remove(ic->ibb->icons, ic);
   if (before)
     {
	ic->ibb->icons = evas_list_prepend_relative(ic->ibb->icons, ic, before);
	e_box_pack_before(ic->ibb->box_object, ic->bg_object, before->bg_object);
     }
   else
     {
	ic->ibb->icons = evas_list_prepend(ic->ibb->icons, ic);
	e_box_pack_start(ic->ibb->box_object, ic->bg_object);
     }
   edje_object_size_min_calc(ic->bg_object, &bw, &bh);
   e_box_pack_options_set(ic->bg_object,
			  1, 1, /* fill */
			  0, 0, /* expand */
			  0.5, 0.5, /* align */
			  bw, bh, /* min */
			  bw, bh /* max */
			  );
   e_box_thaw(ic->ibb->box_object);
}
#endif

static void
_ibar_icon_reorder_after(IBar_Icon *ic, IBar_Icon *after)
{
   Evas_Coord bw, bh;

   e_box_freeze(ic->ibb->box_object);
   e_box_unpack(ic->bg_object);
   ic->ibb->icons = evas_list_remove(ic->ibb->icons, ic);
   if (after)
     {
	ic->ibb->icons = evas_list_append_relative(ic->ibb->icons, ic, after);
	e_box_pack_after(ic->ibb->box_object, ic->bg_object, after->bg_object);
     }
   else
     {
	ic->ibb->icons = evas_list_append(ic->ibb->icons, ic);
	e_box_pack_end(ic->ibb->box_object, ic->bg_object);
     }
   edje_object_size_min_calc(ic->bg_object, &bw, &bh);
   e_box_pack_options_set(ic->bg_object,
			  1, 1, /* fill */
			  0, 0, /* expand */
			  0.5, 0.5, /* align */
			  bw, bh, /* min */
			  bw, bh /* max */
			  );
   e_box_thaw(ic->ibb->box_object);
}

static void
_ibar_bar_frame_resize(IBar_Bar *ibb)
{
   Evas_Coord w, h, bw, bh;
   /* Not finished loading config yet! */
   if ((ibb->x == -1)
       || (ibb->y == -1)
       || (ibb->w == -1)
       || (ibb->h == -1))
     return;

   evas_event_freeze(ibb->evas);
   e_box_freeze(ibb->box_object);

   e_box_min_size_get(ibb->box_object, &w, &h);
   edje_extern_object_min_size_set(ibb->box_object, w, h);
   edje_object_part_swallow(ibb->bar_object, "items", ibb->box_object);
   edje_object_size_min_calc(ibb->bar_object, &bw, &bh);
   edje_extern_object_min_size_set(ibb->box_object, 0, 0);
   edje_object_part_swallow(ibb->bar_object, "items", ibb->box_object);

   e_box_thaw(ibb->box_object);
   evas_event_thaw(ibb->evas);

   if (ibb->ibar->conf->width == IBAR_WIDTH_AUTO)
     {
	e_gadman_client_resize(ibb->gmc, bw, bh);
     }
   else
     {
	if ((e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_LEFT)
	    || (e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_RIGHT))
	  {
	     /* h is the width of the bar */
	     e_gadman_client_resize(ibb->gmc, bw, ibb->h);
	  }
	else if ((e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_TOP)
		 || (e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_BOTTOM))
	  {
	     /* w is the width of the bar */
	     e_gadman_client_resize(ibb->gmc, ibb->w, bh);
	  }
     }
}

static void
_ibar_bar_edge_change(IBar_Bar *ibb, int edge)
{
   Evas_List *l;
   Evas_Coord bw, bh, tmp;
   Evas_Object *o;
   E_Gadman_Policy policy;
   int changed;

   evas_event_freeze(ibb->evas);
   o = ibb->bar_object;
   edje_object_signal_emit(o, "set_orientation", _ibar_main_orientation[edge]);
   edje_object_message_signal_process(o);

   o = ibb->overlay_object;
   edje_object_signal_emit(o, "set_orientation", _ibar_main_orientation[edge]);
   edje_object_message_signal_process(o);

   e_box_freeze(ibb->box_object);

   for (l = ibb->icons; l; l = l->next)
     {
	IBar_Icon *ic;

	ic = l->data;
	o = ic->bg_object;
	edje_object_signal_emit(o, "set_orientation", _ibar_main_orientation[edge]);
	edje_object_message_signal_process(o);
	edje_object_size_min_calc(ic->bg_object, &bw, &bh);

	o = ic->overlay_object;
	edje_object_signal_emit(o, "set_orientation", _ibar_main_orientation[edge]);
	edje_object_message_signal_process(o);

	e_box_pack_options_set(ic->bg_object,
			       1, 1, /* fill */
			       0, 0, /* expand */
			       0.5, 0.5, /* align */
			       bw, bh, /* min */
			       bw, bh /* max */
			       );
     }

   ibb->align_req = 0.5;
   ibb->align = 0.5;
   e_box_align_set(ibb->box_object, 0.5, 0.5);

   policy = E_GADMAN_POLICY_EDGES | E_GADMAN_POLICY_HMOVE | E_GADMAN_POLICY_VMOVE;
   if ((edge == E_GADMAN_EDGE_BOTTOM) ||
       (edge == E_GADMAN_EDGE_TOP))
     {
	changed = (e_box_orientation_get(ibb->box_object) != 1);
	if (changed)
	  {
	     e_box_orientation_set(ibb->box_object, 1);
	     if (ibb->ibar->conf->width == IBAR_WIDTH_FIXED)
	       policy |= E_GADMAN_POLICY_HSIZE;
	     e_gadman_client_policy_set(ibb->gmc, policy);
	     tmp = ibb->w;
	     ibb->w = ibb->h;
	     ibb->h = tmp;
	  }
     }
   else if ((edge == E_GADMAN_EDGE_LEFT) ||
	    (edge == E_GADMAN_EDGE_RIGHT))
     {
	changed = (e_box_orientation_get(ibb->box_object) != 0);
	if (changed)
	  {
	     e_box_orientation_set(ibb->box_object, 0);
	     if (ibb->ibar->conf->width == IBAR_WIDTH_FIXED)
	       policy |= E_GADMAN_POLICY_VSIZE;
	     e_gadman_client_policy_set(ibb->gmc, policy);
	     tmp = ibb->w;
	     ibb->w = ibb->h;
	     ibb->h = tmp;
	  }
     }

   e_box_thaw(ibb->box_object);
   evas_event_thaw(ibb->evas);

   _ibar_bar_frame_resize(ibb);
}

static void
_ibar_bar_update_policy(IBar_Bar *ibb)
{
   E_Gadman_Policy policy;

   policy = E_GADMAN_POLICY_EDGES | E_GADMAN_POLICY_HMOVE | E_GADMAN_POLICY_VMOVE;
   if ((e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_BOTTOM) ||
       (e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_TOP))
     {
	if (ibb->ibar->conf->width == IBAR_WIDTH_FIXED)
	  policy |= E_GADMAN_POLICY_HSIZE;
	e_gadman_client_policy_set(ibb->gmc, policy);
     }
   else if ((e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_LEFT) ||
	    (e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_RIGHT))
     {
	if (ibb->ibar->conf->width == IBAR_WIDTH_FIXED)
	  policy |= E_GADMAN_POLICY_VSIZE;
	e_gadman_client_policy_set(ibb->gmc, policy);
     }
}

static void
_ibar_bar_motion_handle(IBar_Bar *ibb, Evas_Coord mx, Evas_Coord my)
{
   Evas_Coord x, y, w, h;
   double relx, rely;

   evas_object_geometry_get(ibb->box_object, &x, &y, &w, &h);
   if (w > 0) relx = (double)(mx - x) / (double)w;
   else relx = 0.0;
   if (h > 0) rely = (double)(my - y) / (double)h;
   else rely = 0.0;
   if ((e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_BOTTOM) ||
       (e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_TOP))
     {
	ibb->align_req = 1.0 - relx;
	ibb->follow_req = relx;
     }
   else if ((e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_LEFT) ||
	    (e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_RIGHT))
     {
	ibb->align_req = 1.0 - rely;
	ibb->follow_req = rely;
     }
}

static void
_ibar_bar_timer_handle(IBar_Bar *ibb)
{
   if (!ibb->timer)
     ibb->timer = ecore_timer_add(0.01, _ibar_bar_cb_timer, ibb);
   if (!ibb->animator)
     ibb->animator = ecore_animator_add(_ibar_bar_cb_animator, ibb);
}

static void
_ibar_bar_follower_reset(IBar_Bar *ibb)
{
   Evas_Coord ww, hh, bx, by, bw, bh, d1, d2, mw, mh;

   evas_output_viewport_get(ibb->evas, NULL, NULL, &ww, &hh);
   evas_object_geometry_get(ibb->box_object, &bx, &by, &bw, &bh);
   edje_object_size_min_get(ibb->overlay_object, &mw, &mh);
   if ((e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_BOTTOM) ||
       (e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_TOP))
     {
	d1 = bx;
	d2 = ww - (bx + bw);
	if (bw > 0)
	  {
	     if (d1 < d2)
	       ibb->follow_req = -((double)(d1 + (mw * 4)) / (double)bw);
	     else
	       ibb->follow_req = 1.0 + ((double)(d2 + (mw * 4)) / (double)bw);
	  }
     }
   else if ((e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_LEFT) ||
	    (e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_RIGHT))
     {
	d1 = by;
	d2 = hh - (by + bh);
	if (bh > 0)
	  {
	     if (d1 < d2)
	       ibb->follow_req = -((double)(d1 + (mh * 4)) / (double)bh);
	     else
	       ibb->follow_req = 1.0 + ((double)(d2 + (mh * 4)) / (double)bh);
	  }
     }
}

static void
_ibar_icon_cb_intercept_move(void *data, Evas_Object *o, Evas_Coord x, Evas_Coord y)
{
   IBar_Icon *ic;

   ic = data;
   evas_object_move(o, x, y);
   evas_object_move(ic->event_object, x, y);
   evas_object_move(ic->overlay_object, x, y);
}

static void
_ibar_icon_cb_intercept_resize(void *data, Evas_Object *o, Evas_Coord w, Evas_Coord h)
{
   IBar_Icon *ic;

   ic = data;
   evas_object_resize(o, w, h);
   evas_object_resize(ic->event_object, w, h);
   evas_object_resize(ic->overlay_object, w, h);
}

static void
_ibar_bar_cb_intercept_move(void *data, Evas_Object *o, Evas_Coord x, Evas_Coord y)
{
   IBar_Bar *ibb;

   ibb = data;
   evas_object_move(o, x, y);
   evas_object_move(ibb->event_object, x, y);
}

static void
_ibar_bar_cb_intercept_resize(void *data, Evas_Object *o, Evas_Coord w, Evas_Coord h)
{
   IBar_Bar *ibb;

   ibb = data;

   evas_object_resize(o, w, h);
   evas_object_resize(ibb->event_object, w, h);
}

static void
_ibar_icon_cb_mouse_in(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_In *ev;
   IBar_Icon *ic;

   ev = event_info;
   ic = data;
   evas_event_freeze(ic->ibb->evas);
   evas_object_raise(ic->event_object);
   if (ic->raise_on_hilight)
     evas_object_stack_below(ic->bg_object, ic->event_object);
   evas_object_stack_below(ic->overlay_object, ic->event_object);
   evas_event_thaw(ic->ibb->evas);
   edje_object_signal_emit(ic->bg_object, "active", "");
   edje_object_signal_emit(ic->overlay_object, "active", "");
   edje_object_signal_emit(ic->ibb->overlay_object, "active", "");
}

static void
_ibar_icon_cb_mouse_out(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Out *ev;
   IBar_Icon *ic;

   ev = event_info;
   ic = data;
   edje_object_signal_emit(ic->bg_object, "passive", "");
   edje_object_signal_emit(ic->overlay_object, "passive", "");
   edje_object_signal_emit(ic->ibb->overlay_object, "passive", "");
}

static void
_ibar_icon_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   IBar_Icon *ic;

   ev = event_info;
   ic = data;
   if (ev->button == 1)
     {
#if 0
	edje_object_signal_emit(ic->bg_object, "start", "");
	edje_object_signal_emit(ic->overlay_object, "start", "");
	edje_object_signal_emit(ic->ibb->overlay_object, "start", "");
	e_app_exec(ic->app);
#else
	drag_x = ev->output.x;
	drag_y = ev->output.y;
	drag_start = 1; 
	drag = 0; 
#endif
     }
}

static void
_ibar_icon_cb_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Up *ev;
   IBar_Icon *ic;

   ev = event_info;
   ic = data;
   if (ev->button == 1)
     {
#if 0
	edje_object_signal_emit(ic->bg_object, "start_end", "");
	edje_object_signal_emit(ic->overlay_object, "start_end", "");
	edje_object_signal_emit(ic->ibb->overlay_object, "start_end", "");
#else
	if (!drag)
	  e_app_exec(ic->app);
	drag = 0;
	drag_start = 0;
#endif
     }
}

static void
_ibar_icon_cb_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Move *ev;
   IBar_Icon *ic;

   ev = event_info;
   ic = data;

   if (drag_start)
     {
	double dist;

	dist = sqrt(pow((ev->cur.output.x - drag_x), 2) + pow((ev->cur.output.y - drag_y), 2));
	if (dist > 4)
	  {
	     E_Drag *d;
	     Evas_Object *o;
	     Evas_Coord x, y, w, h;

	     drag = 1;
	     drag_start = 0;

	     evas_object_geometry_get(ic->icon_object,
				      &x, &y, &w, &h);
	     d = e_drag_new(ic->ibb->con, x, y, "enlightenment/eapp",
			    ic->app, _ibar_bar_cb_finished);
	     o = edje_object_add(e_drag_evas_get(d));
	     edje_object_file_set(o, ic->app->path, "icon");
	     e_drag_object_set(d, o);

	     e_drag_resize(d, w, h);
	     e_drag_start(d, drag_x, drag_y);
	     evas_event_feed_mouse_up(ic->ibb->evas, 1, EVAS_BUTTON_NONE, NULL);
	     e_app_remove(ic->app);
	  }
     }
}

static void
_ibar_bar_cb_mouse_in(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_In *ev;
   IBar_Bar *ibb;

   ev = event_info;
   ibb = data;
   edje_object_signal_emit(ibb->overlay_object, "active", "");
   _ibar_bar_motion_handle(ibb, ev->canvas.x, ev->canvas.y);
   _ibar_bar_timer_handle(ibb);
}

static void
_ibar_bar_cb_mouse_out(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Out *ev;
   IBar_Bar *ibb;

   ev = event_info;
   ibb = data;
   edje_object_signal_emit(ibb->overlay_object, "passive", "");
   _ibar_bar_follower_reset(ibb);
   _ibar_bar_timer_handle(ibb);
}

static void
_ibar_bar_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   IBar_Bar *ibb;

   ev = event_info;
   ibb = data;
   if (ev->button == 3)
     {
	e_menu_activate_mouse(ibb->menu, e_zone_current_get(ibb->con),
			      ev->output.x, ev->output.y, 1, 1,
			      E_MENU_POP_DIRECTION_DOWN);
	e_util_container_fake_mouse_up_later(ibb->con, 3);
     }
}

static void
_ibar_bar_cb_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Up *ev;
   IBar_Bar *ibb;

   ev = event_info;
   ibb = data;
}

static void
_ibar_bar_cb_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Move *ev;
   IBar_Bar *ibb;

   ev = event_info;
   ibb = data;
   _ibar_bar_motion_handle(ibb, ev->cur.canvas.x, ev->cur.canvas.y);
   _ibar_bar_timer_handle(ibb);
}

static int
_ibar_bar_cb_timer(void *data)
{
   IBar_Bar *ibb;
   double dif, dif2;
   double v;

   ibb = data;
   v = ibb->ibar->conf->autoscroll_speed;
   ibb->align = (ibb->align_req * (1.0 - v)) + (ibb->align * v);
   v = ibb->ibar->conf->follow_speed;
   ibb->follow = (ibb->follow_req * (1.0 - v)) + (ibb->follow * v);

   dif = ibb->align - ibb->align_req;
   if (dif < 0) dif = -dif;
   if (dif < 0.001) ibb->align = ibb->align_req;

   dif2 = ibb->follow - ibb->follow_req;
   if (dif2 < 0) dif2 = -dif2;
   if (dif2 < 0.001) ibb->follow = ibb->follow_req;

   if ((dif < 0.001) && (dif2 < 0.001))
     {
	ibb->timer = NULL;
	return 0;
     }
   return 1;
}

static int
_ibar_bar_cb_animator(void *data)
{
   IBar_Bar *ibb;
   Evas_Coord x, y, w, h, mw, mh;

   ibb = data;

   if ((e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_BOTTOM) ||
       (e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_TOP))
     {
	e_box_align_set(ibb->box_object, ibb->align, 0.5);

	evas_object_geometry_get(ibb->box_object, &x, &y, &w, &h);
	edje_object_size_min_get(ibb->overlay_object, &mw, &mh);
	evas_object_resize(ibb->overlay_object, mw, h);
	evas_object_move(ibb->overlay_object, x + (w * ibb->follow) - (mw / 2), y);
     }
   else if ((e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_LEFT) ||
	    (e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_RIGHT))
     {
	e_box_align_set(ibb->box_object, 0.5, ibb->align);

	evas_object_geometry_get(ibb->box_object, &x, &y, &w, &h);
	edje_object_size_min_get(ibb->overlay_object, &mw, &mh);
	evas_object_resize(ibb->overlay_object, w, mh);
	evas_object_move(ibb->overlay_object, x, y + (h * ibb->follow) - (mh / 2));
     }
   if (ibb->timer) return 1;
   ibb->animator = NULL;
   return 0;
}

static void
_ibar_bar_cb_enter(void *data, const char *type, void *event)
{
   E_Event_Dnd_Enter *ev;
   Evas_Object *o;
   IBar_Bar *ibb;

   ev = event;
   ibb = data;

   o = evas_object_rectangle_add(ibb->evas);
   ibb->drag_object = o;
   evas_object_color_set(o, 255, 0, 0, 255);
   evas_object_resize(o, 32, 32);
}

static void
_ibar_bar_cb_move(void *data, const char *type, void *event)
{
   E_Event_Dnd_Move *ev;
   IBar_Bar *ibb;
   IBar_Icon *ic;
   Evas_Coord x, y, w, h;
   double iw;
   int pos;

   ev = event;
   ibb = data;

   x = ev->x - (ibb->x + ibb->inset.l);
   y = ev->y - (ibb->y + ibb->inset.t);
   w = ibb->w - (ibb->inset.l + ibb->inset.r);
   h = ibb->h - (ibb->inset.t + ibb->inset.b);

   if ((e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_BOTTOM) ||
       (e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_TOP))
     {
	iw = w / (double) evas_list_count(ibb->icons);
	pos = round(x / iw);
     }
   else if ((e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_LEFT) ||
	    (e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_RIGHT))
     {
	iw = h / (double) evas_list_count(ibb->icons);
	pos = round(y / iw);
     }
   else
     return;

   ic = evas_list_nth(ibb->icons, pos);

   e_box_freeze(ibb->box_object);
   evas_object_show(ibb->drag_object);
   e_box_unpack(ibb->drag_object);
   if (ic)
     {
	/* Add new eapp before this icon */
	e_box_pack_before(ibb->box_object, ibb->drag_object, ic->bg_object);
     }
   else
     {
	/* Add at the end */
	e_box_pack_end(ibb->box_object, ibb->drag_object);
     }
   edje_object_size_min_calc(ibb->drag_object, &w, &h);
   e_box_pack_options_set(ibb->drag_object,
			  1, 1, /* fill */
			  0, 0, /* expand */
			  0.5, 0.5, /* align */
			  32, 32, /* min */
			  32, 32 /* max */
			  );
   e_box_thaw(ibb->box_object);

   _ibar_bar_frame_resize(ibb);
}

static void
_ibar_bar_cb_leave(void *data, const char *type, void *event)
{
   E_Event_Dnd_Leave *ev;
   IBar_Bar *ibb;

   ev = event;
   ibb = data;

   e_box_freeze(ibb->box_object);
   e_box_unpack(ibb->drag_object);
   evas_object_del(ibb->drag_object);
   e_box_thaw(ibb->box_object);

   _ibar_bar_frame_resize(ibb);
}

static void
_ibar_bar_cb_drop(void *data, const char *type, void *event)
{
   E_Event_Dnd_Drop *ev;
   E_App *app;
   IBar_Bar *ibb;
   IBar_Icon *ic;
   Evas_Coord x, y, w, h;
   double iw;
   int pos;

   ev = event;
   ibb = data;
   app = ev->data;

   /* remove drag marker */
   e_box_freeze(ibb->box_object);
   e_box_unpack(ibb->drag_object);
   evas_object_del(ibb->drag_object);
   e_box_thaw(ibb->box_object);

   _ibar_bar_frame_resize(ibb);

   /* add dropped element */
   x = ev->x - (ibb->x + ibb->inset.l);
   y = ev->y - (ibb->y + ibb->inset.t);
   w = ibb->w - (ibb->inset.l + ibb->inset.r);
   h = ibb->h - (ibb->inset.t + ibb->inset.b);

   if ((e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_BOTTOM) ||
       (e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_TOP))
     {
	iw = w / (double) evas_list_count(ibb->icons);
	pos = round(x / iw);
     }
   else if ((e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_LEFT) ||
	    (e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_RIGHT))
     {
	iw = h / (double) evas_list_count(ibb->icons);
	pos = round(y / iw);
     }
   else
     return;

   ic = evas_list_nth(ibb->icons, pos);
   if (ic)
     {
	/* Add new eapp before this icon */
	e_app_prepend_relative(app, ic->app);
     }
   else
     {
	/* Add at the end */
	e_app_append(app, ibb->ibar->apps);
     }
}

static void
_ibar_bar_cb_finished(E_Drag *drag, int dropped)
{
   /* Unref the object so it will be deleted. */
   if (!dropped)
     e_object_unref(E_OBJECT(drag->data));
}

static void
_ibar_bar_cb_gmc_change(void *data, E_Gadman_Client *gmc, E_Gadman_Change change)
{
   IBar_Bar *ibb;

   ibb = data;
   switch (change)
     {
      case E_GADMAN_CHANGE_MOVE_RESIZE:
	 e_gadman_client_geometry_get(ibb->gmc, &ibb->x, &ibb->y, &ibb->w, &ibb->h);

	 edje_extern_object_min_size_set(ibb->box_object, 0, 0);
	 edje_object_part_swallow(ibb->bar_object, "items", ibb->box_object);

	 evas_object_move(ibb->bar_object, ibb->x, ibb->y);
	 evas_object_move(ibb->overlay_object, ibb->x, ibb->y);
	 evas_object_resize(ibb->bar_object, ibb->w, ibb->h);
	 evas_object_resize(ibb->overlay_object, ibb->w, ibb->h);

	 _ibar_bar_follower_reset(ibb);
	 _ibar_bar_timer_handle(ibb);

	 e_drop_handler_geometry_set(ibb->drop_handler,
				     ibb->x + ibb->inset.l, ibb->y + ibb->inset.t,
				     ibb->w - (ibb->inset.l + ibb->inset.r),
				     ibb->h - (ibb->inset.t + ibb->inset.b));
	 break;
      case E_GADMAN_CHANGE_EDGE:
	 _ibar_bar_edge_change(ibb, e_gadman_client_edge_get(ibb->gmc));
	 break;
      case E_GADMAN_CHANGE_RAISE:
      case E_GADMAN_CHANGE_ZONE:
	 /* FIXME
	  * Must we do something here?
	  */
	 break;
     }
}

static void
_ibar_bar_cb_width_auto(void *data, E_Menu *m, E_Menu_Item *mi)
{
   IBar          *ib;
   IBar_Bar      *ibb;
   unsigned char  enabled;
   Evas_List     *l;

   ib = data;
   enabled = e_menu_item_toggle_get(mi);
   if ((enabled) && (ib->conf->width == IBAR_WIDTH_FIXED))
     {
	ib->conf->width = IBAR_WIDTH_AUTO;
	for (l = ib->bars; l; l = l->next)
	  {
	     ibb = l->data;
	     _ibar_bar_update_policy(ibb);
	     _ibar_bar_frame_resize(ibb);
	  }
     }
   else if (!(enabled) && (ib->conf->width == IBAR_WIDTH_AUTO))
     {
	ib->conf->width = IBAR_WIDTH_FIXED;
	for (l = ib->bars; l; l = l->next)
	  {
	     ibb = l->data;
	     _ibar_bar_update_policy(ibb);
	     _ibar_bar_frame_resize(ibb);
	  }
     }
   e_config_save_queue();
}

static void
_ibar_bar_iconsize_change(IBar_Bar *ibb)
{
   Evas_List *l;

   e_box_freeze(ibb->box_object);
   for (l = ibb->icons; l; l = l->next)
     {
	IBar_Icon *ic;
	Evas_Object *o;
	Evas_Coord bw, bh;

	ic = l->data;
	o = ic->icon_object;
	edje_extern_object_min_size_set(o, ibb->ibar->conf->iconsize, ibb->ibar->conf->iconsize);

	evas_object_resize(o, ibb->ibar->conf->iconsize, ibb->ibar->conf->iconsize);

	edje_object_part_swallow(ic->bg_object, "item", o);
	edje_object_size_min_calc(ic->bg_object, &bw, &bh);

	e_box_pack_options_set(ic->bg_object,
	      1, 1, /* fill */
	      0, 0, /* expand */
	      0.5, 0.5, /* align */
	      bw, bh, /* min */
	      bw, bh /* max */
	      );
     }
   e_box_thaw(ibb->box_object);
   _ibar_bar_frame_resize(ibb);
}

static void
_ibar_bar_cb_iconsize_microscopic(void *data, E_Menu *m, E_Menu_Item *mi)
{
   IBar *ib;
   Evas_List *l;

   ib = data;
   ib->conf->iconsize = 8;
   for (l = ib->bars; l; l = l->next)
     {
	IBar_Bar *ibb;

	ibb = l->data;
	_ibar_bar_iconsize_change(ibb);
     }
   e_config_save_queue();
}

static void
_ibar_bar_cb_iconsize_tiny(void *data, E_Menu *m, E_Menu_Item *mi)
{
   IBar *ib;
   Evas_List *l;

   ib = data;
   ib->conf->iconsize = 12;
   for (l = ib->bars; l; l = l->next)
     {
	IBar_Bar *ibb;

	ibb = l->data;
	_ibar_bar_iconsize_change(ibb);
     }
   e_config_save_queue();
}

static void
_ibar_bar_cb_iconsize_very_small(void *data, E_Menu *m, E_Menu_Item *mi)
{
   IBar *ib;
   Evas_List *l;

   ib = data;
   ib->conf->iconsize = 16;
   for (l = ib->bars; l; l = l->next)
     {
	IBar_Bar *ibb;

	ibb = l->data;
	_ibar_bar_iconsize_change(ibb);
     }
   e_config_save_queue();
}

static void
_ibar_bar_cb_iconsize_small(void *data, E_Menu *m, E_Menu_Item *mi)
{
   IBar *ib;
   Evas_List *l;

   ib = data;
   ib->conf->iconsize = 24;
   for (l = ib->bars; l; l = l->next)
     {
	IBar_Bar *ibb;

	ibb = l->data;
	_ibar_bar_iconsize_change(ibb);
     }
   e_config_save_queue();
}

static void
_ibar_bar_cb_iconsize_medium(void *data, E_Menu *m, E_Menu_Item *mi)
{
   IBar *ib;
   Evas_List *l;

   ib = data;
   ib->conf->iconsize = 32;
   for (l = ib->bars; l; l = l->next)
     {
	IBar_Bar *ibb;

	ibb = l->data;
	_ibar_bar_iconsize_change(ibb);
     }
   e_config_save_queue();
}

static void
_ibar_bar_cb_iconsize_large(void *data, E_Menu *m, E_Menu_Item *mi)
{
   IBar *ib;
   Evas_List *l;

   ib = data;
   ib->conf->iconsize = 40;
   for (l = ib->bars; l; l = l->next)
     {
	IBar_Bar *ibb;

	ibb = l->data;
	_ibar_bar_iconsize_change(ibb);
     }
   e_config_save_queue();
}

static void
_ibar_bar_cb_iconsize_very_large(void *data, E_Menu *m, E_Menu_Item *mi)
{
   IBar *ib;
   Evas_List *l;

   ib = data;
   ib->conf->iconsize = 48;
   for (l = ib->bars; l; l = l->next)
     {
	IBar_Bar *ibb;

	ibb = l->data;
	_ibar_bar_iconsize_change(ibb);
     }
   e_config_save_queue();
}

static void
_ibar_bar_cb_iconsize_extremely_large(void *data, E_Menu *m, E_Menu_Item *mi)
{
   IBar *ib;
   Evas_List *l;

   ib = data;
   ib->conf->iconsize = 56;
   for (l = ib->bars; l; l = l->next)
     {
	IBar_Bar *ibb;

	ibb = l->data;
	_ibar_bar_iconsize_change(ibb);
     }
   e_config_save_queue();
}

static void
_ibar_bar_cb_iconsize_huge(void *data, E_Menu *m, E_Menu_Item *mi)
{
   IBar *ib;
   Evas_List *l;

   ib = data;
   ib->conf->iconsize = 64;
   for (l = ib->bars; l; l = l->next)
     {
	IBar_Bar *ibb;

	ibb = l->data;
	_ibar_bar_iconsize_change(ibb);
     }
   e_config_save_queue();
}

static void
_ibar_bar_cb_iconsize_enormous(void *data, E_Menu *m, E_Menu_Item *mi)
{
   IBar *ib;
   Evas_List *l;

   ib = data;
   ib->conf->iconsize = 96;
   for (l = ib->bars; l; l = l->next)
     {
	IBar_Bar *ibb;

	ibb = l->data;
	_ibar_bar_iconsize_change(ibb);
     }
   e_config_save_queue();
}

static void
_ibar_bar_cb_iconsize_gigantic(void *data, E_Menu *m, E_Menu_Item *mi)
{
   IBar *ib;
   Evas_List *l;

   ib = data;
   ib->conf->iconsize = 128;
   for (l = ib->bars; l; l = l->next)
     {
	IBar_Bar *ibb;

	ibb = l->data;
	_ibar_bar_iconsize_change(ibb);
     }
   e_config_save_queue();
}

static void
_ibar_bar_cb_menu_enabled(void *data, E_Menu *m, E_Menu_Item *mi)
{
   IBar_Bar *ibb;
   unsigned char enabled;

   ibb = data;
   enabled = e_menu_item_toggle_get(mi);
   if ((ibb->conf->enabled) && (!enabled))
     {  
	_ibar_bar_disable(ibb);
     }
   else if ((!ibb->conf->enabled) && (enabled))
     { 
	_ibar_bar_enable(ibb);
     }
}

static void
_ibar_bar_cb_menu_edit(void *data, E_Menu *m, E_Menu_Item *mi)
{
   IBar_Bar *ibb;

   ibb = data;
   e_gadman_mode_set(ibb->gmc->gadman, E_GADMAN_MODE_EDIT);
}
