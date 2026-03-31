"""Test bindings for CoordinateTransformation."""

import math

import numpy as np
import pytest

from pycanha_core.gmm import CoordinateTransformation, TransformOrder


class TestCoordinateTransformation:
    def test_default_construction(self):
        ct = CoordinateTransformation()
        assert isinstance(ct, CoordinateTransformation)

    def test_construction_with_params(self):
        t = np.array([1.0, 2.0, 3.0])
        r = np.array([0.0, 0.0, math.pi / 2])
        ct = CoordinateTransformation(translation=t, rotation=r)
        np.testing.assert_allclose(ct.translation, t, atol=1e-10)

    def test_translation_roundtrip(self):
        ct = CoordinateTransformation()
        t = np.array([10.0, 20.0, 30.0])
        ct.translation = t
        np.testing.assert_allclose(ct.translation, t, atol=1e-10)

    def test_order_roundtrip(self):
        ct = CoordinateTransformation()
        ct.order = TransformOrder.ROTATION_THEN_TRANSLATION
        assert ct.order == TransformOrder.ROTATION_THEN_TRANSLATION
        ct.order = TransformOrder.TRANSLATION_THEN_ROTATION
        assert ct.order == TransformOrder.TRANSLATION_THEN_ROTATION

    def test_transform_order_enum_values(self):
        assert TransformOrder.TRANSLATION_THEN_ROTATION is not None
        assert TransformOrder.ROTATION_THEN_TRANSLATION is not None

    def test_pure_translation(self):
        ct = CoordinateTransformation(
            translation=np.array([1.0, 2.0, 3.0]),
            rotation=np.array([0.0, 0.0, 0.0]),
        )
        result = ct.transform_point(np.array([0.0, 0.0, 0.0]))
        np.testing.assert_allclose(result, [1.0, 2.0, 3.0], atol=1e-10)

    def test_identity_transform(self):
        ct = CoordinateTransformation()
        point = np.array([5.0, 6.0, 7.0])
        result = ct.transform_point(point)
        np.testing.assert_allclose(result, point, atol=1e-10)
