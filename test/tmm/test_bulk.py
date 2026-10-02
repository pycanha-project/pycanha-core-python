"""Bulk node and coupling insertion, and the bulk getters and setters."""

import numpy as np
import pytest

import pycanha_core as pcc

tmm = pcc.tmm


def _model_with_nodes(numbers, boundary=()):
    model = tmm.ThermalMathematicalModel("bulk")
    model.add_nodes(np.asarray(numbers, dtype=np.int32))
    if boundary:
        model.add_nodes(np.asarray(boundary, dtype=np.int32), type=tmm.NodeType.BOUNDARY)
    return model


class TestAddNodes:
    def test_arrays_land_on_their_nodes(self):
        model = tmm.ThermalMathematicalModel("nodes")
        numbers = np.array([5, 1, 3], dtype=np.int32)
        report = model.add_nodes(
            numbers,
            T=np.array([50.0, 10.0, 30.0]),
            C=np.array([5.0, 1.0, 3.0]),
            qi=np.array([0.0, 2.0, 0.0]),
        )
        assert report.accepted == 3
        assert report.rejected == 0
        nodes = model.nodes
        assert list(nodes.node_numbers()) == [1, 3, 5]
        assert nodes.get_T(5) == 50.0
        assert nodes.get_C(3) == 3.0
        assert nodes.get_qi(1) == 2.0
        assert nodes.get_qi(5) == 0.0

    @pytest.mark.parametrize("dtype", [np.int32, np.int64, np.uint32, np.int16])
    def test_any_integer_dtype(self, dtype):
        model = tmm.ThermalMathematicalModel("dtypes")
        report = model.add_nodes(np.arange(1, 11, dtype=dtype))
        assert report.accepted == 10
        assert model.nodes.num_nodes == 10

    def test_out_of_range_numbers_are_rejected_not_wrapped(self):
        model = tmm.ThermalMathematicalModel("range")
        report = model.add_nodes(np.array([1, 2**32 + 1], dtype=np.int64))
        assert report.accepted == 0
        assert report.rejected == 2
        assert "int32" in report.first_rejections[0]
        assert model.nodes.num_nodes == 0

    def test_duplicates_are_rejected_and_reported(self):
        model = _model_with_nodes([1, 2, 3])
        report = model.add_nodes(np.array([2, 4, 4, 5], dtype=np.int32))
        assert report.accepted == 1
        assert report.rejected == 3
        assert list(model.nodes.node_numbers()) == [1, 2, 3, 5]

    def test_non_contiguous_input(self):
        model = tmm.ThermalMathematicalModel("strided")
        numbers = np.arange(1, 21, dtype=np.int32)[::2]
        temperatures = np.arange(20, dtype=np.float64)[::2]
        assert model.add_nodes(numbers, T=temperatures).accepted == 10
        assert model.nodes.get_T(19) == 18.0

    def test_mismatched_attribute_rejects_the_batch(self):
        model = tmm.ThermalMathematicalModel("mismatch")
        report = model.add_nodes(np.array([1, 2], dtype=np.int32), C=np.array([1.0]))
        assert report.rejected == 2
        assert model.nodes.num_nodes == 0

    def test_available_on_nodes_and_network(self):
        model = tmm.ThermalMathematicalModel("forward")
        assert model.nodes.add_nodes(np.array([1, 2], dtype=np.int32)).accepted == 2
        assert model.network.add_nodes(np.array([3], dtype=np.int32)).accepted == 1
        assert model.nodes.num_nodes == 3


