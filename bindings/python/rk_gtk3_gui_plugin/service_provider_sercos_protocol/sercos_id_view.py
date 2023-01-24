'''
(C) Robert Burger <robert.burger@dlr.de>

This file is part of Robotkernel-GUI.

Robotkernel-GUI is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Robotkernel-GUI is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Robotkernel-GUI.  If not, see <http://www.gnu.org/licenses/>.
'''
from __future__ import print_function
from __future__ import absolute_import
from __future__ import division

from builtins import map
from builtins import range
import os, yaml

import gi
gi.require_version('Gtk', '3.0')
gi.require_version('GLib', '2.0')
from gi.repository import Gtk
from gi.repository import GLib
#from gi.repository import GObject

import logging
logger = logging.getLogger()
logger.debug("loading sercos_view module")

import helpers
from helpers.gui_utils import get_str

from .sercos_device import sercos_device
from .sercos_object import sercos_object
from pyutils.binary_packet import binary_packet

def show_file_dialog(dialog, name=""):
    if len(name):
        dialog.set_current_name(name)
    response = dialog.run()
    if response != Gtk.ResponseType.OK: #cancel button
        dialog.hide()
        return None
    fn = dialog.get_filename()
    dialog.hide()
    Gtk.main_iteration_do(False)
    return fn

class backup_all_dialog(helpers.builder_base):
    def __init__(self):
        fn = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'sercos_dialog_backup_all.ui')
        helpers.builder_base.__init__(self, fn, 'dialog_backup_all')

        self.dialog_backup_all.set_position(Gtk.WindowPosition.CENTER_ALWAYS)
        self.dialog_backup_all.show_all()

"""
typedef struct sercos_service_attribute {
    uint16_t conversionfactor;
    unsigned datalength   : 3;
    unsigned function     : 1;
    unsigned datatype     : 3;
    unsigned reserved1    : 1;
    unsigned decimalpoint : 4;
    unsigned wp_cp2       : 1;
    unsigned wp_cp3       : 1;
    unsigned wp_cp4       : 1;
    unsigned reserved2    : 1;
} sercos_service_attribute_t;
"""


packet_idattr = binary_packet((
    ("conversionfactor", "H"), # 0x0000FFFF

    ("reserved1", 1),
    ("datatype", 3),
    ("function", 1),
    ("datalength", 3),

    ("reserved2", 1),
    ("wp_cp4", 1),
    ("wp_cp3", 1),
    ("wp_cp2", 1),
    ("decimalpoint", 4),
    ))

idattr_datatype = {
    0: "Number",
    1: "Unsigned Decimal",
    2: "Signed Decimal",
    3: "Unsigned Hex",
    4: "Extcharset",
    5: "Unsigned",
    6: "Float",
    7: "Reserved"
}

idattr_datalength = {
    0: "not available",
    1: "2-Byte-fix",
    2: "4-Byte-fix",
    4: "1-Byte-var",
    5: "2-Byte-var",
    6: "4-Byte-var"
}

def idattr_get_datalength(attr):
    return (attr & 0x70000) >> 16

def idattr_get_decimalpoint(attr):
    return (attr & 0xF000000) >> 24

def idattr_get_datatype(attr):
    return (attr & 0x700000) >> 20

