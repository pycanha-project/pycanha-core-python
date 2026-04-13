"""Test bindings for Formula, ParameterFormula, ValueFormula, Formulas."""

import pycanha_core as pcc
import pytest

Entity = pcc.parameters.Entity
ExpressionFormula = pcc.parameters.ExpressionFormula
Formulas = pcc.parameters.Formulas
Node = pcc.tmm.Node
ParameterFormula = pcc.parameters.ParameterFormula
Parameters = pcc.parameters.Parameters
ThermalMathematicalModel = pcc.tmm.ThermalMathematicalModel
ThermalNetwork = pcc.tmm.ThermalNetwork
ValueFormula = pcc.parameters.ValueFormula


@pytest.fixture
def tmm_with_params():
    """TMM with nodes, couplings and a parameter 'k'."""
    tmm = ThermalMathematicalModel("formula_test")
    n1 = Node(1)
    n1.T = 300.0
    n1.qi = 50.0
    n2 = Node(2)
    n2.T = 200.0
    tmm.add_node(n1)
    tmm.add_node(n2)
    tmm.add_conductive_coupling(1, 2, 10.0)
    tmm.parameters.add_parameter("k", 25.0)
    return tmm


# ---------------------------------------------------------------------------
# ParameterFormula
# ---------------------------------------------------------------------------
class TestParameterFormula:
    def test_construction(self, tmm_with_params):
        tmm = tmm_with_params
        entity = Entity.gl(tmm.network, 1, 2)
        formula = ParameterFormula(entity, tmm.parameters, "k")
        assert formula is not None

    def test_compile_and_apply(self, tmm_with_params):
        tmm = tmm_with_params
        entity = Entity.gl(tmm.network, 1, 2)
        formula = ParameterFormula(entity, tmm.parameters, "k")
        formula.compile_formula()
        formula.apply_formula()
        # coupling should now have the value of parameter "k" = 25.0
        assert tmm.conductive_couplings.get_coupling_value(1, 2) == pytest.approx(25.0)

    def test_apply_compiled_formula(self, tmm_with_params):
        tmm = tmm_with_params
        entity = Entity.gl(tmm.network, 1, 2)
        formula = ParameterFormula(entity, tmm.parameters, "k")
        formula.compile_formula()
        formula.apply_compiled_formula()
        assert tmm.conductive_couplings.get_coupling_value(1, 2) == pytest.approx(25.0)

    def test_get_value(self, tmm_with_params):
        tmm = tmm_with_params
        entity = Entity.gl(tmm.network, 1, 2)
        formula = ParameterFormula(entity, tmm.parameters, "k")
        formula.compile_formula()
        formula.apply_formula()
        assert formula.get_value() == pytest.approx(25.0)

    def test_entity_property(self, tmm_with_params):
        tmm = tmm_with_params
        entity = Entity.gl(tmm.network, 1, 2)
        formula = ParameterFormula(entity, tmm.parameters, "k")
        assert formula.entity is not None

    def test_parameter_dependencies(self, tmm_with_params):
        tmm = tmm_with_params
        entity = Entity.gl(tmm.network, 1, 2)
        formula = ParameterFormula(entity, tmm.parameters, "k")
        deps = formula.parameter_dependencies
        assert isinstance(deps, list)

    def test_parameter_change_propagates(self, tmm_with_params):
        tmm = tmm_with_params
        entity = Entity.gl(tmm.network, 1, 2)
        formula = ParameterFormula(entity, tmm.parameters, "k")
        formula.compile_formula()
        formula.apply_formula()
        assert tmm.conductive_couplings.get_coupling_value(1, 2) == pytest.approx(25.0)

        tmm.parameters.set_parameter("k", 50.0)
        formula.apply_formula()
        assert tmm.conductive_couplings.get_coupling_value(1, 2) == pytest.approx(50.0)


# ---------------------------------------------------------------------------
# ValueFormula
# ---------------------------------------------------------------------------
class TestValueFormula:
    def test_construction(self, tmm_with_params):
        tmm = tmm_with_params
        entity = Entity.qi(tmm.network, 1)
        formula = ValueFormula(entity)
        assert formula is not None

    def test_set_and_apply(self, tmm_with_params):
        tmm = tmm_with_params
        entity = Entity.qi(tmm.network, 1)
        formula = ValueFormula(entity)
        formula.set_value(999.0)
        formula.compile_formula()
        formula.apply_formula()
        assert tmm.nodes.get_qi(1) == pytest.approx(999.0)

    def test_get_value(self, tmm_with_params):
        tmm = tmm_with_params
        entity = Entity.qi(tmm.network, 1)
        formula = ValueFormula(entity)
        formula.set_value(123.0)
        formula.compile_formula()
        formula.apply_formula()
        assert formula.get_value() == pytest.approx(123.0)


