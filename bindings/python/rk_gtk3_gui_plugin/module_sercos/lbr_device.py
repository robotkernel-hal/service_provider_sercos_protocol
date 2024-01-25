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

import os
import threading
import time

import yaml

import gi
gi.require_version('GLib', '2.0')
gi.require_version('Gtk', '3.0')
from gi.repository import GLib
from gi.repository import Gtk


import helpers
from service_provider_sercos_protocol import (sercos_device, sercos_object,
                                              # sercos_view # this is unused
                                              )

from .lbr_parameterset import lbr_parameterset


def idattr_get_datalength(attr):
    return (attr & 0x70000) >> 16

def idattr_get_decimalpoint(attr):
    return (attr & 0xF000000) >> 24

def idattr_get_datatype(attr):
    return (attr & 0x700000) >> 20

class lbr_device(sercos_device):
    def __init__(self, service_prefix, app, parent, modname, devname):
        sercos_device.__init__(self, service_prefix, app, parent, modname, devname)
        self.pd_svc_wrapper = helpers.svc_wrapper(app.clnt,
                "%s.%s.%s.process_data_inspection" % (service_prefix, modname, devname))

        self.pdout = None
        self.pdin  = None
        self.pdin_mapping = False
        self.pdout_mapping = False
        self.status_word = -1
        self.sercos_parametersets = {}
        self.parameter_lock = threading.Lock()
        self.actual_parameter_set = -1
        self.commands = []
        self.stopped = True
        self.update_thread = None

        threading.Timer(0.1, self.fill_parametersets).start()

    def __del__(self):
        print('deleting device ', self.devname)
        self.stop_update()

    def fill_parametersets(self):
        fn = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'parameter.yaml')
        with open(fn, "r") as fd:
            data = yaml.full_load(fd)
        for i, parameter_set in enumerate(data):
            name = parameter_set["set"]
            self.sercos_parametersets[i] = lbr_parameterset(self, i, name)
            for j, param in enumerate(parameter_set["parameters"]):
                self.sercos_parametersets[i].parameters.append([list(param.keys())[0], 0.0])

    def update_commands(self):
        for i in eval(self.read_idn(25).value):
            self.commands.append((i, self.read_idn(i).name.decode('cp437')))

    def start_update(self):
        self.stopped = False
        self.update_thread = threading.Thread(target=self.pd_update)
        self.update_thread.daemon = True # terminate on exit of main thread
        self.update_thread.start()

    def stop_update(self):
        self.stopped = True
        if self.update_thread is not None:
            print("waiting for update thread to stop")
            self.update_thread.join()
            self.update_thread = None

    def pd_update(self, from_gui_context=False):
        # both sercos_device and the derived lbr_device
        # share the same device object, but update them from
        # different threads.
        # This means we have to protect the shared
        # buffers with a lock.
        # Also, we need to take into account
        # whether the calls run in GTK mainloop (GUI) context

        # (my (nix_jo) guess is this runs only in the
        # separate updater thread, so the mainloop case is unused,
        # but I am not 100% sure here).
    
        
        _svc = self.pd_svc_wrapper 
        while not self.stopped:
            # Note: the lock is inherited from the parent class
            
            with self.lock:
                if from_gui_context:
                    _svc.svc_out._mainloop = helpers.svc_wrapper._mainloop
                    _svc.svc_out.call_via_mainloop()
                    _svc.svc_out._mainloop = None
                else:
                    _svc.svc_out.call()
            
                if from_gui_context:
                    _svc.svc_in._mainloop = helpers.svc_wrapper._mainloop
                    _svc.svc_in.call_via_mainloop()
                    _svc.svc_in._mainloop = None
                else:
                    _svc.svc_in.call()
                self.pdin = _svc.svc_in.resp.data
                self.pdout  = _svc.svc_out.resp.data
                # call below could perhaps go into parent's
                #  .trigger_update()/update() method

            if from_gui_context:
                self.parent.processdata_view.main.queue_draw()
            else:
                GLib.idle_add(self.parent.processdata_view.main.queue_draw)
                
        time.sleep(0.5)
    
    def create_pd_mapping(self, config_list):
        # this function is used both to create a mapping for pdin and pdout
        # the config_list parameter given is either equal to self.mdt_config or self.at_config
        offset = 2
        for id in config_list:
            if not self.sercos_dictionary[id].valid:
                return False

            attr = self.sercos_dictionary[id].attr
            datalength = idattr_get_datalength(attr)
            decimalpoint = idattr_get_decimalpoint(attr)

            setattr(self.sercos_dictionary[id], "decimalpoint", decimalpoint)
            setattr(self.sercos_dictionary[id], "datalength", datalength)
            setattr(self.sercos_dictionary[id], "offset", offset)
            setattr(self.sercos_dictionary[id], "datatype", idattr_get_datatype(attr))
            offset = offset + (2*datalength)
        return True

    def retrieve_sercos_parametersets(self, progress_display_func=None):
        """retrieve parameters for backing them up.
        This function is called from the GUI thread.

        progress_display_func is a handle to a function which
        can display the progress of the retrival operation
        in a progress bar, so that 0 means "nothing finished"
        and 1.0 means "all finished".
        """

        if progress_display_func is None:
            print("retrieve_sercos_parametersets(): Warning: no progress display!!")
        while True:
            cnt = 0
            for nr, parameterset in list(self.sercos_parametersets.items()):
                if parameterset.all_valid():
                    cnt = cnt + 1
                else:
                    if progress_display_func is not None:
                        progress_display_func(float(cnt)/len(self.sercos_parametersets))
                    parameterset.get_parameters(repeat_interval=0.1)
                    
                    #blocking = False
                    #Gtk.main_iteration_do(blocking)
                    for i in range(100):
                        Gtk.main_iteration()
                        time.sleep(0.001)
                    
                    # wait a moment because the device connection
                    # is busy anyway
                    t0 = time.time()
                    while t0 + 0.8 < time.time():
                        if parameterset.wait_for_all_valid(timeout=0.001):
                            print("retrieve_sercos_parametersets(): got new params for {}!".format(cnt))
                            cnt = cnt + 1
                            break
                        else:
                            Gtk.main_iteration()
                    else:
                        print(("retrieve_sercos_parametersets(): still "
                               "waiting for {}!").format(cnt))


                if progress_display_func is not None:
                    progress_display_func(float(cnt)/len(self.sercos_parametersets))

                    
                #blocking=False
                #Gtk.main_iteration_do(blocking)
                Gtk.main_iteration()
                time.sleep(0.05)

            blocking = False
            Gtk.main_iteration_do(blocking)
            Gtk.main_iteration()

            if cnt == len(self.sercos_parametersets):
                break
            
            print("retrieve_sercos_parametersets(): {} out of {}"
                  " parametersets retrieved, retrying...".format(
                      cnt, len(self.sercos_parametersets)))

    def get_parametersets_as_yaml(self):
        return [x.yaml() for x in list(self.sercos_parametersets.values())]

