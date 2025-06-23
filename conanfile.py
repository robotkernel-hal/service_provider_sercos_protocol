from conan import ConanFile
import os


class MainProject(ConanFile):
    python_requires = "conan_template/[~=5]@robotkernel/stable"
    python_requires_extend = "conan_template.RobotkernelConanFile"

    name = "service_provider_sercos_protocol"
    url = "https://rmc-github.robotic.dlr.de/robotkernel/service_provider_sercos_protocol"
    description = "robotkernel service provider for sercos protocol devices."
    exports_sources = ["*", "!.gitignore", "!bindings"]

    tool_requires = ["robotkernel_service_helper/[~6]@robotkernel/snapshot"]

    def requirements(self):
        self.requires(f"{self.name}_ln_msgdef/{self.version}@{self.user}/{self.channel}")
        self.requires("robotkernel/[~=6]@robotkernel/snapshot")
