"""Test bindings for Coupling class."""

import pytest

import pycanha_core as pcc

Coupling = pcc.tmm.Coupling


class TestCoupling:
    def test_construction(self):
        c = Coupling(1, 2, 10.0)
        assert c.node_1 == 1
        assert c.node_2 == 2
        assert c.value == pytest.approx(10.0)

    def test_node_setters(self):
        c = Coupling(1, 2, 10.0)
        c.node_1 = 5
        c.node_2 = 6
        assert c.node_1 == 5
        assert c.node_2 == 6

    def test_value_setter(self):
        c = Coupling(1, 2, 10.0)
        c.value = 99.9
        assert c.value == pytest.approx(99.9)
