"""Settings, the field table and resolution."""

import copy

import pycanha_core as pcc
import pytest

from .models import make_chain_tmm

solvers = pcc.solvers
Attribute = pcc.DataModelAttribute


class TestConstruction:
    def test_every_field_starts_unset(self):
        settings = solvers.Settings()
        assert settings.to_dict() == {}
        assert settings.solver_type is None
        assert settings.tolerance is None
        assert settings.output_attributes is None
        assert settings.pardiso == solvers.PardisoSettings()
        assert settings.pardiso.level is None
        assert settings.eigen.ordering is None
        assert settings.accelerate.ordering is None

    def test_keyword_constructor(self):
        settings = solvers.Settings(
            time_step=10.0,
            backend=solvers.Backend.EIGEN,
            max_iterations=20,
            reuse_factors=False,
            output_attributes={Attribute.T, Attribute.QI},
            output_name="run",
            pardiso=solvers.PardisoSettings(level=solvers.PardisoLevel.ONE_LEVEL, iparm={3: 61}),
            eigen=solvers.EigenSettings(ordering=solvers.EigenOrdering.AMD),
        )
        assert settings.time_step == 10.0
        assert settings.backend == solvers.Backend.EIGEN
        assert settings.max_iterations == 20
        assert settings.reuse_factors is False
        assert settings.output_attributes == {Attribute.T, Attribute.QI}
        assert settings.output_name == "run"
        assert settings.pardiso.level == solvers.PardisoLevel.ONE_LEVEL
        assert settings.pardiso.iparm == {3: 61}
        assert settings.eigen.ordering == solvers.EigenOrdering.AMD
        assert settings.accelerate.ordering is None

    def test_constructor_takes_keywords_only(self):
        with pytest.raises(TypeError):
            solvers.Settings(1e-3)

    def test_an_integer_is_accepted_for_a_float_field(self):
        assert solvers.Settings(time_step=10).time_step == 10.0

    def test_copies_are_independent(self):
        settings = solvers.Settings(tolerance=1e-3)
        for duplicate in (solvers.Settings(settings), copy.copy(settings), copy.deepcopy(settings)):
            assert duplicate == settings
            assert duplicate is not settings
            duplicate.tolerance = 1.0
            assert settings.tolerance == 1e-3

    def test_equality(self):
        assert solvers.Settings(tolerance=1.0) == solvers.Settings(tolerance=1.0)
        assert solvers.Settings(tolerance=1.0) != solvers.Settings(tolerance=2.0)
        assert solvers.Settings() != "not settings"
        assert solvers.PardisoSettings(verbose=True) == solvers.PardisoSettings(verbose=True)


class TestFields:
    def test_none_unsets(self):
        settings = solvers.Settings(tolerance=1e-3)
        settings.tolerance = None
        assert not settings.is_set("tolerance")

    def test_groups_are_edited_in_place(self):
        settings = solvers.Settings()
        settings.pardiso.level = solvers.PardisoLevel.ONE_LEVEL
        settings.eigen.ordering = solvers.EigenOrdering.AMD
        settings.accelerate.ordering = solvers.AccelerateOrdering.AMD
        assert settings.is_set("pardiso.level")
        assert settings.get("pardiso.level") == "ONE_LEVEL"
        assert settings.get("eigen.ordering") == "AMD"
        assert settings.get("accelerate.ordering") == "AMD"

    def test_a_group_can_be_replaced(self):
        settings = solvers.Settings()
        settings.pardiso = solvers.PardisoSettings(verbose=True)
        assert settings.pardiso.verbose is True

    def test_reading_a_collection_returns_a_copy(self):
        settings = solvers.Settings(output_attributes={Attribute.T}, pardiso=solvers.PardisoSettings(iparm={3: 61}))
        settings.output_attributes.add(Attribute.QI)
        settings.pardiso.iparm[4] = 1
        assert settings.output_attributes == {Attribute.T}
        assert settings.pardiso.iparm == {3: 61}

    def test_wrong_type_on_assignment_raises_type_error(self):
        settings = solvers.Settings()
        with pytest.raises(TypeError):
            settings.reuse_factors = 1
        with pytest.raises(TypeError):
            settings.backend = "EIGEN"


