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
from __future__ import absolute_import, print_function

import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk


from builtins import map
from .sercos_object import sercos_object
import time, helpers, threading, copy

class sercos_dict(dict):
    def __init__(self, parent):
        self.parent = parent

    def __missing__(self, key):
        self[key] = sercos_object(self.parent, key)
        return self[key]

class sercos_device(helpers.svc_wrapper):
    def __init__(self, service_prefix, app, parent, modname, devname):
        helpers.svc_wrapper.__init__(self, app.clnt,
                "{}.{}.{}.sercos_protocol".format(service_prefix, modname, devname))

        self.parent = parent
        self.app = app
        self.modname = modname
        self.devname = devname

        self.sercos_dictionary = sercos_dict(self)
        self.lock = threading.Lock()
        self.updater_condition = threading.Condition()

        #threading.Thread(target=self.list_dictionary).start()

        self.ids_to_update = []
        thread = threading.Thread(target=self.update_thread)
        thread.daemon = True
        thread.start()


    def device_id(self):
        """return a constant id which is unique for a device and
        the same for different instances of the same device"""
        
        return (self.prefix, self.modname, self.devname)
    
    def set_command(self, command):
        list([self.write_idn(command, x) for x in [1, 3, 0]])

    def read_idn(self, idn, from_gui_context=False):
        # blocking read on data
        with self.lock:
            self.svc_read_id.utf8_decode_char_fields(False)
            self.svc_read_id.req.idn = idn
            self.svc_read_id.req.elements = 0x8C
            import traceback
            if from_gui_context:
                self.svc_read_id._mainloop = helpers.svc_wrapper._mainloop
                self.svc_read_id.call_via_mainloop()
                self.svc_read_id._mainloop = None
            else:
                self.svc_read_id.call()
            return copy.copy(self.svc_read_id.resp)

    def write_idn(self, idn, value, from_gui_context=False):
        with self.lock:
            self.svc_write_id.req.idn = idn
            self.svc_write_id.req.elements = 0x80
            self.svc_write_id.req.value = value
            if from_gui_context:
                self.svc_write_id._mainloop = helpers.svc_wrapper._mainloop
                self.svc_write_id.call_via_mainloop()
                self.svc_write_id._mainloop = None
            else:
                self.svc_write_id.call()

    def list_dictionary(self):
        for x in [17, 18, 19, 21, 22, 25]:
            data = self.read_idn(x, from_gui_context=True)
            if data.value:
                vals = eval(data.value)
                # wrap single int value as edge case
                if type(vals) is int:
                    vals = [ vals, ]
                for x in vals:
                    if x not in self.sercos_dictionary:
                        self.sercos_dictionary[x] = sercos_object(self, x)
                self.parent.update()

    def update_idn(self, idn, force=False):
        obj = self.sercos_dictionary[idn]

        if (force or not obj.valid) and (idn not in self.ids_to_update):
            obj.valid = False
            self.ids_to_update.append(idn)
            with self.updater_condition:
                self.updater_condition.notify()

    def update_thread(self):
        while True:
            with self.updater_condition:
                self.updater_condition.wait()

            while len(self.ids_to_update) > 0:
                idn = self.ids_to_update[0]
                data = self.read_idn(idn)
                self.sercos_dictionary[idn].set_data(data)
                self.ids_to_update.pop(0)
                self.parent.trigger_update() # todo: update only changed row, and only at needed rate

    def retrieve_dictionary(self, progress_display_func=None):
        """retrieve sercos dictionary for storing it in a backup.
        This method is called from the GUI thread.

        progress_display_func is a handle to a function which
        can display the progress of the retrival operation
        in a progress bar, so that 0 means "nothing finished"
        and 1.0 means "all finished".
        """
        for idn, obj in list(self.sercos_dictionary.items()):
            obj.valid = False

        while True:
           if len(self.sercos_dictionary) == 0:
               for list_idn in [17, 18, 19, 21, 22, 25]:
                   resp = self.read_idn(list_idn, from_gui_context=True).value

                   if not resp:
                       continue

                   new_ids = [ x for x in eval(resp) if x not in self.sercos_dictionary ]
                   for x in new_ids:
                       self.sercos_dictionary.update({ x : sercos_object(self, x) })
                         

           cnt = 0
           for idn, obj in list(self.sercos_dictionary.items()):
               if obj.valid:
                   cnt = cnt + 1
               else:
                   self.update_idn(idn)

           if progress_display_func is not None:
               progress_display_func(float(cnt) /len(self.sercos_dictionary))

           for k in range(10):
               Gtk.main_iteration()
               time.sleep(0.001)

           if cnt == len(self.sercos_dictionary):
               break

    def get_dictionary_as_yaml(self):
        #dev_data[dev.devname] = [dev.sercos_dictionary[idn].yaml()
        #                         for idn in sorted(dev.sercos_dictionary.keys())]
        
        return [x.yaml() for idn, x in sorted(self.sercos_dictionary.items())]
        