class sercos_id_view(helpers.service_provider_view, helpers.builder_base):
    def __init__(self, parent, container):
        fn = os.path.join(os.path.dirname(__file__), 'sercos_id_view.ui')
        helpers.builder_base.__init__(self, fn, 'sercos_protocol_box')
        helpers.service_provider_view.__init__(self, parent.app, parent, self.sercos_protocol_box, 'read_id')

        container.pack_start(self.sercos_protocol_box, True, True, 0)

        self.devices = {}
        self.current_device = None

        self.active_color = helpers.gui_utils.get_active_color_str(self.app.window)
        self.create_dictionary_treeview()

    # experimental for debugging
    def on_key_value_tv_button_press_event(self, btn, ev):
        # todo: why does the gtk builder xml file want a callback for this signal?
        return False
    
    def create_dictionary_treeview(self):
        self.treestore_dictionary = store = Gtk.TreeStore(int)
        self.treestore_id_cache = set()
        view = self.treeview_dictionary
        view.set_model(store)
        view.set_border_width(4)
        view.set_headers_visible(True)

        cell_colors = { True: self.active_color, False: 'darkgrey' }

        # ------------------ index column ------------------
        def cb_idn(column, cell, store, iter):
            cell.set_property("xalign", 1.0)
            value = store[iter][0]
            if value < 32768:
                cell.set_property('text', 'S-%04d' % (value))
            else:
                cell.set_property('text', 'P-%04d' % (value-32768))
            return True

        col_cnt = view.insert_column_with_data_func(-1, "IDn" , Gtk.CellRendererText(), cb_idn)
        column  = view.get_column(col_cnt - 1)
        column.set_sizing(Gtk.TreeViewColumnSizing.GROW_ONLY)

        # ------------------ name column -------------------
        def cb_name(column, cell, store, iter):
            parent_iter = store.iter_parent(iter)
            if not helpers.treestore_helpers.is_row_visible(store, iter, self.treeview_dictionary):
                return

            idn = store[iter][0]
            dev = self.devices[self.current_device]
            dev.update_idn(idn)
            obj = dev.sercos_dictionary[idn]

            cell.set_property("foreground", cell_colors[obj.valid])
            cell.set_property('text', obj.name)
            return True

        col_cnt = view.insert_column_with_data_func(-1, "Name", Gtk.CellRendererText(), cb_name)
        column  = view.get_column(col_cnt - 1)
        column.set_sizing(Gtk.TreeViewColumnSizing.AUTOSIZE)
        column.set_expand(True)
        column.set_resizable(True)

        # ------------------ datatype column ---------------
        def cb_datatype(column, cell, store, iter):
            parent_iter = store.iter_parent(iter)
            if not helpers.treestore_helpers.is_row_visible(store, iter, self.treeview_dictionary):
                return

            idn = store[iter][0]
            dev = self.devices[self.current_device]
            dev.update_idn(idn)
            obj = dev.sercos_dictionary[idn]

            cell.set_property("foreground", cell_colors[obj.valid])
            if obj.valid:
                dt = idattr_get_datatype(obj.attr)
                cell.set_property('text', idattr_datatype[dt])
            else:
                cell.set_property('text', '--')
            return True

        col_cnt = view.insert_column_with_data_func(-1, "Datatype", Gtk.CellRendererText(), cb_datatype)
        column  = view.get_column(col_cnt - 1)
        column.set_sizing(Gtk.TreeViewColumnSizing.AUTOSIZE)
        column.set_expand(True)
        column.set_resizable(True)

        # ------------------ datalength column ---------------
        def cb_datalength(column, cell, store, iter):
            parent_iter = store.iter_parent(iter)
            if not helpers.treestore_helpers.is_row_visible(store, iter, self.treeview_dictionary):
                return

            idn = store[iter][0]
            dev = self.devices[self.current_device]
            dev.update_idn(idn)
            obj = dev.sercos_dictionary[idn]

            cell.set_property("foreground", cell_colors[obj.valid])
            if obj.valid:
                dt = idattr_get_datalength(obj.attr)
                cell.set_property('text', get_str(idattr_datalength[dt]))
            else:
                cell.set_property('text', '--')
            return True

        col_cnt = view.insert_column_with_data_func(-1, "Datalength", Gtk.CellRendererText(), cb_datalength)
        column  = view.get_column(col_cnt - 1)
        column.set_sizing(Gtk.TreeViewColumnSizing.AUTOSIZE)
        column.set_expand(True)
        column.set_resizable(True)

        # ------------------ data column -------------------
        def cb_data(column, cell, store, iter):
            parent_iter = store.iter_parent(iter)
            if not helpers.treestore_helpers.is_row_visible(store, iter, self.treeview_dictionary):
                return

            idn = store[iter][0]
            dev = self.devices[self.current_device]
            dev.update_idn(idn)
            obj = dev.sercos_dictionary[idn]

            cell.set_property("foreground", cell_colors[obj.valid])
            cell.set_property("text", get_str(obj.value))
            return True

        cell_renderer = Gtk.CellRendererText()
        cell_renderer.set_property("editable", False)
        cell_renderer.connect("edited", self.on_edit_sercos_value)
        self.idvalue_renderer = cell_renderer

        col_cnt = view.insert_column_with_data_func(-1, "Value", cell_renderer, cb_data)
        column  = view.get_column(col_cnt - 1)
        column.set_sizing(Gtk.TreeViewColumnSizing.GROW_ONLY)
        column.set_expand(True)
        column.set_resizable(True)
        store.set_sort_column_id(0, 0)

    def show_device(self, device):
        device_key = (device.modname, device.devname)

        if device_key not in self.devices:
            self.devices[device_key] = device

        self._show_internal(device_key)

    def show(self, modname, devname):
        device_key = (modname, devname)

        if device_key not in self.devices:
            self.devices[device_key] = sercos_device(
                    self.service_prefix, self.parent.app, self, modname, devname)

        self._show_internal(device_key)

    def add(self, modname, devname):
        device_key = (modname, devname)
        print('adding ', device_key)

        if device_key not in self.devices:
            self.devices[device_key] = sercos_device(
                    self.service_prefix, self.parent.app, self, modname, devname)

    def _show_internal(self, device_key):
        if not self.current_device or self.current_device != device_key:
            self.current_device = device_key
            self.treestore_dictionary.clear()
            self.treestore_id_cache = set()

        self.devices[device_key].list_dictionary()
        helpers.service_provider_view.show(self)
        self.show_indices()

    def show_indices(self):
        if not self.current_device:
            return

        dev = self.devices[self.current_device]
        new_idn = set(dev.sercos_dictionary.keys()).difference(self.treestore_id_cache)
        for idn in new_idn:
            sibling_iter = None
            for row in self.treestore_dictionary:
                if row[0] > idn:
                    sibling_iter = row.iter
                    break
            self.treestore_dictionary.insert_after(None, sibling_iter, [idn, ])
            self.treestore_id_cache.add(idn)

    def trigger_update(self):
        if self._idle_update_id is None:
            self._idle_update_id = GLib.idle_add(self.update)
        
    def update(self):
        self._idle_update_id = None
        self.show_indices()
        self.treeview_dictionary.queue_draw()
        return False
    
    def backup_ids(self, devices):
        dlg = backup_all_dialog()

        for dev_cnt, dev in enumerate(devices, start=0):            
            dlg.progressbar_devices.set_fraction(float(dev_cnt) / len(devices))

            display_func = dlg.progressbar_parametersets.set_fraction
            dev.load_dictionary(progress_display_func=display_func)


        dlg.dialog_backup_all.hide()

        dev_data = {}
        for dev in devices:
            dev_data[dev.devname] = dev.get_dictionary_as_yaml()

        file_save_dialog = Gtk.FileChooserDialog("Select File", None, Gtk.FileChooserAction.SAVE,
                (Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL, Gtk.STOCK_SAVE, Gtk.ResponseType.OK))

        fn = show_file_dialog(file_save_dialog)
        with open(fn, "w") as fp:
            yaml_str = yaml.dump(dev_data, fp, allow_unicode=True, default_flow_style=False)

    #CALLBACKS
    def on_togglebutton_edit_toggled(self, widget):
        #enable editable ids
        self.idvalue_renderer.set_property("editable", widget.get_active())
        return True

    def on_button_refresh_clicked(self, button):
        #force update visible ids
        visible_range = self.treeview_dictionary.get_visible_range()
        if visible_range == None:
            return False
        begin, end = visible_range

        dev = self.devices[self.current_device]
        dev.list_dictionary()

        for row in range(begin[0], end[0] + 1):
            idn = self.treestore_dictionary[row][0]
            dev.update_idn(idn, force=True)

        return True

    def on_edit_sercos_value(self, cr, path, newvalue):
        it = self.treestore_dictionary.get_iter(path)
        idn = self.treestore_dictionary[it][0]
        dev = self.devices[self.current_device]
        dev.sercos_dictionary[idn].write(newvalue)
        return True

    def on_button_backup_all_clicked(self, btn):
        devices = []
        for device_key, dev in list(self.devices.items()):
            devices.append(dev)

        self.backup_ids(devices)

    def on_button_backup_clicked(self, btn):
        devices = [ self.devices[self.current_device], ]
        self.backup_ids(devices)

    def on_button_load_clicked(self, btn):
        file_open_dialog = Gtk.FileChooserDialog("Select File", None, Gtk.FileChooserAction.OPEN,
                (Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL, Gtk.STOCK_OPEN, Gtk.ResponseType.OK))

        fn = show_file_dialog(file_open_dialog)
        if fn is None:
            return False
        # FIXME: use context manager here
        data = yaml.load(open(fn))
        for slave, slave_data in list(data.items()):
            slave_dev = None
            for device_key, dev in list(self.devices.items()):
                if dev.devname == slave:
                    slave_dev = dev
                    break

            if not slave_dev:
                print('corresponding slave ', slave, ' not found!')
                continue

            for obj in slave_data:
                idn   = obj['idn']
                name  = obj['name']
                value = obj['value']

                dev.write_idn(idn, value)