class TestAddCouplings:
    def test_sorted_grid_is_appended(self):
        model = _model_with_nodes(range(1, 101))
        node_1 = np.arange(1, 100, dtype=np.int32)
        node_2 = node_1 + 1
        report = model.add_conductive_couplings(node_1, node_2, np.full(99, 0.5))
        assert report.accepted == 99
        couplings = model.conductive_couplings
        assert couplings.get_coupling_value(50, 51) == 0.5
        assert len(couplings.to_arrays()[2]) == 99

    @pytest.mark.parametrize(
        ("merge", "expected"),
        [
            (tmm.CouplingMerge.OVERWRITE, 3.0),
            (tmm.CouplingMerge.SUM, 6.0),
            (tmm.CouplingMerge.NEW, 1.0),
        ],
    )
    def test_merge_modes(self, merge, expected):
        model = _model_with_nodes([1, 2, 3])
        couplings = model.conductive_couplings
        couplings.add_couplings(
            np.array([1], dtype=np.int32), np.array([2], dtype=np.int32), np.array([1.0])
        )
        report = couplings.add_couplings(
            np.array([2, 1], dtype=np.int64),
            np.array([1, 2], dtype=np.int64),
            np.array([2.0, 3.0]),
            merge=merge,
        )
        assert report.merged == 2
        assert couplings.get_coupling_value(1, 2) == expected

    def test_bad_entries_are_dropped(self):
        model = _model_with_nodes([1, 2, 3], boundary=[10])
        report = model.add_radiative_couplings(
            np.array([1, 1, 2, 99, 3], dtype=np.int64),
            np.array([2, 1, 3, 1, 10], dtype=np.int64),
            np.array([1.0, 1.0, -1.0, 1.0, np.nan]),
        )
        assert report.accepted == 1
        assert report.rejected == 4
        assert model.radiative_couplings.get_coupling_value(1, 2) == 1.0

    def test_unsorted_input_matches_per_element_calls(self):
        rng = np.random.default_rng(38)
        numbers = np.arange(1, 51, dtype=np.int32)
        pairs = rng.integers(1, 51, size=(400, 2))
        pairs = pairs[pairs[:, 0] != pairs[:, 1]]
        values = rng.uniform(0.1, 2.0, size=len(pairs))

        bulk = _model_with_nodes(numbers)
        bulk.add_conductive_couplings(pairs[:, 0], pairs[:, 1], values)

        reference = _model_with_nodes(numbers)
        for (first, second), value in zip(pairs, values, strict=True):
            reference.add_conductive_coupling(int(first), int(second), float(value))

        got = bulk.conductive_couplings.to_arrays()
        want = reference.conductive_couplings.to_arrays()
        for got_array, want_array in zip(got, want, strict=True):
            np.testing.assert_array_equal(got_array, want_array)

    def test_append_by_internal_index(self):
        model = _model_with_nodes(range(1, 7))
        couplings = model.conductive_couplings
        index = np.array([0, 1], dtype=np.int32)
        report = couplings.append_couplings(index, index + 1, np.array([1.0, 2.0]), offset=3)
        assert report.accepted == 2
        assert couplings.get_coupling_value(4, 5) == 1.0
        assert couplings.get_coupling_value(5, 6) == 2.0
        # Out of order against what is stored: rejected whole.
        late = couplings.append_couplings(index, index + 1, np.array([1.0, 1.0]))
        assert late.rejected == 2


class TestBulkValues:
    def test_node_values_round_trip(self):
        model = _model_with_nodes([1, 2, 3, 4])
        nodes = model.nodes
        report = nodes.set_values(
            tmm.NodeAttribute.QI, np.array([4, 2], dtype=np.int32), np.array([4.0, 2.0])
        )
        assert report.accepted == 2
        np.testing.assert_array_equal(
            nodes.get_values(tmm.NodeAttribute.QI), [0.0, 2.0, 0.0, 4.0]
        )
        read = nodes.get_values(tmm.NodeAttribute.QI, np.array([2, 9], dtype=np.int32))
        assert read[0] == 2.0
        assert np.isnan(read[1])

    def test_coupling_values_round_trip(self):
        model = _model_with_nodes([1, 2, 3])
        couplings = model.conductive_couplings
        couplings.add_couplings(
            np.array([1, 2], dtype=np.int32), np.array([2, 3], dtype=np.int32), np.array([1.0, 2.0])
        )
        first = np.array([2, 1], dtype=np.int32)
        second = np.array([3, 3], dtype=np.int32)
        values = couplings.get_values(first, second)
        assert values[0] == 2.0
        assert np.isnan(values[1])
        assert couplings.set_values(first, second, np.array([5.0, 5.0])).rejected == 1
        node_1, node_2, stored = couplings.to_arrays()
        np.testing.assert_array_equal(node_1, [1, 2])
        np.testing.assert_array_equal(node_2, [2, 3])
        np.testing.assert_array_equal(stored, [1.0, 5.0])

    def test_structure_version_moves_with_the_nodes(self):
        model = _model_with_nodes([1, 2])
        version = model.nodes.structure_version
        model.add_node(3)
        assert model.nodes.structure_version != version


