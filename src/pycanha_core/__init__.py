"""Python package bootstrap for the compiled ``pycanha_core`` extension.

This module ensures the Intel MKL runtime shipped via ``mkl``/``mkl-devel``
Python wheels is made available before the native extension is loaded.

MKL is always loaded from the current Python environment (pip-installed).
System MKL installations are deliberately ignored to prevent ABI mismatches.
"""

from __future__ import annotations

import ctypes
import os
import sys
from importlib import import_module
from pathlib import Path
from typing import List

_RTLD_GLOBAL = getattr(ctypes, "RTLD_GLOBAL", 0)
_MKL_READY = False
_USE_MKL = True


def _pip_mkl_dirs() -> List[Path]:
    """Return MKL library directories from the current Python environment.

    pip-installed MKL places libraries at predictable locations relative to
    ``sys.prefix``:
      - Windows: ``<prefix>/Library/bin`` (DLLs)
      - Linux:   ``<prefix>/lib``
      - macOS:   ``<prefix>/lib``
    """
    dirs: List[Path] = []
    seen: set[Path] = set()

    prefixes = {
        Path(sys.prefix),
        Path(getattr(sys, "base_prefix", sys.prefix)),
        Path(sys.exec_prefix),
    }

    if os.name == "nt":
        suffixes = ("Library/bin", "Library/lib")
    else:
        suffixes = ("lib", "lib64")

    for prefix in prefixes:
        for suffix in suffixes:
            candidate = prefix / suffix
            try:
                resolved = candidate.resolve(strict=False)
            except OSError:
                continue
            if resolved.exists() and resolved not in seen:
                seen.add(resolved)
                dirs.append(resolved)

    return dirs


def _prepare_mkl_runtime() -> None:
    """Ensure the Intel MKL runtime is visible to the extension module.

    On Windows (Python >= 3.8), DLL search paths must be registered
    explicitly via ``os.add_dll_directory``.  On Linux/macOS the library
    directory is prepended to ``LD_LIBRARY_PATH``/``DYLD_LIBRARY_PATH``
    and the MKL runtime library is pre-loaded with global visibility so
    that the extension module can resolve MKL symbols.
    """
    global _MKL_READY
    if _MKL_READY or not _USE_MKL:
        return

    mkl_dirs = _pip_mkl_dirs()

    if os.name == "nt":
        for directory in mkl_dirs:
            try:
                os.add_dll_directory(str(directory))
            except (OSError, AttributeError):
                continue
        _MKL_READY = True
        return

    # Linux / macOS: prepend pip MKL lib dir to the library search path
    # and pre-load the MKL runtime with RTLD_GLOBAL visibility.
    if sys.platform == "darwin":
        path_var = "DYLD_LIBRARY_PATH"
        rt_names = ("libmkl_rt.dylib", "libmkl_rt.*.dylib")
        omp_names = ("libiomp5.dylib",)
    else:
        path_var = "LD_LIBRARY_PATH"
        rt_names = ("libmkl_rt.so", "libmkl_rt.so.*")
        omp_names = ("libiomp5.so", "libiomp5.so.*")

    # Add dirs to library search path
    existing = os.environ.get(path_var, "").split(os.pathsep)
    for d in mkl_dirs:
        if str(d) not in existing:
            existing.insert(0, str(d))
    os.environ[path_var] = os.pathsep.join(p for p in existing if p)

    # Pre-load MKL runtime and OpenMP
    for directory in mkl_dirs:
        for patterns in (rt_names, omp_names):
            for pattern in patterns:
                for match in directory.glob(pattern):
                    if match.is_file():
                        try:
                            ctypes.CDLL(str(match), mode=_RTLD_GLOBAL)
                        except OSError:
                            continue

    _MKL_READY = True


def load_mkl_runtime() -> None:
    """Public helper to (re)run the MKL loader."""
    _prepare_mkl_runtime()


if _USE_MKL:
    _prepare_mkl_runtime()

_EXTENSION = import_module(".pycanha_core", __name__)
_EXTENSION.load_mkl_runtime = load_mkl_runtime
_EXTENSION.__dict__.setdefault(
    "__all__", ["gmm", "tmm", "parameters", "solvers", "NodeType", "print_package_info"]
)
_EXTENSION.__path__ = [str(Path(__file__).resolve().parent)]

sys.modules[__name__] = _EXTENSION
