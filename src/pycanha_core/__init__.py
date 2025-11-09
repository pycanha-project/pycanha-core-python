"""Python package bootstrap for the compiled ``pycanha_core`` extension.

This module ensures the Intel MKL runtime shipped via ``mkl``/``mkl-devel``
Python wheels is made available before the native extension is loaded.
"""

from __future__ import annotations

import ctypes
import os
import sys
from importlib import import_module
from pathlib import Path
from typing import Iterable, List, Optional, Tuple


_RTLD_GLOBAL = getattr(ctypes, "RTLD_GLOBAL", 0)
_MKL_READY = False


def _gather_candidate_dirs() -> List[Path]:
    """Collect likely directories containing MKL shared libraries."""

    def _add(paths: List[Path], seen: set[Path], candidate: Path) -> None:
        try:
            resolved = candidate.resolve(strict=False)
        except OSError:
            return
        if resolved.exists() and resolved not in seen:
            seen.add(resolved)
            paths.append(resolved)

    collected: List[Path] = []
    seen: set[Path] = set()

    for env_key in ("MKLROOT", "MKL_ROOT"):
        env_val = os.environ.get(env_key)
        if not env_val:
            continue
        root = Path(env_val)
        for suffix in ("", "lib", "lib64", "lib/intel64", "lib/intel64_lin"):
            _add(collected, seen, root / suffix)

    prefixes = {
        Path(sys.prefix),
        Path(getattr(sys, "base_prefix", sys.prefix)),
        Path(sys.exec_prefix),
    }
    for prefix in prefixes:
        for suffix in ("", "lib", "lib64", "Library/lib"):
            _add(collected, seen, prefix / suffix)

    try:
        import site
    except Exception:  # pragma: no cover - site may be unavailable in rare setups
        site_dirs: Tuple[Path, ...] = ()
    else:
        site_dirs = (
            Path(site.getuserbase()) / "lib",
            Path(site.getuserbase()) / "Library/lib",
        )
    for entry in site_dirs:
        _add(collected, seen, entry)

    for raw in os.environ.get("LD_LIBRARY_PATH", "").split(os.pathsep):
        if raw:
            _add(collected, seen, Path(raw))

    return collected


def _locate_library(patterns: Iterable[str]) -> Optional[Path]:
    """Return the first library matching any of the glob patterns."""

    candidates = _gather_candidate_dirs()
    for directory in candidates:
        for pattern in patterns:
            for match in directory.glob(pattern):
                if match.is_file():
                    return match.resolve()

    for directory in candidates:
        for pattern in patterns:
            try:
                iterator = directory.rglob(pattern)
            except (PermissionError, OSError):
                continue
            for match in iterator:
                if match.is_file():
                    return match.resolve()

    return None


def _load_shared_library(path: Path) -> None:
    """Attempt to load a shared library with RTLD_GLOBAL visibility."""

    try:
        ctypes.CDLL(str(path), mode=_RTLD_GLOBAL)
    except OSError as exc:  # pragma: no cover - depends on host configuration
        raise ImportError(f"Failed to load shared library '{path}'.") from exc


def _prepare_mkl_runtime() -> None:
    """Ensure the Intel MKL runtime is visible to the extension module."""

    global _MKL_READY
    if _MKL_READY:
        return

    if os.name == "nt":  # pragma: no cover - Windows not exercised in CI
        for directory in _gather_candidate_dirs():
            try:
                os.add_dll_directory(str(directory))
            except (OSError, AttributeError):
                continue
        _MKL_READY = True
        return

    if sys.platform == "darwin":
        runtime_patterns = ("libmkl_rt.dylib", "libmkl_rt.*.dylib")
        omp_patterns = ("libiomp5.dylib", "libiomp5.*.dylib")
    else:
        runtime_patterns = ("libmkl_rt.so", "libmkl_rt.so.*", "libmkl_rt.*.so")
        omp_patterns = ("libiomp5.so", "libiomp5.so.*")

    runtime = _locate_library(runtime_patterns)
    if runtime is None:
        raise ImportError(
            "Intel MKL runtime libraries could not be located. "
            "Install the 'mkl-devel' wheel inside your virtual environment "
            "or set the MKLROOT environment variable before importing pycanha_core."
        )

    _load_shared_library(runtime)

    omp = _locate_library(omp_patterns)
    if omp is not None:
        _load_shared_library(omp)

    _MKL_READY = True


def load_mkl_runtime() -> None:
    """Public helper to (re)run the MKL loader."""

    _prepare_mkl_runtime()


_prepare_mkl_runtime()

_EXTENSION = import_module(".pycanha_core", __name__)
_EXTENSION.load_mkl_runtime = load_mkl_runtime
_EXTENSION.__dict__.setdefault(
    "__all__", ["gmm", "tmm", "parameters", "solvers", "NodeType", "print_package_info"]
)
_EXTENSION.__path__ = [str(Path(__file__).resolve().parent)]

sys.modules[__name__] = _EXTENSION
