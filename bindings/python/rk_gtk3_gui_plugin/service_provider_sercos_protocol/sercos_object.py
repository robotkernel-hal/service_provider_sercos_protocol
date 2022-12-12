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

from builtins import map
from builtins import object
import gi
gi.require_version('Gtk', '3.0')
gi.require_version('GLib', '2.0')
from gi.repository import Gtk
from gi.repository import GObject
import time
import traceback

class sercos_object(object):
    def __init__(self, device, idn):
        self.sercos_device = device
        self.idn = idn
        self.name = 'N/A'
        self.value = None
        self.objcode = 0
        self.data_type = 0
        self.fd_get_data = None
        self.valid = False

    def yaml(self):
        return dict(idn=self.idn, name=self.name, value=self.value)

    def read(self):
        # assume called from gui context
        data = self.sercos_device.read_idn(self.idn, from_gui_context=True)
        self.set_data(data)
        self.sercos_device.parent.update()

    def write(self, value):
        # assume called from gui context
        self.sercos_device.write_idn(self.idn, value, from_gui_context=True)
        self.valid = False
        self.read()

    def set_data(self, data):
        for key, value in data.__dict__.items():
            if key == "name":
                value = value.decode('cp437', 'ignore')
            elif key == "value":
                value = value.decode("utf-8")            
            setattr(self, key, value)
        self.valid = True

