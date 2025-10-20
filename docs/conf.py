# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# TODO: If pycanha_core is installed, the binary distribution will be imported,
# not the "mockup" with the stubs. To build the documentation, pycanha_core cannot be installed.
# To be fixed in the future.

# Transfor pyi stub files to python files
# Create the subdirectory pycanha_core
# Copy ../src/pycanha_core/pycanha_core.pyi to pycanha_core/pycanha_core.py
import os
import sys
import shutil
import toml


os.makedirs("pycanha_core", exist_ok=True)
# shutil.copyfile("../src/pycanha_core/__init__.pyi", "pycanha_core/__init__.py")
shutil.copyfile("../src/pycanha_core/gmm.pyi", "pycanha_core/gmm.py")
shutil.copyfile("../src/pycanha_core/tmm.pyi", "pycanha_core/tmm.py")
sys.path.insert(0, os.path.abspath("."))
print(sys.path)


# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information
# import pycanha_core

project = "pycanha-core"
copyright = "2024, Javier Piqueras"
author = "Javier Piqueras"
release = toml.load("../pyproject.toml")["project"]["version"]

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = ["sphinx.ext.autodoc", "myst_parser"]


# templates_path = ["_templates"]
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store"]

# If true, the current module name will be prepended to all description
# unit titles (such as .. function::).
add_module_names = False


# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = "alabaster"
# html_static_path = ["_static"]