class TestByName:
    def test_set_and_get(self):
        settings = solvers.Settings()
        settings.set("tolerance", 1e-4)
        settings.set("backend", "EIGEN")
        settings.set("pardiso.iparm", {3: 61})
        assert settings.tolerance == 1e-4
        assert settings.backend == solvers.Backend.EIGEN
        assert settings.get("backend") == "EIGEN"
        assert settings.get("pardiso.iparm") == {3: 61}
        assert settings.get("time_step") is None
        settings.unset("tolerance")
        assert not settings.is_set("tolerance")

    def test_enum_members_are_accepted(self):
        settings = solvers.Settings()
        settings.set("backend", solvers.Backend.EIGEN)
        settings.set("output_attributes", {Attribute.T, Attribute.QI})
        settings.set("solver_type", solvers.SolverType.CRANK_NICOLSON)
        assert settings.backend == solvers.Backend.EIGEN
        assert settings.output_attributes == {Attribute.T, Attribute.QI}
        assert settings.solver_type == solvers.SolverType.CRANK_NICOLSON
        assert sorted(settings.get("output_attributes")) == ["QI", "T"]

    def test_output_attributes_by_name(self):
        settings = solvers.Settings()
        settings.set("output_attributes", ["T", "QI"])
        assert settings.output_attributes == {Attribute.T, Attribute.QI}

    def test_bool_is_not_taken_as_an_integer(self):
        settings = solvers.Settings()
        settings.set("reuse_factors", True)
        settings.set("max_iterations", 7)
        assert settings.get("reuse_factors") is True
        value = settings.get("max_iterations")
        assert value == 7
        assert type(value) is int
        dictionary = settings.to_dict()
        assert type(dictionary["reuse_factors"]) is bool
        assert type(dictionary["max_iterations"]) is int
        with pytest.raises(ValueError):
            settings.set("reuse_factors", 1)
        with pytest.raises(ValueError):
            settings.set("max_iterations", True)

    def test_an_integer_is_accepted_for_a_float_setting(self):
        settings = solvers.Settings()
        settings.set("time_step", 10)
        assert settings.time_step == 10.0

    @pytest.mark.parametrize(
        ("name", "value"),
        [
            ("no_such_setting", 1.0),
            ("tolerance", "small"),
            ("tolerance", -1.0),
            ("backend", "NO_SUCH_BACKEND"),
            ("output_attributes", ["NO_SUCH_ATTRIBUTE"]),
        ],
    )
    def test_bad_values_raise_value_error_and_change_nothing(self, name, value):
        settings = solvers.Settings(tolerance=1e-3)
        with pytest.raises(ValueError):
            settings.set(name, value)
        assert settings == solvers.Settings(tolerance=1e-3)

    @pytest.mark.parametrize("method", ["is_set", "get", "unset"])
    def test_unknown_names_raise_value_error(self, method):
        with pytest.raises(ValueError):
            getattr(solvers.Settings(), method)("no_such_setting")


class TestDict:
    def test_round_trip(self):
        settings = solvers.Settings(
            solver_type=solvers.SolverType.CRANK_NICOLSON,
            time_step=1.0,
            backend=solvers.Backend.EIGEN,
            output_attributes={Attribute.T},
            pardiso=solvers.PardisoSettings(iparm={3: 61}, verbose=False),
        )
        dictionary = settings.to_dict()
        assert dictionary["backend"] == "EIGEN"
        assert dictionary["solver_type"] == "CRANK_NICOLSON"
        assert dictionary["pardiso.iparm"] == {3: 61}
        assert dictionary["output_attributes"] == ["T"]
        assert solvers.Settings.from_dict(dictionary) == settings

    def test_from_dict_accepts_enum_members(self):
        settings = solvers.Settings.from_dict({"backend": solvers.Backend.EIGEN})
        assert settings.backend == solvers.Backend.EIGEN

    def test_from_dict_refuses_unknown_keys(self):
        with pytest.raises(ValueError):
            solvers.Settings.from_dict({"no_such_setting": 1})


class TestPrinting:
    def test_loose_settings_list_the_set_fields(self):
        text = str(solvers.Settings(time_step=10.0))
        assert "time_step" in text
        assert "tolerance" not in text

    def test_with_a_solver_type_every_applicable_field_is_listed(self):
        text = str(solvers.Settings(solver_type=solvers.SolverType.STEADY_STATE, time_step=10.0))
        assert "tolerance" in text
        assert "time_step" in text  # listed as set but not applied

    def test_enums_print_their_name(self):
        assert str(solvers.Backend.PARDISO) == "PARDISO"
        assert str(solvers.SolverType.CRANK_NICOLSON) == "CRANK_NICOLSON"
        assert str(solvers.Origin.AUTO_CONFIGURE) == "AUTO_CONFIGURE"
        assert str(solvers.SolveStatus.NOT_RUN) == "NOT_RUN"


