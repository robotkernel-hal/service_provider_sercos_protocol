#!/usr/bin/env python
# -*- encoding: utf-8 -*-
from __future__ import absolute_import

from future import standard_library
standard_library.install_aliases()
import logging
import os
import sys
import traceback

import gi
import yaml
gi.require_version('Gtk', '3.0')
gi.require_version('GLib', '2.0')
from gi.repository import GObject, Gtk, Gdk

import helpers
from helpers.gui_utils import get_str
import links_and_nodes as ln
from service_provider_sercos_protocol import backup_all_dialog

from service_provider_sercos_protocol.sercos_id_view import show_file_dialog

logger = logging.getLogger()



# this is already contained in service_provider_sercos_protocol,
# and does not need to be duplicated.

#class backup_all_dialog(helpers.builder_base):
#    def __init__(self):
#        fn = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'sercos_dialog_backup_all.ui')
#        helpers.builder_base.__init__(self, fn, 'dialog_backup_all')
#
#        self.dialog_backup_all.set_position(Gtk.WindowPosition.CENTER_ALWAYS)
#        self.dialog_backup_all.show_all()
#


class sercos_param_subview(helpers.builder_base):
    def __init__(self, parent_window, name, app, device_store):
        fn = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'sercos_param_subview.ui')
        helpers.builder_base.__init__(self, fn, 'main')

        self.parent = parent_window
        self.name = name
        self.app = app
        self.clnt = self.app.clnt
        self.active_color = helpers.gui_utils.get_active_color_str(self.app.window)
        self.device_store = device_store
        self.init_gui()

    def init_gui(self):
        #device view at param page
        self.create_device_treeview()
        self.devices.add(self.device_view_params)
        self.device_view_params.connect("cursor-changed", self.on_device_view_params_cursor_changed)

        #treeview for params
        self.param_store, self.param_view = self.create_parameter_treeview()
        self.values.add(self.param_view)
        self.param_view.connect("key-press-event", self.on_keypress_param_view)

        #force the view initially to be loaded with dev 0
        self.device_view_params.set_cursor((0, ))
        self.update_param_view()

        self.main.show_all()

    #CREATORS
    def create_device_treeview(self):
        self.device_view_params = view = Gtk.TreeView(self.device_store) #set_model(store)
        view.set_border_width(4)
        view.set_headers_visible(True)

        # populate store
        def view_str(column, cell, store, iter, i):
            if iter is None:
                print("view_str: iter = {}, i = {!r}".format(iter, i))
                return
            if i is None:
                print("view_str: iter = {}, i = {!r}".format(iter, i))
                return
            
            if not store.iter_is_valid(iter):
                print("view_str(): iterator invalid, returning!")
                return
            
            v = store.get_value(iter,i)
            cell.set_property('text', get_str(v))
            column.set_sizing(Gtk.TreeViewColumnSizing.AUTOSIZE)

        # populate store
        def view_name(column, cell, store, iter, i):
            if iter is None:
                return
            if not store.iter_is_valid(iter):
                print("view_name(): iterator invalid, returning!")
                return
            row = store[iter]
            #if len(row) < 3:
            #    print("view_name(): row empty, returning!")
            #    return
            dev = row[2]
            name = 'N/A'
            if 30 in dev.sercos_dictionary:
                dev.update_idn(30)
                idn = dev.sercos_dictionary[30]
                name = idn.value
            cell.set_property('text', get_str(name))
            column.set_sizing(Gtk.TreeViewColumnSizing.AUTOSIZE)

        cr = Gtk.CellRendererText()
        view.insert_column_with_data_func(-1, "Device" , cr,  view_str, 0)
        view.insert_column_with_data_func(-1, "Name" , cr,  view_name, 1)

    def create_parameter_treeview(self):
        store = Gtk.TreeStore(str, str, GObject.TYPE_PYOBJECT) # sercos id type
        self.params_model_view = view = Gtk.TreeView(store) #set_model(store)
        view.set_border_width(4)
        view.set_headers_visible(True)

        def insert_str(column, cell, store, iter, i):
            v = store.get_value(iter,i)
            cell.set_property('text', v)
            column.set_sizing(Gtk.TreeViewColumnSizing.AUTOSIZE)
            return

        cr = Gtk.CellRendererText()
        view.insert_column_with_data_func(-1, "" , cr,  insert_str, 0)
        view.insert_column_with_data_func(-1, "Name" , cr,  insert_str, 1)
        self.paramvalue_renderer = Gtk.CellRendererText()
        self.paramvalue_renderer.connect("edited", self.edit_sercos_param_value)
        view.insert_column_with_data_func(-1, "Value" , self.paramvalue_renderer,  self.insert_sercos_parameter)

        store.set_sort_column_id(0, 0)
        for i, col in enumerate(view.get_columns()):
            col.set_sizing(Gtk.TreeViewColumnSizing.AUTOSIZE)
            col.set_resizable(True)
            col.set_min_width(75 * (i*2 + 1))
        return store, view

    #HELPER
    def insert_sercos_parameter(self, column, cell, model, iter):
        #datafunc for param
        if not helpers.treestore_helpers.is_row_visible(model, iter, self.param_view):
            return True

        set_number = model.get_path(iter)[0]
        if set_number is None:
            return
        sercos_device = self.get_selected_device()
        if sercos_device is None:
            return True
        sercos_parameterset = sercos_device.sercos_parametersets[set_number]

        row = model[iter]
        if row[0].startswith("Set"):
            cell.set_property("text", "")          # all(sercos_parameterset.valid_set)) ## ?!?
            return True

        n = int(row[0].split(" ")[-1])
        sercos_parameterset.get_parameters()
        cell.set_property("text", get_str(sercos_parameterset.parameters[n][1]))
        if not sercos_parameterset.valid_set[n]:
            cell.set_property("foreground", "grey")
        else:
            cell.set_property("foreground", self.active_color)
        return True

    def get_selected_device(self):
        model, iter = self.device_view_params.get_selection().get_selected()
        if iter is None:
            return None
        return model[iter][2] #device_id, device_name, pyobject

    def backup_parametersets(self, devices):
        # for some reason this dialog does not shows up
        dlg = backup_all_dialog()
        
        blocking = False
        Gtk.main_iteration_do(blocking)

        for dev_cnt, dev in enumerate(devices, start=0):
            dlg.progressbar_devices.set_fraction(float(dev_cnt + 0.5)/len(devices))

            display_func = dlg.progressbar_parametersets.set_fraction
            dev.retrieve_sercos_parametersets(progress_display_func=display_func)

        dlg.dialog_backup_all.hide()

        dev_data = {}
        for dev in devices:
            dev_data[dev.devname] = dev.get_parametersets_as_yaml()
            
        file_save_dialog = Gtk.FileChooserDialog("Select File",
                                                 None,
                                                 Gtk.FileChooserAction.SAVE,
                                                 (Gtk.STOCK_CANCEL,
                                                  Gtk.ResponseType.CANCEL,
                                                  Gtk.STOCK_SAVE,
                                                  Gtk.ResponseType.OK))
        
        fn = show_file_dialog(self.parent.file_save_dialog)        
        with open(fn, "w") as fd:
            yaml.dump(dev_data, fd, default_flow_style=False)

    def update_param_view(self):
        #force update all paraemters
        sercos_device = self.get_selected_device()
        if sercos_device is None:
            return True
        for name, param_set in list(sercos_device.sercos_parametersets.items()):
            param_set.valid_set = [False] * 10
        self.param_view.queue_draw()
        self.params_model_view.queue_draw()
        self.device_view_params.queue_draw()
        return True

    #CALLBACKS
    def on_togglebutton_edit_toggled(self, widget):
        #enable editable ids
        self.paramvalue_renderer.set_property("editable", widget.get_active())
        return True

    def on_device_view_params_cursor_changed(self, widget):
        dev = self.get_selected_device()
        if not dev:
            return True

        self.param_store.clear()

        for set_number in dev.sercos_parametersets:
            pset = dev.sercos_parametersets[set_number]
            iter = self.param_store.insert(None, -1, ["Set %i" % pset.number, "", ""])
            for j, (name, value) in enumerate(pset.parameters):
                 iter2 = self.param_store.insert(iter, -1, ["Parameter %i" %j, name, None])
        self.param_view.expand_all()
        return True

    def on_keypress_param_view(self, widget, event):
        #catch F5 for update treeview element
        key = Gdk.keyval_name(event.keyval)
        if key in ["F5"]:
            self.update_param_view()
        else:
            return False
        return True

    def edit_sercos_param_value(self, cr, path, newvalue):
        #write to sercos after editing (confirm with ENTER)
        path = path.split(":")
        set_number = int(path[0])
        parameter_number = int(path[1])
        sercos_device = self.get_selected_device()
        parameter_set = sercos_device.sercos_parametersets[set_number]
        parameter_set.set_parameter(parameter_number, newvalue)
        parameter_set.get_parameters(force_update=True)
        return True

    def on_button_refresh_clicked(self, btn):
        self.update_param_view()

    def on_button_backup_all_clicked(self, btn):
        devices = []
        for nr, name, dev in self.device_store:
            devices.append(dev)

        self.backup_parametersets(devices)

    def on_button_backup_clicked(self, btn):
        dev = self.get_selected_device()
        devices = [ dev, ]
        self.backup_parametersets(devices)
        self.param_view.queue_draw()

    def on_button_load_clicked(self, btn):
        fn = show_file_dialog(self.parent.file_open_dialog)
        data = yaml.load(file(fn))
        for slave, slave_data in list(data.items()):
            logger.info(repr(slave))
            found = False
            slave_dev = None
            for nr, name, dev in self.device_store:
                if dev.devname == slave:
                    slave_dev = dev
                    break

            if not slave_dev:
                logger.info("corresponding slave {} not found!".format(slave))
                continue

            for pset in slave_data:
                setnr = pset['set']
                parameterset = slave_dev.sercos_parametersets[setnr]

                for nr, p in enumerate(pset['entries']):
                    for key, value in list(p.items()):
                        parameterset.set_parameter(nr, value, enable_set=False)

                slave_dev.set_command(264)
