import os
from conans import ConanFile, AutoToolsBuildEnvironment

class lnrk_interface_python(ConanFile):
    name = "service_provider_sercos_protocol_rkgui"
    description = "python rkgui binding to service_provider_sercos_protocol."
    author = "Robert Burger <robert.burgert@dlr.de>"
    license = "GPLv3"
    
    url = f"https://rmc-github.robotic.dlr.de/robotkernel/service_provider_sercos_protocol"
    settings = "os"
    pure_python_folder = os.path.join("bindings","python")
    exports_sources = os.path.join(pure_python_folder, "*")
    
    def requirements(self):
        self.requires(f"service_provider_sercos_protocol_ln_msgdef/{self.version}@{self.user}/{self.channel}")

    def package(self):
        self.copy(os.path.join(self.pure_python_folder, "*"))

    def package_info(self):
        self.env_info.PYTHONPATH.append(os.path.join(self.package_folder, os.path.dirname(self.pure_python_folder)))
    
