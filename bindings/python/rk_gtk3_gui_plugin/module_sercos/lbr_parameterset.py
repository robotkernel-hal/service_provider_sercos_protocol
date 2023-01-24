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

from builtins import range
from builtins import object
import math
import sys
import threading
from os import environ

class lbr_parameterset(object):
    def __init__(self, device, set_number, set_name):
        self.sercos_device = device
        self.number = set_number
        self.name = set_name
        self.parameters = []

        self.valid_set = [False] * 10
        self.fd_get_data = None

        self.m_base = 33034
        self.e_base = 33053

    def yaml(self):
        data = {}
        data['set'] = self.number
        data['name'] = self.name
        data['entries'] = [{ p[0]:p[1], } for p in self.parameters]

        return data

    def update_callback(self, force_update):
        self.fd_get_data = None
        self.get_parameters(force_update)

    def select_parameterset(self):
        # blocking write on parameter selection id 217
        self.sercos_device.write_idn(217, self.number)
        self.sercos_device.set_command(216)

        # blocking read on selected parameterset id 254
        selected_parameterset = int(self.sercos_device.read_idn(254).value)
        if selected_parameterset != self.number:
            raise Exception("Unable to select parameterset #%s of device %s! #%s is still active" % (
                self.number, self.sercos_device.devname, selected_parameterset))

        self.sercos_device.actual_parameter_set = self.number

        for i in range(10):
            self.sercos_device.sercos_dictionary[self.m_base + i].valid = False
            self.sercos_device.sercos_dictionary[self.e_base + i].valid = False

        self.valid_set = [False] * 10

    def get_parameters(self, force_update=False):
        """
        Get sercos parameters.

        This function can be called from the GUI thread.
        """
        debug = environ.get("DEBUG_SERCOS", "")
        if bool(debug) and (debug != "0"):
            print("lbr_parameterset.get_parameters():"
                  " getting parameters for device = {}, number = {}, name {}".format(
                      self.sercos_device.devname,
                      self.number,
                      self.name))
        # everything already loaded
        if all(self.valid_set) and not force_update:
            return

        # another parameterset is currently updating
        if not self.sercos_device.parameter_lock.acquire(0):
            if not self.fd_get_data:
                self.fd_get_data = threading.Timer(0.01, self.update_callback, args=(force_update, ))
                self.fd_get_data.start()

            return

        def update(self):
            # update this parameterset
            if self.sercos_device.actual_parameter_set != self.number:
                self.select_parameterset()

            # iterate over all parameters in the set and check if already valid, if not get it from pyobject
            for i in [ x for x in range(10) if not self.valid_set[x] ]:
                mantisse = int(self.sercos_device.read_idn(self.m_base + i,
                                                           from_gui_context=False).value)
                exponent = int(self.sercos_device.read_idn(self.e_base + i,
                                                           from_gui_context=False).value)

                # if finally valid data for mantisse and exponent, print it in the tree view
                self.valid_set[i] = True
                self.parameters[i][1] = int(mantisse) * pow(10, int(exponent))

                # warning: sercos_view.update() must _only_
                # be called in the GUI thread!
                # (also, updating every single time is probably to heavy,
                # and trigger_update() takes care of that, too).
                self.sercos_device.parent.trigger_update()

            self.sercos_device.parameter_lock.release()

        threading.Thread(target=update, args=(self, )).start()

    def set_parameter(self, parameter_number, newvalue, select_set=True, enable_set=True):
        # sets a single parameter, but updates the complete set afterwards
        if select_set:
            self.sercos_device.write_idn(217, self.number)
            self.sercos_device.set_command(216)

        mantisse, exponent = self.frexp10(newvalue)
        self.sercos_device.write_idn(self.m_base + parameter_number, mantisse)
        self.sercos_device.write_idn(self.e_base + parameter_number, exponent)

        if enable_set:
            self.sercos_device.set_command(264)

    def frexp10(self, value):
        #rational: from robotkernel lbrserver.cpp by robert burger
        man = float(value)
        exp = 0
        res = 1.0
        frac, integer = math.modf(man)

        while frac != 0.0 and exp > -6:
            exp -= 1
            man *= 10.0
            frac, integer = math.modf(man)

        while man > sys.maxsize:
            exp += 1
            man /= 10.0;
        return int(man), exp
