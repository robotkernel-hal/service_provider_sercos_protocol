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

    def set_command(self, command):
        list([self.write_idn(command, x) for x in [1, 3, 0]])

    def read_idn(self, idn):
        #print("sercos_device {} : reading idn {}".format(self.devname, idn))

        # blocking read on data
        with self.lock:
            self.svc_read_id.utf8_decode_char_fields(False)
            self.svc_read_id.req.idn = idn
            self.svc_read_id.req.elements = 0x8C
            self.svc_read_id.call()
            return copy.copy(self.svc_read_id.resp)

    def read_idn_async_timer(self, idn, user_cb):
        self.sercos_dictionary[idn].fd_get_data = None
        return self.read_idn_async(idn, user_cb)

    def read_idn_async(self, idn, user_cb):
        print("sercos_device {} : reading idn {}".format(self.devname, idn))

        if not self.lock.acquire(False):
            if not self.sercos_dictionary[idn].fd_get_data:
                self.sercos_dictionary[idn].fd_get_data = threading.Timer(0.01, self.read_idn_async_timer, args=(idn, user_cb))
                self.sercos_dictionary[idn].fd_get_data.start()
            return

        def cb_read(cb, idn):
            while not self.svc_read_id.finished():
                time.sleep(0)

            cb(self.svc_read_id.resp)
            self.lock.release()

        self.svc_read_id.req.idn = idn
        self.svc_read_id.req.elements = 0x8C
        self.svc_read_id.call_async()
        threading.Thread(target=cb_read, args=(user_cb, idn)).start()

    def write_idn(self, idn, value):
        with self.lock:
            self.svc_write_id.req.idn = idn
            self.svc_write_id.req.elements = 0x80
            self.svc_write_id.req.value = value
            self.svc_write_id.call()

    def list_dictionary(self):
        for x in [17, 18, 19, 21, 22, 25]:
            data = self.read_idn(x, from_gui_context=True)
            if data.value:
                for x in eval(data.value):
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