class TestSetTypes:
    @pytest.mark.parametrize(
        "edge", [[1, 4], np.array([1, 4], dtype=np.int32), np.array([1, 4], dtype=np.int64)]
    )
    def test_nodes_become_boundaries_with_their_couplings(self, edge):
        model = _model_with_nodes([1, 2, 3, 4])
        model.nodes.set_values(
            tmm.NodeAttribute.T, np.array([1, 2, 3, 4], dtype=np.int32), np.array([1.0, 2.0, 3.0, 4.0])
        )
        model.add_conductive_couplings(
            np.array([1, 2, 3], dtype=np.int32), np.array([2, 3, 4], dtype=np.int32), np.array([1.0, 2.0, 3.0])
        )
        report = model.nodes.set_types(edge, tmm.NodeType.BOUNDARY)
        assert report.accepted == 2
        assert model.nodes.get_type(1) == tmm.NodeType.BOUNDARY
        assert model.nodes.get_type(2) == tmm.NodeType.DIFFUSIVE
        assert list(model.nodes.node_numbers()) == [2, 3, 1, 4]
        assert model.nodes.get_T(4) == 4.0
        couplings = model.conductive_couplings
        assert couplings.get_coupling_value(1, 2) == 1.0
        assert couplings.get_coupling_value(3, 4) == 3.0

    def test_unknown_and_out_of_range_numbers(self):
        model = _model_with_nodes([1, 2])
        report = model.nodes.set_types([2, 9], tmm.NodeType.BOUNDARY)
        assert report.accepted == 1
        assert report.rejected == 1
        wide = model.nodes.set_types(np.array([2**32 + 1], dtype=np.int64), tmm.NodeType.DIFFUSIVE)
        assert wide.rejected == 1
        assert model.nodes.get_type(1) == tmm.NodeType.DIFFUSIVE

    def test_single_set_type_moves_the_node(self):
        model = _model_with_nodes([1, 2, 3])
        assert model.nodes.set_type(2, tmm.NodeType.BOUNDARY)
        assert model.nodes.get_type(2) == tmm.NodeType.BOUNDARY
        assert model.nodes.set_type(2, tmm.NodeType.BOUNDARY)  # already: still True


class TestNumbersOutsideInt32:
    """An int64 number that does not fit NodeNum never lands on another node.

    numpy would wrap 2**32 + 1 to 1 when converting to int32; every bulk call
    takes the int64 path instead and treats such a number as an unknown node.
    """

    WIDE = 2**32 + 1  # wraps to node 1 in int32

    def test_node_getter_reads_nan(self):
        model = _model_with_nodes([1, 2])
        model.nodes.set_T(1, 10.0)
        read = model.nodes.get_values(
            tmm.NodeAttribute.T, np.array([self.WIDE, 1], dtype=np.int64)
        )
        assert np.isnan(read[0])
        assert read[1] == 10.0

    def test_node_setter_skips_and_reports(self):
        model = _model_with_nodes([1, 2])
        report = model.nodes.set_values(
            tmm.NodeAttribute.T, np.array([self.WIDE, 2], dtype=np.int64), np.array([7.0, 8.0])
        )
        assert report.accepted == 1
        assert report.rejected == 1
        assert "int32" in report.first_rejections[0]
        assert model.nodes.get_T(1) == 0.0
        assert model.nodes.get_T(2) == 8.0

    def test_node_setter_length_mismatch(self):
        model = _model_with_nodes([1, 2])
        report = model.nodes.set_values(
            tmm.NodeAttribute.T, np.array([1, 2], dtype=np.int64), np.array([7.0])
        )
        assert report.rejected == 2
        assert model.nodes.get_T(1) == 0.0

    def test_coupling_getter_and_setter(self):
        model = _model_with_nodes([1, 2, 3])
        couplings = model.conductive_couplings
        couplings.add_couplings(
            np.array([1, 2], dtype=np.int32), np.array([2, 3], dtype=np.int32), np.array([1.0, 2.0])
        )
        first = np.array([self.WIDE, 2], dtype=np.int64)
        second = np.array([2, 3], dtype=np.int64)
        read = couplings.get_values(first, second)
        assert np.isnan(read[0])
        assert read[1] == 2.0

        report = couplings.set_values(first, second, np.array([9.0, 4.0]))
        assert report.accepted == 1
        assert report.rejected == 1
        assert couplings.get_coupling_value(1, 2) == 1.0
        assert couplings.get_coupling_value(2, 3) == 4.0

    def test_other_dtypes_take_the_checked_path(self):
        model = _model_with_nodes([1, 2])
        model.nodes.set_values(
            tmm.NodeAttribute.C, np.array([1, 2], dtype=np.uint64), np.array([3.0, 4.0])
        )
        np.testing.assert_array_equal(
            model.nodes.get_values(tmm.NodeAttribute.C, np.array([2, 1], dtype=np.int16)), [4.0, 3.0]
        )

    def test_append_rejects_the_whole_call(self):
        model = _model_with_nodes(range(1, 4))
        couplings = model.conductive_couplings
        report = couplings.append_couplings(
            np.array([0, 2**32], dtype=np.int64),
            np.array([1, 2], dtype=np.int64),
            np.array([1.0, 1.0]),
        )
        assert report.rejected == 2
        assert "int32" in report.first_rejections[0]
        assert len(couplings.to_arrays()[2]) == 0
        # An int64 array that fits goes through.
        ok = couplings.append_couplings(
            np.array([0], dtype=np.int64), np.array([1], dtype=np.int64), np.array([1.0])
        )
        assert ok.accepted == 1


