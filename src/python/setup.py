# setup.py (with automatic dependency tracking)
from setuptools import setup

setup(
    name='renderlib',
    version='1.0',
    packages=['renderlib'],
    setup_requires=['cffi>=1.0.0'],
    cffi_modules=['build.py:ffi'],
    install_requires=['cffi>=1.0.0', 'matlib>=1.0.0'],
)
