"""Test bindings for ThermalMathematicalModel."""

import pytest

import pycanha_core as pcc

Coupling = pcc.tmm.Coupling
Node = pcc.tmm.Node
NodeType = pcc.tmm.NodeType
ThermalMathematicalModel = pcc.tmm.ThermalMathematicalModel


class TestThermalMathematicalModel:
    def test_construction(self):
        tmm = ThermalMathematicalModel("my_model")
        assert tmm.name == "my_model"

    def test_name_setter(self):
        tmm = ThermalMathematicalModel("old")
        tmm.name = "new"
        assert tmm.name == "new"

    def test_time_roundtrip(self):
        tmm = ThermalMathematicalModel("m")
        tmm.time = 100.0
        assert tmm.time == pytest.approx(100.0)

    def test_add_node_object(self):
        tmm = ThermalMathematicalModel("m")
        node = Node(1)
        node.T = 300.0
        tmm.add_node(node)
        assert tmm.nodes.num_nodes == 1
        assert tmm.nodes.get_T(1) == pytest.approx(300.0)

    def test_add_node_by_number(self):
        tmm = ThermalMathematicalModel("m")
        tmm.add_node(1)
        assert tmm.nodes.num_nodes == 1
        assert tmm.nodes.is_node(1) is True

    def test_add_conductive_coupling_by_numbers(self):
        tmm = ThermalMathematicalModel("m")
        tmm.add_node(1)
        tmm.add_node(2)
        tmm.add_conductive_coupling(1, 2, 10.0)
        assert tmm.conductive_couplings.get_coupling_value(1, 2) == pytest.approx(10.0)

    def test_add_conductive_coupling_from_object(self):
        tmm = ThermalMathematicalModel("m")
        tmm.add_node(1)
        tmm.add_node(2)
        tmm.add_conductive_coupling(Coupling(1, 2, 7.5))
        assert tmm.conductive_couplings.get_coupling_value(1, 2) == pytest.approx(7.5)

    def test_add_radiative_coupling_by_numbers(self):
        tmm = ThermalMathematicalModel("m")
        tmm.add_node(1)
        tmm.add_node(2)
        tmm.add_radiative_coupling(1, 2, 0.5)
        assert tmm.radiative_couplings.get_coupling_value(1, 2) == pytest.approx(0.5)

    def test_add_radiative_coupling_from_object(self):
        tmm = ThermalMathematicalModel("m")
        tmm.add_node(1)
        tmm.add_node(2)
        tmm.add_radiative_coupling(Coupling(1, 2, 0.3))
        assert tmm.radiative_couplings.get_coupling_value(1, 2) == pytest.approx(0.3)

    def test_network_property(self):
        tmm = ThermalMathematicalModel("m")
        tmm.add_node(1)
        assert tmm.network.nodes.num_nodes == 1

    def test_parameters_property(self):
        tmm = ThermalMathematicalModel("m")
        params = tmm.parameters
        params.add_parameter("k", 10.0)
        assert tmm.parameters.get_parameter("k") == pytest.approx(10.0)

    def test_formulas_property(self):
        tmm = ThermalMathematicalModel("m")
        formulas = tmm.formulas
        assert formulas is not None

    def test_thermal_data_property(self):
        tmm = ThermalMathematicalModel("m")
        td = tmm.thermal_data
        assert td is not None

    def test_callback_flags_default(self):
        tmm = ThermalMathematicalModel("m")
        assert isinstance(tmm.callbacks_active, bool)
        assert isinstance(tmm.python_callbacks_active, bool)
        assert isinstance(tmm.internal_callbacks_active, bool)

    def test_callback_flags_settable(self):
        tmm = ThermalMathematicalModel("m")
        tmm.callbacks_active = True
        tmm.python_callbacks_active = True
        tmm.internal_callbacks_active = True
        assert tmm.callbacks_active is True
        assert tmm.python_callbacks_active is True
        assert tmm.internal_callbacks_active is True

    def test_python_callback_assignment(self):
        tmm = ThermalMathematicalModel("m")
        call_count = 0

        def my_callback():
            nonlocal call_count
            call_count += 1

        tmm.python_extern_callback_solver_loop = my_callback
        tmm.python_extern_callback_transient_time_change = my_callback
        tmm.python_extern_callback_transient_after_timestep = my_callback
        # Just verify assignment doesn't raise
        assert tmm.python_extern_callback_solver_loop is not None

    def test_mixed_node_types(self):
        tmm = ThermalMathematicalModel("m")
        d = Node(1)
        d.type = NodeType.DIFFUSIVE
        b = Node(99)
        b.type = NodeType.BOUNDARY
        tmm.add_node(d)
        tmm.add_node(b)
        assert tmm.nodes.num_diff_nodes == 1
        assert tmm.nodes.num_bound_nodes == 1