class TestStructureSync:
    def test_container_path_needs_no_radiative_workaround(self):
        """Nodes added through tmm.nodes and conduction only: SSLU sizes the radiative blocks."""
        model = tmm.ThermalMathematicalModel("container")
        for number in (1, 2):
            node = tmm.Node(number)
            node.capacity = 1.0
            model.nodes.add_node(node)
        boundary = tmm.Node(3)
        boundary.type = tmm.NodeType.BOUNDARY
        boundary.T = 300.0
        model.nodes.add_node(boundary)
        model.nodes.set_qi(1, 1.0)
        model.add_conductive_coupling(1, 2, 1.0)
        model.add_conductive_coupling(2, 3, 1.0)

        solver = pcc.solvers.SSLU(model)
        solver.initialize()
        solver.solve()
        assert solver.solver_converged
        assert model.nodes.get_T(1) == pytest.approx(302.0)

    def test_solver_refuses_a_model_changed_after_initialize(self):
        model = _model_with_nodes([1, 2], boundary=[3])
        model.nodes.set_values(
            tmm.NodeAttribute.T, np.array([1, 2, 3], dtype=np.int32), np.array([5.0, 5.0, 300.0])
        )
        model.add_conductive_couplings(
            np.array([1, 2], dtype=np.int32), np.array([2, 3], dtype=np.int32), np.array([1.0, 1.0])
        )
        solver = pcc.solvers.SSLU(model)
        solver.initialize()
        model.add_nodes(np.array([4], dtype=np.int32), C=np.array([1.0]))
        solver.solve()
        assert not solver.solver_converged
        assert model.nodes.get_T(1) == 5.0

    def test_nodes_inserted_mid_block_keep_their_couplings(self):
        model = _model_with_nodes([10, 30])
        model.add_conductive_coupling(10, 30, 2.0)
        model.add_nodes(np.array([20], dtype=np.int32))
        model.nodes.add_node(tmm.Node(5))
        couplings = model.conductive_couplings
        assert couplings.get_coupling_value(10, 30) == 2.0
        # Nothing shifted onto the new neighbours.
        read = couplings.get_values(
            np.array([10, 20, 5], dtype=np.int32), np.array([20, 30, 10], dtype=np.int32)
        )
        assert np.isnan(read).all()


class TestSmallerEntryPoints:
    def test_nodes_add_nodes_from_node_objects(self):
        model = tmm.ThermalMathematicalModel("objects")
        first = tmm.Node(2)
        first.T = 20.0
        boundary = tmm.Node(9)
        boundary.type = tmm.NodeType.BOUNDARY
        model.nodes.add_nodes([first, tmm.Node(1), boundary])
        assert list(model.nodes.node_numbers()) == [1, 2, 9]
        assert model.nodes.get_T(2) == 20.0
        assert model.nodes.get_type(9) == tmm.NodeType.BOUNDARY

    def test_network_synchronize_structure(self):
        network = tmm.ThermalNetwork()
        for number in (1, 2, 3):
            network.nodes.add_node(tmm.Node(number))
        network.synchronize_structure()
        matrices = network.radiative_couplings.matrices()
        assert matrices.num_diff_nodes == 3
