from conans import ConanFile, tools
import os

class MainProject(ConanFile):
    python_requires = "conan_template/[~=5]@robotkernel/stable"
    python_requires_extend = "conan_template.RobotkernelConanFile"

    name = "service_provider_sercos_protocol"
    url = "https://rmc-github.robotic.dlr.de/robotkernel/service_provider_sercos_protocol"
    description = "robotkernel service provider for sercos protocol devices."
    exports_sources = ["*", "!.gitignore"] + ["!%s" % x for x in tools.Git().excluded_files()]

    def requirements(self):
        self.requires(f"{self.name}_ln_msgdef/5.0.6@{self.user}/stable")
        #self.requires(f"{self.name}_ln_msgdef/{self.version}@{self.user}/{self.channel}")
        self.requires("robotkernel_service_helper/[*]@robotkernel/stable")
        self.requires("robotkernel/[~=5]@robotkernel/stable")

    def package_info(self):
        base = self.python_requires["conan_template"].module.RobotkernelConanFile
        base.package_info(self)

        # FIXME: The lines below also strangelys eem to add the old
        # Gtk2 plugin path, which won't work. Currently it needs to be
        # stripped out in the robotkernel_gui plugin manager.
        self.env_info.PYTHONPATH.append(os.path.join(self.package_folder, "bindings/python"))
        self.env_info.PYTHONPATH.append(os.path.join(self.package_folder, "bindings/python/rk_gtk3_gui_plugin"))