# ---------------------------------------------------------------------------
# ExpressionFormula
# ---------------------------------------------------------------------------
class TestExpressionFormula:
    def test_apply_formula(self, tmm_with_params):
        tmm = tmm_with_params
        tmm.parameters.add_parameter("offset", 2.0)
        entity = Entity.qi(tmm.network, 1)
        formula = ExpressionFormula(entity, tmm.parameters, "k + offset")

        assert formula.expression == "k + offset"
        assert formula.parameter_dependencies == ["k", "offset"]

        formula.apply_formula()

        assert tmm.nodes.get_qi(1) == pytest.approx(27.0)
        assert formula.get_value() == pytest.approx(27.0)

    def test_compile_and_calculate_derivatives(self, tmm_with_params):
        tmm = tmm_with_params
        tmm.parameters.add_parameter("offset", 2.0)
        entity = Entity.gl(tmm.network, 1, 2)
        formula = ExpressionFormula(entity, tmm.parameters, "k * offset")

        formula.compile_formula()
        formula.calculate_derivatives()

        assert formula.get_derivative_values() == pytest.approx([2.0, 25.0])


# ---------------------------------------------------------------------------
# Formulas collection
# ---------------------------------------------------------------------------
class TestFormulas:
    def test_construction(self):
        formulas = Formulas()
        assert formulas is not None

    def test_associate_and_add(self, tmm_with_params):
        tmm = tmm_with_params
        formulas = tmm.formulas
        entity = Entity.gl(tmm.network, 1, 2)
        formula = ParameterFormula(entity, tmm.parameters, "k")
        formulas.add_formula(formula)

        assert len(formulas.formulas) == 1

    def test_apply_formulas(self, tmm_with_params):
        tmm = tmm_with_params
        entity = Entity.gl(tmm.network, 1, 2)
        formula = ParameterFormula(entity, tmm.parameters, "k")
        tmm.formulas.add_formula(formula)
        tmm.formulas.apply_formulas()
        assert tmm.conductive_couplings.get_coupling_value(1, 2) == pytest.approx(25.0)

    def test_parametric_study(self, tmm_with_params):
        """Simulate a parametric study: change parameter, apply, check."""
        tmm = tmm_with_params
        entity = Entity.gl(tmm.network, 1, 2)
        formula = ParameterFormula(entity, tmm.parameters, "k")
        tmm.formulas.add_formula(formula)

        for value in [5.0, 10.0, 20.0]:
            tmm.parameters.set_parameter("k", value)
            tmm.formulas.apply_formulas()
            assert tmm.conductive_couplings.get_coupling_value(1, 2) == pytest.approx(
                value
            )

    def test_parameter_dependencies_property(self, tmm_with_params):
        tmm = tmm_with_params
        entity = Entity.gl(tmm.network, 1, 2)
        formula = ParameterFormula(entity, tmm.parameters, "k")
        tmm.formulas.add_formula(formula)
        deps = tmm.formulas.parameter_dependencies
        assert isinstance(deps, dict)

    def test_formulas_list_property(self, tmm_with_params):
        tmm = tmm_with_params
        entity = Entity.gl(tmm.network, 1, 2)
        formula = ParameterFormula(entity, tmm.parameters, "k")
        tmm.formulas.add_formula(formula)
        flist = tmm.formulas.formulas
        assert isinstance(flist, list)
        assert len(flist) == 1

    def test_calculate_derivatives(self, tmm_with_params):
        tmm = tmm_with_params
        tmm.parameters.add_parameter("offset", 2.0)
        entity = Entity.qi(tmm.network, 1)
        formula = ExpressionFormula(entity, tmm.parameters, "k + offset")

        tmm.formulas.add_formula(formula)
        tmm.formulas.apply_formulas()
        tmm.formulas.calculate_derivatives()

        stored_formula = tmm.formulas.formulas[0]
        assert stored_formula.get_derivative_values() == pytest.approx([1.0, 1.0])

        deps = tmm.formulas.parameter_dependencies
        assert set(deps) == {"k", "offset"}
        assert len(deps["k"]) == 1
        assert len(deps["offset"]) == 1