class TestFieldTable:
    def test_fields_document_themselves(self):
        fields = solvers.setting_fields()
        names = [field.name for field in fields]
        assert "tolerance" in names
        assert "pardiso.level" in names
        assert len(names) == len(set(names))
        for field in fields:
            assert field.doc
            assert field.group in ("", "pardiso", "eigen", "accelerate")
            assert field.mutability in (solvers.Mutability.LIVE, solvers.Mutability.INIT)

    def test_lookup_by_name(self):
        field = solvers.setting_field("time_step")
        assert field.name == "time_step"
        assert field.applies_to(solvers.SolverType.CRANK_NICOLSON)
        assert not field.applies_to(solvers.SolverType.STEADY_STATE)
        assert field.solver_types & (1 << solvers.SolverType.CRANK_NICOLSON.value)
        with pytest.raises(ValueError):
            solvers.setting_field("no_such_setting")

    def test_output_attribute_sets(self):
        dense = solvers.all_dense_output_attributes()
        everything = solvers.all_output_attributes()
        assert Attribute.T in dense
        assert dense < everything
        assert Attribute.JAC in everything

    def test_accelerate_availability_is_a_bool(self):
        assert isinstance(solvers.accelerate_available(), bool)
        if not pcc.ACCELERATE_ENABLED:
            assert solvers.accelerate_available() is False


class TestResolution:
    def test_defaults_and_origins(self):
        context = solvers.ResolveContext()
        resolved = solvers.resolve(solvers.Settings(tolerance=1e-4, time_step=1.0), context)
        assert resolved.solver_type == solvers.SolverType.STEADY_STATE
        assert resolved.tolerance == 1e-4
        assert resolved.origin("tolerance") == solvers.Origin.USER
        assert resolved.origin("max_iterations") == solvers.Origin.DEFAULT
        assert resolved.applies("tolerance")
        assert not resolved.applies("time_step")
        assert resolved.value("tolerance") == 1e-4
        assert [entry.name for entry in resolved.not_applied] == ["time_step"]
        assert resolved.not_applied[0].reason
        assert len(resolved.fields) == len(solvers.setting_fields())

    def test_context_follows_the_model(self, basic_tmm):
        conduction = solvers.resolve_context(solvers.SolverType.CRANK_NICOLSON, make_chain_tmm())
        radiative = solvers.resolve_context(solvers.SolverType.STEADY_STATE, basic_tmm)
        assert conduction.solver_type == solvers.SolverType.CRANK_NICOLSON
        assert conduction.has_radiation is False
        assert radiative.has_radiation is True
        assert conduction.mkl_available is pcc.MKL_ENABLED

    def test_context_is_editable(self):
        context = solvers.ResolveContext()
        context.solver_type = solvers.SolverType.CRANK_NICOLSON
        context.has_radiation = True
        context.mkl_available = False
        context.accelerate_available = False
        resolved = solvers.resolve(solvers.Settings(time_step=1.0), context)
        assert resolved.backend == solvers.Backend.EIGEN
        with pytest.raises(ValueError):
            solvers.resolve(solvers.Settings(time_step=1.0, backend=solvers.Backend.PARDISO), context)

    def test_contradictions_raise_or_are_listed(self):
        settings = solvers.Settings(backend=solvers.Backend.EIGEN, reuse_factors=True)
        context = solvers.ResolveContext()
        with pytest.raises(ValueError, match="reuse_factors"):
            solvers.resolve(settings, context)
        explanation = solvers.explain_settings(settings, context)
        assert explanation.contradictions
        assert explanation.resolved.backend == solvers.Backend.EIGEN
        names = [field.name for field in explanation.fields]
        assert "tolerance" in names
        tolerance = explanation.fields[names.index("tolerance")]
        assert tolerance.origin == solvers.Origin.DEFAULT
        assert tolerance.value_text

    def test_cholesky_with_radiation_is_a_contradiction(self):
        context = solvers.ResolveContext()
        context.has_radiation = True
        with pytest.raises(ValueError):
            solvers.resolve(solvers.Settings(factorization=solvers.Factorization.CHOLESKY), context)

    def test_init_field_changes(self):
        context = solvers.ResolveContext()
        before = solvers.Settings()
        after = solvers.Settings(backend=solvers.Backend.EIGEN, storage=solvers.Storage.DENSE)
        changed = solvers.changed_init_fields(before, after, context)
        assert "storage" in changed
        solvers.check_settings_change(before, after, context, False)
        with pytest.raises(RuntimeError, match="storage"):
            solvers.check_settings_change(before, after, context, True)
