"""Tests for the 0.15 CoordinateTransformation value class."""

import numpy as np
import pytest

import pycanha_core as pcc

gmm = pcc.gmm
CoordinateTransformation = gmm.CoordinateTransformation


class TestCoordinateTransformation:
    def test_default_is_identity(self):
        ct = CoordinateTransformation()
        assert ct.is_identity() is True
        np.testing.assert_allclose(ct.translation, [0.0, 0.0, 0.0], atol=1e-12)
        np.testing.assert_allclose(ct.rotation, np.eye(3), atol=1e-12)

    def test_from_translation(self):
        ct = CoordinateTransformation.from_translation(np.array([1.0, 2.0, 3.0]))
        np.testing.assert_allclose(ct.apply(np.array([0.0, 0.0, 0.0])), [1.0, 2.0, 3.0], atol=1e-12)

    def test_from_euler_rotation(self):
        ct = CoordinateTransformation.from_euler(
            np.array([0.0, 0.0, 0.0]), np.array([0.0, 0.0, np.pi / 2.0])
        )
        result = ct.apply(np.array([1.0, 0.0, 0.0]))
        np.testing.assert_allclose(result, [0.0, 1.0, 0.0], atol=1e-9)

    def test_from_rotation_quaternion(self):
        # Identity quaternion (w, x, y, z).
        ct = CoordinateTransformation.from_rotation(np.array([1.0, 0.0, 0.0, 0.0]))
        assert ct.is_identity() is True

    def test_translation_roundtrip(self):
        ct = CoordinateTransformation()
        t = np.array([10.0, 20.0, 30.0])
        ct.translation = t
        np.testing.assert_allclose(ct.translation, t, atol=1e-12)

    def test_apply_normal_ignores_translation(self):
        ct = CoordinateTransformation.from_translation(np.array([5.0, 0.0, 0.0]))
        n = ct.apply_normal(np.array([0.0, 0.0, 1.0]))
        np.testing.assert_allclose(n, [0.0, 0.0, 1.0], atol=1e-12)

    def test_compose_then_inverse(self):
        a = CoordinateTransformation.from_translation(np.array([1.0, 0.0, 0.0]))
        b = CoordinateTransformation.from_euler(
            np.array([0.0, 0.0, 0.0]), np.array([0.0, 0.0, np.pi / 2.0])
        )
        composed = a.compose(b)
        inv = composed.inverse()
        point = np.array([1.0, 2.0, 3.0])
        back = inv.apply(composed.apply(point))
        np.testing.assert_allclose(back, point, atol=1e-9)

    def test_construct_from_translation_and_matrix(self):
        rot = np.eye(3)
        ct = CoordinateTransformation(np.array([1.0, 1.0, 1.0]), rot)
        np.testing.assert_allclose(ct.linear(), rot, atol=1e-12)
