from conans import ConanFile, tools
import os

class MainProject(ConanFile):
    python_requires = "conan_template/[~=5]@robotkernel/stable"
    python_requires_extend = "conan_template.RobotkernelConanFile"

    name = "service_provider_sercos_protocol"
    description = "robotkernel service provider for sercos protocol devices."
    exports_sources = ["*", "!.gitignore"] + ["!%s" % x for x in tools.Git().excluded_files()]

    def requirements(self):
        self.requires(f"{self.name}_ln_msgdef/{self.version}@{self.user}/{self.channel}")
        self.requires("robotkernel_service_helper/[*]@robotkernel/stable")
        self.requires("robotkernel/[~=5]@robotkernel/stable")

    def package_info(self):
        base = self.python_requires["conan_template"].module.RobotkernelConanFile
        base.package_info(self)

        self.env_info.PYTHONPATH.append(os.path.join(self.package_folder, "bindings/python"))
        self.env_info.PYTHONPATH.append(os.path.join(self.package_folder, "bindings/python/rk_gui_plugin"))

