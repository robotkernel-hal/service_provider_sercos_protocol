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
from __future__ import absolute_import

# the whole package should be moved to service_provider_sercos_protocol,
# to avoid duplication

from ._sercos_diag_subview import sercos_diag_subview
from ._sercos_processdata_subview import sercos_processdata_subview
from ._sercos_view import sercos_view

#from ._sercos_wrapper import sercos_id
#from ._sercos_wrapper import sercos_parameterset

## # note there is also an equally named class sercos_device in
## # service_provider_sercos_protocol/bindings/python/rk_gtk3_gui_plugin/service_provider_sercos_protocol/sercos_device.py
## from ._sercos_wrapper import sercos_device


def init_plugin(parent):
    parent.add_module_gui('module_sercos', sercos_view, 'libmodule_sercos.so')
    parent.add_module_gui('module_sercos_service_test', sercos_view, 'libmodule_sercos_service_test.so')
