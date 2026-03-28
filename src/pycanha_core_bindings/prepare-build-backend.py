"""Custom build backend that runs Conan steps before scikit-build-core."""

from __future__ import annotations

# Guard against direct execution of this file
if __name__ == "__main__":
    raise TypeError(
        "This module cannot be executed directly. It should be called by pip install."
    )


import json
import os
import re
import subprocess
from pathlib import Path
from typing import Any, Dict, Iterable, Optional

import scikit_build_core.build as _scikit_build
import toml
from scikit_build_core.build import *  # noqa: F401,F403 - re-export build backend hooks

_SCRIPT_DIR = Path(__file__).resolve().parent
_PROJECT_ROOT = _SCRIPT_DIR.parent.parent
_CORE_SOURCE_DIR = (_PROJECT_ROOT.parent / "pycanha-core").resolve()
_PROFILE_PATH = _SCRIPT_DIR / "pycanha-core-conan-profile"
_CORE_OUTPUT_DIR = _SCRIPT_DIR / "build" / "pycanha-core-package"
_PYCORE_INIT_TEMPLATE = _PROJECT_ROOT / "src" / "pycanha_core" / "__init__.py.in"
_PYCORE_INIT_OUTPUT = _PROJECT_ROOT / "src" / "pycanha_core" / "__init__.py"

_CONAN_PREPARED = False
_LAST_CONAN_MKL: Optional[bool] = None
_PYPROJECT_CACHE: Optional[Dict[str, Any]] = None


def _get_pyproject() -> Dict[str, Any]:
    global _PYPROJECT_CACHE
    if _PYPROJECT_CACHE is None:
        _PYPROJECT_CACHE = toml.load(_PROJECT_ROOT / "pyproject.toml")
    return _PYPROJECT_CACHE


def _load_bindings_version() -> str:
    pyproject = _get_pyproject()
    return ".".join(pyproject["project"]["version"].split(".")[:2])


def _load_mkl_version() -> Optional[str]:
    pyproject = _get_pyproject()
    optional = pyproject.get("project", {}).get("optional-dependencies", {})
    mkl_deps = optional.get("mkl")
    if not mkl_deps:
        return None

    for dep in mkl_deps:
        match = re.match(r"mkl==([\w\.\-]+)", dep)
        if match:
            return match.group(1)

    return None


def _write_bindings_conanfile(version: str) -> None:
    template_path = _SCRIPT_DIR / "conanfile.txt.in"
    output_path = _SCRIPT_DIR / "conanfile.txt"
    content = template_path.read_text()
    output_path.write_text(content.replace("@CONFIG_PYCANHA_CORE_VERSION@", version))


def _render_python_init(use_mkl: bool) -> None:
    if not _PYCORE_INIT_TEMPLATE.is_file():
        return

    template = _PYCORE_INIT_TEMPLATE.read_text()
    rendered = template.replace("@PYCANHA_USE_MKL@", "True" if use_mkl else "False")

    if _PYCORE_INIT_OUTPUT.exists():
        current = _PYCORE_INIT_OUTPUT.read_text()
        if current == rendered:
            return

    _PYCORE_INIT_OUTPUT.write_text(rendered)


def _verify_core_version(expected_version: str) -> None:
    conanfile = _CORE_SOURCE_DIR / "conanfile.py"
    if not conanfile.is_file():
        return

    content = conanfile.read_text()
    match = re.search(r'version\s*=\s*"(\d+\.\d+)"', content)
    if match is None:
        raise RuntimeError(
            "Could not find version in pycanha-core/conanfile.py. "
            f"Required version is {expected_version}"
        )

    if match.group(1) != expected_version:
        raise RuntimeError(
            "Version mismatch. pycanha-core version is "
            f"{match.group(1)}, but bindings version is {expected_version}"
        )


def _extract_config_value(
    config_settings: Optional[Dict[str, Any]], key: str
) -> Optional[str]:
    if not config_settings:
        return None

    for candidate in (key, key.lower(), key.upper()):
        if candidate in config_settings:
            value = config_settings[candidate]
            if isinstance(value, (list, tuple)):
                return None if not value else str(value[-1])
            return str(value)
    return None


def _parse_bool(value: Optional[str], *, default: bool = False) -> bool:
    if value is None or value == "":
        return default

    normalized = value.strip().lower()
    if normalized in {"1", "true", "on", "yes"}:
        return True
    if normalized in {"0", "false", "off", "no"}:
        return False
    raise ValueError(f"Invalid boolean value '{value}' for PYCANHA_OPTION_USE_MKL")


def _resolve_mkl_option(config_settings: Optional[Dict[str, Any]]) -> bool:
    raw = _extract_config_value(config_settings, "PYCANHA_OPTION_USE_MKL")
    if raw is None:
        raw = os.environ.get("PYCANHA_OPTION_USE_MKL")
    return _parse_bool(raw, default=False)


