#!/usr/bin/env python
# -*- encoding: utf-8 -*-
from __future__ import absolute_import, print_function

from builtins import object
import os
import sys
import traceback

import gi
import yaml
from gi.repository import GObject, Gtk, Gdk
#from numpy import *

import helpers
from helpers.gui_utils import get_str
import links_and_nodes as ln

#from . import _sercos_wrapper

gi.require_version('Gtk', '3.0')
gi.require_version('GLib', '2.0')



# state conversion
state_class1_mask = 0x2000
state_class1 = {
        0x0000: ( "Ok", "darkgreen" ),
        0x2000: ( "Error", "red" ) }
state_class2_mask = 0x1000
state_class2 = {
        0x0000: ( "Ok", "darkgreen" ),
        0x1000: ( "Warning", "red" ) }
state_class3_mask = 0x0800
state_class3 = {
        0x0000: ( "Ok", "darkgreen" ),
        0x0800: ( "Warning", "red" ) }
state_power_mask = 0xC000
state_power = {
        0x0000: ( "Not Ready", "red" ),
        0x4000: ( "Logic Ready / Emergency Pressed", "red" ),
        0x8000:	( "Ready (Power OFF)", "blue" ),
        0xC000: ( "Ready (Power ON)","darkgreen" ) }
state_emergency_mask = 0x00C0
state_emergency = {
        0x0000: ( "Unknown", "red" ),
        0x0040: ( "Emergency Pressed", "red" ),
        0x0080:	( "Emergency Released", "blue" ),
        0x00C0: ( "Unknown","darkgreen" ) }
state_controller_mask = 0x0700
state_controller = {
        0x0100 : ( "Torque", "darkgreen" ),
        0x0200 : ( "Speed", "blue" ),
        0x0300 : ( "None", "red" ),
        0x0400 : ( "State-Feedback", "orange" ),
        0x0000 : ( "Position", "magenta") }

state_class2_warnings = {
        0x0002 : ( "Powersection temperature warning", "red" ),
        0x0004 : ( "Motor temperature warning", "red" ),
        0x0008 : ( "DSP temperature warning", "red" ) }


class sercos_diag_subview(object):
    def __init__(self, parent_window, name, app, device_store):
        self.parent_window = parent_window
        self.name = name
        self.app = app
        self.clnt = self.app.clnt
        self.active_color = helpers.gui_utils.get_active_color(self.app.window)
        self.device_store = device_store
        self.enable_command = True
        self.init_gui()
        def update_view():
            self.treeview.queue_draw()
            return True

        GObject.timeout_add(1000, update_view)


    def init_gui(self):
        self.liststore, self.treeview = self.create_overview()
        self.main = Gtk.ScrolledWindow()
        self.main.add_with_viewport(self.treeview)
        self.main.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        self.treeview.connect("button-press-event", self.on_treeview_clicked)
        self.main.show_all()

    def fill_device(self):
        self.liststore.clear()

        for d in self.device_store:
            dev = d[2] #get from initial view, created by id_subview device_pyobject
            self.liststore.append([dev])

    #CREATORS
    def create_overview(self):
        store = Gtk.ListStore(GObject.TYPE_PYOBJECT) # name, data
        view = Gtk.TreeView(store) #set_model(store)
        view.set_border_width(4)
        view.set_headers_visible(True)

        cr = Gtk.CellRendererText()

        def _id(column, cell, store, iter):
            name = store[iter][0].devname
            cell.set_property('text', name)
            cell.set_property("foreground", "black")

        view.insert_column_with_data_func(-1, "Device", cr, _id)

        def _name(column, cell, store, iter):
            dev = store[iter][0]
            dev.update_idn(30)
            if dev.sercos_dictionary[30].value:
                cell.set_property('text', eval(dev.sercos_dictionary[30].value))
            else:
                cell.set_property('text', '--')
            cell.set_property("foreground", "black")

        view.insert_column_with_data_func(-1, "Name", cr, _name)
        view.insert_column_with_data_func(-1, "Diagnosis", cr, self.parse_status_word)
        view.insert_column_with_data_func(-1, "Controller", cr, self.parse_status_word)
        view.insert_column_with_data_func(-1, "Power", cr, self.parse_status_word)

        for i, col in enumerate(view.get_columns()):
            if i == 0:
                col.set_min_width(50)
                col.set_sort_order(Gtk.SortType.ASCENDING)
                col.set_sort_column_id(0)
                col.set_sort_indicator(True)
                continue
            col.set_expand(True)
            col.set_resizable(True)
            col.set_min_width(100)
            col.set_sizing(Gtk.TreeViewColumnSizing.AUTOSIZE)

        view.get_selection().set_mode(Gtk.SelectionMode.MULTIPLE)
        #store.set_sort_column_id(0, Gtk.SortType.ASCENDING)
        return store, view

    def parse_status_word(self, column, cell, store, iter):
        dev = store[iter][0]
        if dev.pdin is None:
            cell.set_property('text', "---")
            cell.set_property("foreground", "grey")
            return
        status_word = dev.pdin[1] << 8 | dev.pdin[0]
        if column.get_title() == "Diagnosis":
            ok, color = state_class1[status_word & state_class1_mask]
            if status_word != dev.status_word:
                dev.diagnosis = dev.read_idn(95).value
            dev.status_word = status_word
            value = dev.diagnosis
        elif column.get_title() == "Controller":
            value, color = state_controller[status_word & state_controller_mask]
        elif column.get_title() == "Power":
            value, color = state_power[status_word & state_power_mask]
        #elif column.get_title() == "Emergency":
        #    value, color = state_emergency[pdin & state_emergency_mask]
        else:
            value, color = "", "black"

        cell.set_property('text', get_str(value)) #"0x%04X" %value )
        cell.set_property("foreground", color)

    def on_treeview_clicked(self, widget, event):
        def foreach(model, path, iter, selected):
            selected.append(model[iter][0])

        selected_devs = [] #get all selected rows, for multiselect treeview
        self.treeview.get_selection().selected_foreach(foreach, selected_devs)

        if not len(selected_devs):
            return None

        if event.button == 1 and event.type == Gdk.EventType._2BUTTON_PRESS:
            dev = selected_devs[0] #device_id, device_name, pyobject
            dev.set_command(99)
            return True
        if event.button == 3:
            #handler on click, to set command for all selected devs
            def handle_click(widget, event, selected_devs, idn):
                for dev in selected_devs:
                    print("set_command %i for device %s" %(idn, dev.devname))
                    dev.set_command(idn)
                return True

            #get the finally clicked entries command list
            pthinfo = widget.get_path_at_pos(int(event.x), int(event.y))
            if pthinfo is None:
                return True

            iter = self.liststore.get_iter(pthinfo[0])
            dev = self.liststore[iter][0]

            if not len(dev.commands):
                dev.update_commands()
            #    for i in eval(dev.read_idn(25).value):
            #        dev.commands.append((i, dev.read_idn(i).name))

            #create the popupmenu on the fly, with the given command list
            popup_menu = Gtk.Menu()
            for idn, name in dev.commands:
                menu = Gtk.MenuItem("%i %s" %(idn, name))
                popup_menu.append(menu)
                menu.connect("button-press-event", handle_click, selected_devs, idn)
                menu.set_sensitive(self.enable_command)
                menu.show()
            popup_menu.popup(None, None, None, None, event.button, event.time)
            return True
        return False
