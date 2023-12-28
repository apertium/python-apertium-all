from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup
from glob import glob
import shlex
import subprocess

__version__ = "0.0.1"

def pkgconfig(mode, *libs):
    proc = subprocess.run(['pkg-config', mode] + list(libs), capture_output=True)
    return shlex.split(proc.stdout.decode('utf-8'))

ext = Pybind11Extension(
    "lttoolbox",
    sorted(glob("src/*.cpp")),
    define_macros=[("VERSION_INFO", __version__)],
    cxx_std=17,
)
ext._add_ldflags(pkgconfig('--libs', 'lttoolbox'))
ext._add_cflags(pkgconfig('--cflags', 'libxml-2.0'))
ext._add_cflags(['-DHAVE_DECL_FMEMOPEN']) # TODO: HACK

setup(
    name="lttoolbox",
    version=__version__,
    author="Daniel Swanson",
    author_email="awesomeevildudes@gmail.com",
    url="https://github.com/apertium/python-apertium-all",
    description="Pybind11 wrapper for Lttoolbox",
    long_description="",
    ext_modules=[ext],
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
    python_requires=">=3.7",
)
