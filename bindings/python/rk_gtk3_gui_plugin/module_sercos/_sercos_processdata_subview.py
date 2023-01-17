#!/usr/bin/env python
# -*- encoding: utf-8 -*-
from __future__ import absolute_import

import os
import sys
import ast
import threading
import traceback

import gi
import yaml
gi.require_version('Gtk', '3.0')
gi.require_version('GLib', '2.0')
from gi.repository import GObject, Gtk
from gi.repository import GLib

import numpy as np

import helpers
import links_and_nodes as ln
from pyutils.binary_packet import binary_packet


class sercos_processdata_subview(helpers.builder_base):
    def __init__(self, parent_window, name, app, device_store):
        fn = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'sercos_pd_subview.ui')
        helpers.builder_base.__init__(self, fn, 'main')

        self.parent = parent_window
        self.name = name
        self.app = app
        self.clnt = self.app.clnt
        self.active_color = helpers.gui_utils.get_active_color(self.app.window)
        self.device_store = device_store
        self.is_initialized = False

    def update_view(self):
        if self.is_initialized:
            self.treeview_act.queue_draw()
            self.treeview_des.queue_draw()
        return True

        
    def create_views(self):
        self.liststore_act, self.treeview_act, self.act_columns = self.create_view(16)
        self.sw_act = self.builder.get_object("top") #scrolledwindow
        self.sw_act.add(self.treeview_act)

        for d in self.device_store:
            self.liststore_act.append([d[2]]) #get from initial view, created by id_subview device_pyobject

        self.liststore_des, self.treeview_des, self.des_columns = self.create_view(24)
        self.sw_des = self.builder.get_object("bottom") #scrolledwindow
        self.sw_des.add(self.treeview_des)

        for d in self.device_store:
            self.liststore_des.append([d[2]]) #get from initial view, created by id_subview device_pyobject


        self.is_initialized = True
        # GObject.timeout_add(1000, self.update_view) # possibly not needed, as sercos device triggers updates
        self.main.show_all()

    #CREATORS
    def create_view(self, columns_idn):
        # this must run in the GUI thread
        store = Gtk.ListStore(GObject.TYPE_PYOBJECT) # name, data
        view = Gtk.TreeView(store) #set_model(store)
        view.set_border_width(4)
        view.set_headers_visible(True)
        view.set_property('hexpand', True)

        cr = Gtk.CellRendererText()

        def _id(column, cell, store, iter):
            cell.set_property('text', store[iter][0].devname)
            cell.set_property("foreground", "black")

        view.insert_column_with_data_func(-1, "Device", cr, _id)

        def _name(column, cell, store, iter):
            dev = store[iter][0]
            dev.update_idn(30)
            obj = dev.sercos_dictionary[30]
            #obj.read(from_gui_context=True)
            if obj.value:
                cell.set_property('text', eval(obj.value))
            else:
                cell.set_property('text', '--')
            cell.set_property("foreground", "black")

        view.insert_column_with_data_func(-1, "Name", cr, _name)

        columns = dict()
        
        if columns_idn == 16:
            def insert_statusword(column, cell, store, iter):
                value = ""
                dev = store[iter][0]
                if dev.pdin is not None: # pdin not yet received from sercos (t < 1sec)
                    value = "0x%04X" %(dev.pdin[1] << 8 | dev.pdin[0])
                cell.set_property('text', value)

            callback = self.parse_pdin
            view.insert_column_with_data_func(-1, "Status", cr, insert_statusword)
            
        if columns_idn == 24:
            def insert_controlword(column, cell, store, iter):
                value = ""
                dev = store[iter][0]
                if dev.pdout is not None: # pdout not yet received from sercos (t < 1sec)
                    value = "0x%04X" %(dev.pdout[1] << 8 | dev.pdout[0])
                cell.set_property('text', value)

            callback = self.parse_pdout
            view.insert_column_with_data_func(-1, "Control", cr, insert_controlword)
            
        for nr, name, dev in self.device_store :
            # uses ast.literal_eval() here, it is safer
            obj = dev.sercos_dictionary[columns_idn]
            obj.read(from_gui_context=True)

            dev_columns = ast.literal_eval(obj.value)
            if columns_idn == 16:
                dev.at_config = dev_columns
            if columns_idn == 24:
                dev.mdt_config = dev_columns
            for c in [ x for x in dev_columns if x not in columns ]:
                obj = dev.sercos_dictionary[c]
                obj.read(from_gui_context=True)
                columns[c] = obj.name

        for idn in sorted(map(int, columns.keys())):
            name = columns[idn].split("Parameter ")[-1]
            view.insert_column_with_data_func(idn, name, cr, callback)
                

        for col in view.get_columns():
            col.set_resizable(True)
            col.set_min_width(100)
            col.set_sizing(Gtk.TreeViewColumnSizing.AUTOSIZE)
                

        return store, view, columns

    def parse_pdin(self, column, cell, store, iter):
        value = "0.0"
        cell.set_property("foreground", "grey")
        cell.set_alignment(1.0, 0.5)
        dev = store[iter][0]
        if dev.pdin is None: # pdin not yet received from sercos (t < 1sec)
            cell.set_property('text', value)
            return

        for idn, name in list(self.act_columns.items()): #get the correct index, as ordered in joints sercos data
            if column.get_title() == name:
                pd_index = dev.at_config.index(idn)
                break
        else:
            namelist = list(self.act_columns.values())
            raise Exception("column title '{!r}' not matching any valid column, present columns = {!r}".format(column.get_title(), namelist))

        obj = dev.sercos_dictionary[idn]

        if not obj.valid:  # data for idn not yet valid
            dev.update_idn(idn)
            cell.set_property('text', value)
            return

        if not dev.pdin_mapping:
            if not dev.create_pd_mapping(dev.at_config):
                cell.set_property('text', value)
                return
            dev.pdin_mapping = True

        data = dev.pdin[obj.offset:obj.offset + (2*obj.datalength)]

        if obj.datalength == 2: # 4-byte-fix
            if obj.datatype == 0 or obj.datatype == 2:
                value = np.int32(data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24)) * np.power(10.0, obj.decimalpoint * -1)
            elif obj.datatype == 1 or obj.datatype == 3 or obj.datatype == 5:
                value = np.uint32(data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24)) * np.power(10.0, obj.decimalpoint * -1)
        else: # 1 : # 2-byte-fix
            if obj.datatype == 0 or obj.datatype == 2:
                value = np.int16(data[0] | (data[1] << 8)) * np.power(10.0, obj.decimalpoint * -1)
            elif obj.datatype == 1 or obj.datatype == 3 or obj.datatype == 5:
                value = np.uint16(data[0] | (data[1] << 8)) * np.power(10.0, obj.decimalpoint * -1)

        if obj.decimalpoint:
            cell.set_property('text', '%7.3f' % value)
        else:
            if obj.datatype == 0 or obj.datatype == 2:
                cell.set_property('text', '%d' % value)
            elif obj.datatype == 1 or obj.datatype == 5:
                cell.set_property('text', '%u' % value)
            elif obj.datatype == 3:
                cell.set_property('text', '0x%X' % int(value))
        cell.set_property("foreground", "black")

    def parse_pdout(self, column, cell, store, iter):
        value = "0.0"
        cell.set_property("foreground", "grey")
        cell.set_alignment(1.0, 0.5)
        dev = store[iter][0]
        if dev.pdout is None: # pdout not yet received from sercos (t < 1sec)
            cell.set_property('text', value)
            return

        for idn, name in list(self.des_columns.items()): #get the correct index, as ordered in joints sercos data
            if column.get_title() == name.split("Parameter ")[-1]:
                pd_index = dev.mdt_config.index(idn)
                break
        else:
            raise Exception("column title '%s' not matching any valid column" %column.get_title())

        obj = dev.sercos_dictionary[idn]

        if not obj.valid:  # data for idn not yet valid
            dev.update_idn(idn)
            cell.set_property('text', value)
            return

        if not dev.pdout_mapping:
            if not dev.create_pd_mapping(dev.mdt_config):
                cell.set_property('text', value)
                return
            #print "%s pdout mapping complete" %dev.device_name
            dev.pdout_mapping = True

        data = dev.pdout[obj.offset:obj.offset + (2*obj.datalength)]

        if obj.datalength == 2: # 4-byte-fix
            if obj.datatype == 0 or obj.datatype == 2:
                value = np.int32(data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24)) * np.power(10.0, obj.decimalpoint * -1)
            elif obj.datatype == 1 or obj.datatype == 3 or obj.datatype == 5:
                value = np.uint32(data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24)) * np.power(10.0, obj.decimalpoint * -1)
        else: # 1 : # 2-byte-fix
            if obj.datatype == 0 or obj.datatype == 2:
                value = np.int16(data[0] | (data[1] << 8)) * np.power(10.0, obj.decimalpoint * -1)
            elif obj.datatype == 1 or obj.datatype == 3 or obj.datatype == 5:
                value = np.uint16(data[0] | (data[1] << 8)) * np.power(10.0, obj.decimalpoint * -1)

        if obj.decimalpoint:
            cell.set_property('text', '%7.3f' % value)
        else:
            if obj.datatype == 0 or obj.datatype == 2:
                cell.set_property('text', '%d' % value)
            elif obj.datatype == 1 or obj.datatype == 5:
                cell.set_property('text', '%u' % value)
            elif obj.datatype == 3:
                cell.set_property('text', '0x%X' % value)

        cell.set_property("foreground", "black")
