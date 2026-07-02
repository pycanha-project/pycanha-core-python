"""Tests for the 0.15 ThermalMesh value class."""

import numpy as np
import pytest

import pycanha_core as pcc

gmm = pcc.gmm


class TestThermalMesh:
    def test_default(self):
        mesh = gmm.ThermalMesh()
        assert mesh.is_valid() is True
        # Default unit square = one face pair.
        assert mesh.num_pair_faces == 1

    def test_uv_cuts(self):
        mesh = gmm.ThermalMesh()
        mesh.dir1_mesh = [0.0, 0.5, 1.0]
        mesh.dir2_mesh = [0.0, 1.0]
        assert list(mesh.dir1_mesh) == pytest.approx([0.0, 0.5, 1.0])
        assert mesh.num_pair_faces == 2

    def test_ctor_from_cuts(self):
        mesh = gmm.ThermalMesh([0.0, 0.5, 1.0], [0.0, 0.5, 1.0])
        assert mesh.num_pair_faces == 4

    def test_per_side_properties(self):
        mesh = gmm.ThermalMesh()
        mesh.side1_activity = False
        mesh.side2_thick = 1.0e-3
        mesh.side1_color = gmm.Color(255, 0, 0)
        assert mesh.side1_activity is False
        assert mesh.side2_thick == pytest.approx(1.0e-3)
        assert mesh.side1_color.red == 255

    def test_materials_nullable_and_shared(self):
        mesh = gmm.ThermalMesh()
        assert mesh.side1_material is None
        alu = gmm.BulkMaterial("alu", 2700.0, 167.0, 896.0)
        mesh.side1_material = alu
        mesh.side2_material = alu
        assert mesh.side1_material is not None
        assert mesh.side1_material.name == "alu"

    def test_node_assignment(self):
        mesh = gmm.ThermalMesh([0.0, 0.5, 1.0], [0.0, 1.0])
        # Defaults: unassigned.
        assert mesh.node1_start == -1
        mesh.node1_start = 100
        mesh.node1_step = 1
        assert mesh.node_of(0, 0, 1) == 100
        assert mesh.node_of(1, 0, 1) == 101