def _run_command(command: Iterable[str], *, cwd: Optional[Path] = None) -> None:
    process = subprocess.run(
        list(command),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        cwd=None if cwd is None else str(cwd),
        check=False,
    )

    if process.returncode != 0:
        stdout = process.stdout.decode()
        stderr = process.stderr.decode()
        raise RuntimeError(
            f"Command failed: {' '.join(command)}\nstdout:\n{stdout}\nstderr:\n{stderr}"
        )


def _invoke_conan(use_mkl: bool) -> None:
    option_value = "True" if use_mkl else "False"
    option_flag = f"pycanha-core*:PYCANHA_OPTION_USE_MKL={option_value}"
    mkl_version = _load_mkl_version()
    version_flag = (
        ["-o", f"pycanha-core*:PYCANHA_OPTION_MKL_VERSION={mkl_version}"]
        if mkl_version
        else []
    )

    if _CORE_SOURCE_DIR.is_dir():
        _CORE_OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

        # Keep the local pycanha-core recipe in the cache so install/build see recent edits
        export_cmd = [
            "conan",
            "export",
            str(_CORE_SOURCE_DIR),
        ]
        _run_command(export_cmd, cwd=_PROJECT_ROOT)

        core_install_cmd = [
            "conan",
            "install",
            str(_CORE_SOURCE_DIR),
            f"-pr:h={_PROFILE_PATH}",
            f"-pr:b={_PROFILE_PATH}",
            "--build=missing",
            "-of",
            str(_CORE_OUTPUT_DIR),
            "-o",
            option_flag,
        ] + version_flag
        _run_command(core_install_cmd, cwd=_PROJECT_ROOT)

        core_build_cmd = [
            "conan",
            "build",
            str(_CORE_SOURCE_DIR),
            f"-pr:h={_PROFILE_PATH}",
            f"-pr:b={_PROFILE_PATH}",
            "-of",
            str(_CORE_OUTPUT_DIR),
            "-o",
            option_flag,
        ] + version_flag
        _run_command(core_build_cmd, cwd=_PROJECT_ROOT)

    bindings_install_cmd = [
        "conan",
        "install",
        str(_SCRIPT_DIR),
        "--build=missing",
        f"-pr:h={_PROFILE_PATH}",
        f"-pr:b={_PROFILE_PATH}",
        "-o",
        option_flag,
    ] + version_flag
    _run_command(bindings_install_cmd, cwd=_PROJECT_ROOT)


def _patch_cmake_presets() -> None:
    files_to_patch = []

    # Conan may emit presets directly in CMakePresets.json, or via includes
    # from CMakeUserPresets.json depending on Conan version/config.
    for candidate in (
        _SCRIPT_DIR / "CMakePresets.json",
        _SCRIPT_DIR / "CMakeUserPresets.json",
    ):
        if candidate.is_file():
            files_to_patch.append(candidate)

    user_presets = _SCRIPT_DIR / "CMakeUserPresets.json"
    if user_presets.is_file():
        try:
            data = json.loads(user_presets.read_text())
        except json.JSONDecodeError:
            data = {}

        for include_file in data.get("include", []):
            include_path = (user_presets.parent / include_file).resolve()
            if include_path.is_file():
                files_to_patch.append(include_path)

    seen_paths = set()
    unique_files = []
    for preset_file in files_to_patch:
        path_str = str(preset_file)
        if path_str in seen_paths:
            continue
        seen_paths.add(path_str)
        unique_files.append(preset_file)

    for preset_file in unique_files:
        content = preset_file.read_text()
        patched = content.replace("conan-default", "conan-release")
        patched = patched.replace("Unix Makefiles", "Ninja")
        if patched != content:
            preset_file.write_text(patched)


def _prepare_conan(config_settings: Optional[Dict[str, Any]]) -> None:
    global _CONAN_PREPARED, _LAST_CONAN_MKL

    use_mkl = _resolve_mkl_option(config_settings)
    if _CONAN_PREPARED and _LAST_CONAN_MKL == use_mkl:
        return

    bindings_version = _load_bindings_version()
    _write_bindings_conanfile(bindings_version)
    _verify_core_version(bindings_version)
    _render_python_init(use_mkl)

    _invoke_conan(use_mkl)
    _patch_cmake_presets()

    _CONAN_PREPARED = True
    _LAST_CONAN_MKL = use_mkl


_original_build_wheel = _scikit_build.build_wheel


def build_wheel(
    wheel_directory: str,
    config_settings: Optional[Dict[str, Any]] = None,
    metadata_directory: Optional[str] = None,
) -> str:
    _prepare_conan(config_settings)
    return _original_build_wheel(
        wheel_directory,
        config_settings=config_settings,
        metadata_directory=metadata_directory,
    )


_original_build_editable = getattr(_scikit_build, "build_editable", None)


def build_editable(
    requirements_directory: str,
    config_settings: Optional[Dict[str, Any]] = None,
    metadata_directory: Optional[str] = None,
) -> str:
    if _original_build_editable is None:
        raise AttributeError("scikit_build_core.build does not expose build_editable")

    _prepare_conan(config_settings)
    return _original_build_editable(
        requirements_directory,
        config_settings=config_settings,
        metadata_directory=metadata_directory,
    )
