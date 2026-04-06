"""Sphinx configuration for pycanha-core-python documentation."""

from datetime import date
from importlib.metadata import version as package_version


project = "pycanha-core"
author = "Javier Piqueras Carreño"
copyright = f"{date.today().year}, {author}"  # noqa: A001
release = package_version("pycanha-core")
version = ".".join(release.split(".")[:2])

extensions = [
	"sphinx.ext.autodoc",
	"sphinx.ext.autosummary",
	"sphinx.ext.napoleon",
	"sphinx.ext.intersphinx",
	"sphinx.ext.viewcode",
	"myst_parser",
	"sphinx_design",
]

templates_path = ["_templates"]
exclude_patterns = [
	"_build",
	"Thumbs.db",
	".DS_Store",
	"installation.md",
	"modules.rst",
	"pycanha_core.rst",
]

autodoc_default_options = {
	"members": True,
	"undoc-members": False,
	"show-inheritance": True,
}
autodoc_member_order = "bysource"
autodoc_typehints = "description"
autosummary_generate = True
add_module_names = False

napoleon_google_docstring = False
napoleon_numpy_docstring = True
napoleon_use_param = False
napoleon_use_rtype = False

intersphinx_mapping = {
	"python": ("https://docs.python.org/3", None),
	"numpy": ("https://numpy.org/doc/stable", None),
}

html_theme = "pydata_sphinx_theme"
html_static_path = ["_static"]
html_theme_options = {
	"github_url": "https://github.com/pycanha-project/pycanha-core-python",
	"logo": {
		"text": "pycanha-core",
	},
	"navigation_with_keys": False,
	"show_toc_level": 2,
	"navbar_align": "left",
	"navbar_end": ["theme-switcher", "navbar-icon-links"],
	"secondary_sidebar_items": ["page-toc", "edit-this-page"],
	"footer_start": ["copyright"],
	"footer_end": ["theme-version"],
	"use_edit_page_button": True,
	"icon_links": [
		{
			"name": "PyPI",
			"url": "https://pypi.org/project/pycanha-core/",
			"icon": "fa-solid fa-box",
		},
	],
}
html_context = {
	"github_user": "pycanha-project",
	"github_repo": "pycanha-core-python",
	"github_version": "main",
	"doc_path": "docs",
}
html_show_sourcelink = False
