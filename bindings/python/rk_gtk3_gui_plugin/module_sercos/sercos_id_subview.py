#!/usr/bin/env python
# -*- encoding: utf-8 -*-
from builtins import range
import logging
import os
import sys
import traceback

import gi
import yaml
gi.require_version('Gtk', '3.0')
gi.require_version('GLib', '2.0')
from gi.repository import GObject, Gtk
from gi.repository import GLib

import helpers
from helpers.gui_utils import get_str
from service_provider_sercos_protocol import (sercos_device, sercos_object,
                                              sercos_id_view # note that this is diferent from plugins.module_sercos/_sercos_view/sercos_view
                                              )


logger = logging.getLogger()


class sercos_id_subview(helpers.builder_base):
    def __init__(self, parent_window, name, app, device_store):
        logger.info("initializing sercos_id_subview")
        fn = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'sercos_id_subview.ui')
        # here, we do not yet connect signals because on_device_view_ids_cursor_changed()
        # can be called right after that, and self.id_view needs to be constructed
        # first (which happens in self.init_view())
        helpers.builder_base.__init__(self, fn, 'main', connect_signals=False)

        self.parent = parent_window
        self.name = name
        self.app = app

        self.active_color = {
                True  : helpers.gui_utils.get_active_color(self.app.window),
                False : "grey" }

        self.device_store = device_store
        self.init_gui()

    def __getattr__(self, name):
        widget = self.builder.get_object(name)
        if widget is None:
            raise AttributeError(name)
        setattr(self, name, widget)
        return widget

    def init_gui(self):
        #device view at ids page
        
        self.create_device_treeview()

        #treeview for ids
        hbox = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
        self.id_view = sercos_id_view(self.parent.parent, hbox)
        self.values.add_with_viewport(hbox)

        #force the view initially to be loaded with dev 0
        if len(self.device_store):
            self.device_view_ids.set_cursor((0, ))
            self.update_id_view()

        logger.info("sercos_id_subview.init_gui(): connecting signals for {} from module {}".format('sercos_id_subview.ui', self.__module__))
        self.builder.connect_signals(self)
        self.main.show_all()
        #GLib.idle_add(self.update_id_view)
        return self.main

    #CREATORS
    def create_device_treeview(self):
        self.device_view_ids.set_model(self.device_store)
        self.device_view_ids.set_border_width(4)
        self.device_view_ids.set_headers_visible(True)

        # populate store
        def view_str(column, cell, store, iter, i):
            v = store.get_value(iter,i)
            cell.set_property('text', get_str(v))
            column.set_sizing(Gtk.TreeViewColumnSizing.AUTOSIZE)
            return

        # populate store
        def view_name(column, cell, store, iter, i):
            dev = store[iter][2]
            name = 'N/A'
            if 30 in dev.sercos_dictionary:
                dev.update_idn(30)
                idn = dev.sercos_dictionary[30]
                name = idn.value
            cell.set_property('text', get_str(name))
            column.set_sizing(Gtk.TreeViewColumnSizing.AUTOSIZE)
            return

        cr = Gtk.CellRendererText()
        self.device_view_ids.insert_column_with_data_func(-1, "Device" , cr,  view_str, 0)
        self.device_view_ids.insert_column_with_data_func(-1, "Name" , cr,  view_name, 1)

    #HELPER
    def insert_sercos_id(self, column, cell, model, iter):
        #datafunc for id_view, works on all 3 columns
        if not helpers.treestore_helpers.is_row_visible(model, iter, self.id_view):
            return True

        #column.set_sizing(Gtk.TreeViewColumnSizing.AUTOSIZE)
        #insert idn directly, it is known from click on device view
        row = model[iter]
        if column.get_title() == "idn":
            idn = row[0]
            if idn & 0x8000:
                cell.set_property("text", "P-0-%04d (%d)" %(idn - 0x8000, idn))
            else:
                cell.set_property("text", "S-0-%04d (%d)" %(idn, idn))
            return True

        #for name and value ask sercos_id object, it will call svc if vars are empty
        sercos_device = self.get_selected_device()
        sercos_id = sercos_device.sercos_ids[row[0]]
        idn, name, value, valid = sercos_id.read() #note that this call will reads from sercos bus if needed
        cell.set_property("text", locals()[column.get_title()])
        cell.set_property("foreground", self.active_color[valid])
        return True


    def get_selected_device(self):
        # self.device_view_ids is a GtkTreeView configured
        # in the builder file, i.e. it is an Gtk GUI object
        model, iter = self.device_view_ids.get_selection().get_selected()
        if iter is None:
            return None
        return model[iter][2] #device_id, device_name, pyobject


    #CALLBACKS

    def on_device_view_ids_cursor_changed(self, widget):
        dev = self.get_selected_device()
        if dev is None:
            # that can apparently happen when a window/view
            # is newly exposed and no valid object has
            # yet been chosen.
            return
        self.id_view.show_device(dev)

    def on_id_view_clicked(self, widget, event):
        #catch double cklick to update id view element
        sercos_device = self.get_selected_device()

        if event.button == 1 and event.type == Gdk._2BUTTON_PRESS:
            pthinfo = widget.get_path_at_pos(int(event.x), int(event.y))
            if pthinfo is None:
                return True
            clicked_id = pthinfo[0]
            idn = self.id_store[clicked_id][0]
            idn, name, value, valid = sercos_device.sercos_ids[idn].read()
            return True
        return False


    def on_keypress_id_view(self, widget, event):
        #catch F5 for update treeview element
        key = Gdk.keyval_name(event.keyval)
        if key in ["F5"]:
            self.update_id_view()
        else:
            return False
        return True

    def update_id_view(self):
        #force update visible ids
        visible_range = self.id_view.get_visible_range()
        if visible_range == None:
            return False
        begin, end = visible_range

        sercos_device = self.get_selected_device()
        if sercos_device is None:
            return True
        for row in range(begin[0], end[0] + 1):
            idn = self.id_store[row][0]
            sercos_device.sercos_ids[idn].valid = False
        self.id_view.queue_draw()
        return True

    def edit_sercos_id_value(self, cr, path, newvalue):
        #write to sercos after editing (confirm with ENTER)
        sercos_device = self.get_selected_device()
        idn = self.id_store[self.id_store.get_iter(path)][0]
        sercos_device.write_id(idn=idn, elements=0x80, value=newvalue)
        idn, name, value, valid = sercos_device.sercos_ids[idn].read()
        return True

    def progress(self, text, progress):
        self.parent.progressbar.set_fraction(progress)
        self.parent.label_action.set_text(text)
        Gtk.main_iteration()

        
