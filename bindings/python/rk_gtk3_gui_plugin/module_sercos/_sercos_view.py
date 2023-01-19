#!/usr/bin/env python
# -*- encoding: utf-8 -*-
from __future__ import absolute_import, print_function

import datetime
import logging
import os
import sys
import traceback

import gi
import yaml
gi.require_version('Gtk', '3.0')
gi.require_version('GLib', '2.0')

from gi.repository import GLib
from gi.repository import GObject, Gtk

import helpers
import links_and_nodes as ln
from service_provider_sercos_protocol import (sercos_device, sercos_object,
                                              # the following class is
                                              # shadowed by the definition below
                                              # sercos_id_view
                                              )

from . import (_sercos_diag_subview,
               # sercos_flasher_subview, ## inactivated, because unused
               sercos_id_subview,
               sercos_param_subview, _sercos_processdata_subview)
from .lbr_device import lbr_device

logger = logging.getLogger()
logger.debug("loading sercos_view module")






class sercos_view(helpers.builder_base):
    def __init__(self, parent):
        fn = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'base.ui')
        helpers.builder_base.__init__(self, fn, 'main')
        logger.debug(" 'base.ui' initialized.")

        self.parent = parent
        self.app = parent.app
        self.active_color = helpers.gui_utils.get_active_color(self.app.window)
        
        self.clnt = self.app.clnt
        self.name = 'keine ahnung'

        self.device_store = Gtk.ListStore(int, str, GObject.TYPE_PYOBJECT) # name, data
        self.device_store.set_sort_column_id(0, 0)

        #fill the invisible notebook with desired pages
        self.id_view = sercos_id_subview.sercos_id_subview(self, self.name, self.app, self.device_store)
        self.processdata_view  = _sercos_processdata_subview.sercos_processdata_subview(self, self.name, self.app, self.device_store)
        self.param_view = sercos_param_subview.sercos_param_subview(self, self.name, self.app, self.device_store)
        self.diag_view = _sercos_diag_subview.sercos_diag_subview(self, self.name, self.app, self.device_store)
        #self.flasher_view = sercos_flasher_subview.sercos_flasher_subview(self, self.name, self.app)

        self.parent.module_notebook.append_page(self.diag_view.main, Gtk.Label(label="Overview"))
        self.parent.module_notebook.append_page(self.processdata_view.main, Gtk.Label(label="Process data"))
        self.parent.module_notebook.append_page(self.id_view.main, Gtk.Label(label="IDs"))
        self.parent.module_notebook.append_page(self.param_view.main, Gtk.Label(label="Parameter"))
        #self.notebook.append_page(self.flasher_view.main, Gtk.Label(label="sercos flasher"))

        self.file_save_dialog = Gtk.FileChooserDialog("Select File", None, Gtk.FileChooserAction.SAVE,
                             (Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL, Gtk.STOCK_SAVE, Gtk.ResponseType.OK))

        self.folder_select_dialog = Gtk.FileChooserDialog("Select File", None, Gtk.FileChooserAction.SELECT_FOLDER,
                             (Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL, Gtk.STOCK_SAVE, Gtk.ResponseType.OK))

        self.file_open_dialog = Gtk.FileChooserDialog("Select File", None, Gtk.FileChooserAction.OPEN,
                                     (Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL, Gtk.STOCK_OPEN, Gtk.ResponseType.OK))

        self.active_module = None
        self.hide()

    def show(self, modname, module):
        pagenum = self.parent.module_notebook.get_current_page()

        if not hasattr(module, '_lbr_devices'):
            setattr(module, '_lbr_devices', dict())
        else:
            # stop running updates
            if self.active_module:
                for key, dev in list(module._lbr_devices.items()):
                    dev.stop_update()

        ## warning: resetting the used device store is _very_
        ## problematic because it leads to crashes in GTK
        ## views which use the GtkListStore.
        ## This is possibly due to unfixed bugs in GTK3.
        
        # self.device_store.clear()



        # initially fill all devices found by ln
        for s in module.childs:
            if s not in module._lbr_devices:
                try:
                    dev = lbr_device(module.robotkernel_name, self.app, self, module.name, s)
                    module._lbr_devices[s] = dev
                except:
                    import traceback
                    print(traceback.format_exc())
                    pass

        # Add any devices which are new to the tree store.
        # This does not remove devices which have disappeared -
        # for this, robotkernel_gui currently needs to be restarted
        # until the GUI bug on GtkListStore.clear() is fixed.
        device_store_old_content = set([r[2].device_id() for r in self.device_store])
        for key, dev in list(module._lbr_devices.items()):
            dev.start_update()
            number = int(key.split('_')[-1])
            if (dev.device_id() not in device_store_old_content):
                new_row = (number, "", dev)
                self.device_store.append( new_row )


        self._idle_update_id = None
        
        self.id_view.main.show()
        self.processdata_view.create_views()
        self.processdata_view.main.show()
        self.param_view.main.show()
        self.diag_view.fill_device()
        self.diag_view.main.show()

        if pagenum:
            self.parent.module_notebook.set_current_page(pagenum)

    def hide(self):
        if self.active_module:
            for key, dev in list(module._lbr_devices.items()):
                dev.stop_update()

        self.id_view.main.hide()
        self.processdata_view.main.hide()
        self.param_view.main.hide()
        self.diag_view.main.hide()

    #CALLBACKS
    def on_togglebutton_edit_toggled(self, widget):
        #enable editable ids
        self.id_view.idvalue_renderer.set_property("editable", widget.get_active())
        self.param_view.paramvalue_renderer.set_property("editable", widget.get_active())
        self.diag_view.enable_command = widget.get_active()
        return True

    def on_button_backup_all_clicked(self, widget):
        #create backup of sercos device ids/parameterset
        dictionary = maketrans(" ", "-")
        day = datetime.date.today().strftime('%Y%m%d')
        text = helpers.gui_utils.get_current_nb_text(self.notebook)
        p = helpers.gui_utils.show_file_dialog(self.folder_select_dialog)
        if p is None:
            return True
        print("waiting for data", end=' ')
        for d in self.id_view.device_store:
            dev = d[2]
            if text == "sercos ids":
                dev.list_sercos_ids(self.id_view.id_view) #list in case they are not yet gathered from bus
                fn = ("ids_%s_%s_%s.yaml" %(dev.device_name, dev.device_number, day)).translate(dictionary)
                dev.backup_ids("%s/%s" %(p, fn))
            elif text == "sercos parameter":
                dev.list_sercos_parametersets(self.param_view.param_view) #list in case they are not yet gathered from bus
                fn = ("parameters_%s_%s_%s.yaml" %(dev.device_name, dev.device_number, day)).translate(dictionary)
                dev.backup_parametersets("%s/%s" %(p, fn))
        return True


    def on_button_backup_clicked(self, widget):
        #create backup of sercos device ids/parameterset
        translate_dict = (" ", "-")
        day = datetime.date.today().strftime('%Y%m%d')
        text = helpers.gui_utils.get_current_nb_text(self.notebook)
        if text == "sercos ids":
            dev = self.id_view.get_selected_device()
            if dev is None:
                return True
            sugestion = ("ids_%s_%s_%s.yaml" %(dev.device_name, dev.device_number, day)).replace(*translate_dict)
            f = helpers.gui_utils.show_file_dialog(self.file_save_dialog, name=sugestion)
            if f is not None:
                print("waiting for data", end=' ')
                dev.backup_ids(f)
        elif text == "sercos parameter":
            dev = self.param_view.get_selected_device()
            if dev is None:
                return True
            sugestion = ("parameters_%s_%s_%s.yaml" %(dev.device_name, dev.device_number, day)).replace(translate_dict)
            f = helpers.gui_utils.show_file_dialog(self.file_save_dialog, name=sugestion)
            if f is not None:
                print("waiting for data", end=' ')
                dev.backup_parametersets(f)
        return True


    def on_button_load_clicked(self, widget):
        #create backup of sercos device ids/parameterset
        text = helpers.gui_utils.get_current_nb_text(self.notebook)
        if text == "sercos ids":
            dev = self.id_view.get_selected_device()
            if dev is None:
                return True
            f = helpers.gui_utils.show_file_dialog(self.file_open_dialog)
            if f is not None:
                dev.load_ids(f)
        elif text == "sercos parameter":
            dev = self.param_view.get_selected_device()
            if dev is None:
                return True
            f = helpers.gui_utils.show_file_dialog(self.file_open_dialog)
            if f is not None:
                dev.load_parametersets(f)
        return True


    def on_button_refresh_clicked(self, widget):
        #update activated value view
        text = helpers.gui_utils.get_current_nb_text(self.notebook)
        if text == "sercos ids":
            self.id_view.update_id_view(widget)
        if text == "sercos parameter":
            self.param_view.update_param_view(widget)
        return True


    def change_nb_page(self, widget, *args):
        #when mode selected from combobox
        model = widget.get_model()
        active = widget.get_active()
        if active < 0:
            return True
        page = model[active][1]
        self.notebook.set_current_page(page)
        text = helpers.gui_utils.get_current_nb_text(self.notebook)

        sensitive = text not in ["overview", "process data", "sercos flasher"]
        self.button_backup_all.set_sensitive(sensitive)
        self.button_backup.set_sensitive(sensitive)
        setattr(self.button_backup, 'visible', sensitive)
        self.button_load.set_sensitive(sensitive)
        self.button_refresh.set_sensitive(sensitive)
        sensitive = text not in ["process data", "sercos flasher"]
        self.togglebutton_edit.set_sensitive(sensitive)

        if text == "process data" and not self.processdata_view.views_created:
            self.processdata_view.create_views()

        return True

    def update(self):
        # this needs to run in GTK mainloop context
        self._idle_update_id = None
        self.param_view.update_param_view()
        #self.id_view.id_view.show_indices()
        self.id_view.main.queue_draw()
        self.id_view.id_view.update()
        self.id_view.device_view_ids.queue_draw()
        #self.id_view.update_id_view()
        self.processdata_view.update_view()

    def trigger_update(self):
        # this can be called from any thread.
        # It is called from sercos_device.
        if self._idle_update_id is None:
            self._idle_update_id = GLib.idle_add(self.update)
        
    
