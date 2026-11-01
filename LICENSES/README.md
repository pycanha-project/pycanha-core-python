# Third-party licenses

The `pycanha-core` Python package is released under the [MIT License](../LICENSE).

Its compiled extension contains pycanha-core and the libraries compiled into
it, plus the nanobind binding library. Their license files travel with every
wheel (under `*.dist-info/licenses/`). Each folder holds the project's own
license files, copied unmodified.

| Component | Version | License | Files |
|---|---|---|---|
| [pycanha-core](https://github.com/pycanha-project/pycanha-core) and the libraries compiled into it | 0.24 | MIT, plus the licenses listed in its own index | [pycanha-core/](pycanha-core/) ([index](pycanha-core/README.md)) |
| [nanobind](https://github.com/wjakob/nanobind) | 3.1.0 | BSD-3-Clause | [nanobind/](nanobind/) |
| [robin-map](https://github.com/Tessil/robin-map) (bundled in nanobind) | the copy in nanobind 3.1.0 | MIT | [robin-map/](robin-map/) |

The package's `license` field in `pyproject.toml` is the SPDX expression of
everything above: `MIT AND BSD-3-Clause AND Apache-2.0 AND MPL-2.0 AND BSL-1.0
AND Zlib`.

## Keeping this up to date

1. At every pycanha-core bump, replace `pycanha-core/` with the core's
   `LICENSES/` folder at the matching tag (`git -C ../pycanha-core archive
   v<major.minor> LICENSES`) and update its version above. If its index lists a
   new license, add it to `license` in `pyproject.toml`.
2. When the nanobind requirement in `pyproject.toml` changes, copy its
   `LICENSE` and the one of its bundled robin-map again, and update the rows.

Only code that ends up inside the extension is listed. Runtime dependencies
installed as their own wheels (NumPy, SciPy, MKL) carry their own licenses.
