"""Tests for the 0.15 GMM material value classes."""

import pytest

import pycanha_core as pcc

gmm = pcc.gmm


class TestColor:
    def test_channels(self):
        color = gmm.Color(127, 64, 255)
        assert color.red == 127
        assert color.green == 64
        assert color.blue == 255
        assert tuple(color.rgb) == (127, 64, 255)


class TestBulkMaterial:
    def test_construction_and_props(self):
        mat = gmm.BulkMaterial(
            name="aluminum_6061",
            density=2700.0,
            conductivity=167.0,
            specific_heat=896.0,
        )
        assert mat.name == "aluminum_6061"
        assert mat.density == pytest.approx(2700.0)
        assert mat.conductivity == pytest.approx(167.0)
        assert mat.specific_heat == pytest.approx(896.0)

    def test_setter_validation(self):
        mat = gmm.BulkMaterial()
        with pytest.raises((ValueError, RuntimeError)):
            mat.density = -1.0


class TestOpticalMaterial:
    def test_two_property_ctor(self):
        opt = gmm.OpticalMaterial(
            name="white_paint", emissivity_ir=0.88, absorptivity_solar=0.22
        )
        assert opt.name == "white_paint"
        assert opt.emissivity_ir == pytest.approx(0.88)
        assert opt.absorptivity_solar == pytest.approx(0.22)

    def test_full_properties(self):
        props = [0.9, 0.0, 0.0, 0.3, 0.0, 0.0]
        opt = gmm.OpticalMaterial("mat", props)
        assert list(opt.th_optical_properties) == pytest.approx(props)
