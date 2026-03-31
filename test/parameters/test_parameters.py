"""Test bindings for Parameters class."""

import numpy as np
import pytest

import pycanha_core as pcc

Parameters = pcc.parameters.Parameters


class TestParameters:
    def test_construction(self):
        params = Parameters()
        assert params.size() == 0

    def test_add_and_get_double(self):
        params = Parameters()
        params.add_parameter("k", 10.0)
        assert params.get_parameter("k") == pytest.approx(10.0)

    def test_add_and_get_int(self):
        params = Parameters()
        params.add_parameter("n", 42)
        assert params.get_parameter("n") == 42

    def test_add_and_get_bool(self):
        params = Parameters()
        params.add_parameter("flag", True)
        assert params.get_parameter("flag") is True

    def test_add_and_get_string(self):
        params = Parameters()
        params.add_parameter("label", "hello")
        assert params.get_parameter("label") == "hello"

    def test_add_and_get_numpy_double_matrix(self):
        params = Parameters()
        mat = np.array([[1.0, 2.0], [3.0, 4.0]])
        params.add_parameter("mat", mat)
        retrieved = params.get_parameter("mat")
        np.testing.assert_allclose(retrieved, mat)

    def test_set_parameter(self):
        params = Parameters()
        params.add_parameter("k", 10.0)
        params.set_parameter("k", 20.0)
        assert params.get_parameter("k") == pytest.approx(20.0)

    def test_contains(self):
        params = Parameters()
        params.add_parameter("x", 1.0)
        assert params.contains("x") is True
        assert params.contains("y") is False

    def test_remove_parameter(self):
        params = Parameters()
        params.add_parameter("x", 1.0)
        params.remove_parameter("x")
        assert params.contains("x") is False
        assert params.size() == 0

    def test_size(self):
        params = Parameters()
        params.add_parameter("a", 1.0)
        params.add_parameter("b", 2.0)
        assert params.size() == 2

    def test_data_property(self):
        params = Parameters()
        params.add_parameter("k", 10.0)
        params.add_parameter("name", "test")
        data = params.data
        assert isinstance(data, dict)
        assert "k" in data
        assert "name" in data

    def test_get_memory_address(self):
        params = Parameters()
        params.add_parameter("k", 10.0)
        addr = params.get_memory_address("k")
        assert isinstance(addr, int)
        assert addr != 0

    def test_get_idx(self):
        params = Parameters()
        params.add_parameter("first", 1.0)
        params.add_parameter("second", 2.0)
        idx = params.get_idx("first")
        assert isinstance(idx, int)

    def test_get_size_of_parameter(self):
        params = Parameters()
        params.add_parameter("k", 10.0)
        size = params.get_size_of_parameter("k")
        assert isinstance(size, int)
        assert size > 0
