"""Report, memory estimate, auto_configure, explain and origin."""

import pycanha_core as pcc
import pytest

from .models import make_chain_tmm

solvers = pcc.solvers


def steady(model, **settings):
    solver = solvers.steady.SteadyState(model)
    solver.set_settings(solvers.Settings(**{"tolerance": 1e-8, "max_iterations": 100, **settings}))
    return solver


def transient(model, **settings):
    solver = solvers.transient.CrankNicolson(model)
    solver.set_settings(solvers.Settings(**{"tolerance": 1e-8, "max_iterations": 100, **settings}))
    solver.set_simulation_time(0.0, 1000.0, 100.0, 500.0)
    return solver


class TestReport:
    def test_a_new_solver_has_not_run(self, basic_tmm):
        report = solvers.steady.SteadyState(basic_tmm).report()
        assert report.status == solvers.SolveStatus.NOT_RUN
        assert report.converged is False

    def test_steady_run(self, basic_tmm):
        solver = steady(basic_tmm)
        solver.initialize()
        solver.solve()
        report = solver.report()
        assert report.solver == "SteadyState"
        assert report.status == solvers.SolveStatus.CONVERGED
        assert report.converged
        assert report.steps == 0
        assert report.iterations == solver.iterations > 1
        assert report.factorizations == solver.num_factorizations >= 1
        assert report.solves >= report.factorizations
        assert 0.0 <= report.reuse_rate <= 1.0
        assert report.solve_time >= 0.0
        assert report.initialize_time >= 0.0
        assert report.nonconverged == []
        assert report.memory.total > 0
        assert "SteadyState" in str(report)

    def test_transient_run(self, basic_tmm):
        solver = transient(basic_tmm)
        solver.initialize()
        solver.solve()
        report = solver.report()
        assert report.solver == "CrankNicolson"
        assert report.steps == 10
        assert report.max_step_iterations >= 1
        assert report.memory.output > 0
        dictionary = report.to_dict()
        assert dictionary["steps"] == 10
        assert dictionary["status"] == "CONVERGED"
        assert "memory.factors" in dictionary
        assert dictionary["nonconverged.step"] == []

    def test_non_converged_steady_state(self, basic_tmm):
        solver = steady(basic_tmm, max_iterations=1)
        solver.initialize()
        solver.solve()
        report = solver.report()
        assert report.status == solvers.SolveStatus.NOT_CONVERGED
        assert solver.converged is False
        assert report.nonconverged_steps == 1
        (step,) = report.nonconverged
        assert step.step == 0
        assert step.iterations == 1
        assert step.max_dT > 0.0
        assert step.node in (10, 15, 20, 25)

    def test_non_converged_transient_steps_are_listed(self, basic_tmm):
        solver = transient(basic_tmm, max_iterations=1)
        solver.initialize()
        solver.solve()
        report = solver.report()
        assert report.status == solvers.SolveStatus.NOT_CONVERGED
        assert 0 < report.nonconverged_steps <= report.steps
        assert len(report.nonconverged) == min(report.nonconverged_steps, report.max_listed_steps)
        assert report.nonconverged[0].step >= 1
        assert report.nonconverged[0].time > 0.0
        assert report.to_dict()["nonconverged.step"]
        assert solvers.SolverReport.max_listed_steps == 100

    def test_report_is_a_copy(self, basic_tmm):
        solver = steady(basic_tmm)
        solver.initialize()
        solver.solve()
        report = solver.report()
        solver.initialize()
        assert solver.report().status == solvers.SolveStatus.NOT_RUN
        assert report.status == solvers.SolveStatus.CONVERGED


class TestMemory:
    def test_estimate_before_initialize(self, basic_tmm):
        memory = steady(basic_tmm).estimate_memory()
        assert isinstance(memory, solvers.SolverMemory)
        assert memory.total == memory.factors + memory.matrix + memory.output
        assert memory.total > 0
        assert isinstance(memory.exact, bool)
        assert "B" in str(memory)

    def test_estimate_of_a_transient_counts_the_output(self, basic_tmm):
        assert transient(basic_tmm).estimate_memory().output > 0

    def test_sizes(self):
        assert solvers.format_bytes(1_210_000_000) == "1.21 GB"
        assert solvers.format_bytes(17) == "17 B"
        assert solvers.parse_memory_size("16 GB") == 16_000_000_000
        assert solvers.parse_memory_size("1 KiB") == 1024
        assert solvers.parse_memory_size("2000000") == 2_000_000
        for bad in ("lots", "-1 GB", "0"):
            with pytest.raises(ValueError):
                solvers.parse_memory_size(bad)


class TestAutoConfigure:
    @pytest.mark.parametrize("max_memory", [None, 16_000_000_000, "16 GB"])
    def test_returns_and_applies_the_settings(self, max_memory, basic_tmm):
        solver = steady(basic_tmm)
        chosen = (
            solver.auto_configure() if max_memory is None else solver.auto_configure(max_memory)
        )
        assert isinstance(chosen, solvers.Settings)
        assert chosen.backend is not None
        assert solver.settings() == chosen
        assert solver.origin("backend") == solvers.Origin.AUTO_CONFIGURE
        assert solver.origin("tolerance") == solvers.Origin.USER
        solver.initialize()
        solver.solve()
        assert solver.converged

    def test_a_bad_size_raises_value_error(self, basic_tmm):
        with pytest.raises(ValueError):
            steady(basic_tmm).auto_configure("a lot")

    def test_refused_on_an_initialized_solver(self, basic_tmm):
        solver = steady(basic_tmm)
        solver.initialize()
        with pytest.raises(RuntimeError):
            solver.auto_configure()

    def test_a_tiny_budget_still_gives_a_choice(self, basic_tmm):
        solver = steady(basic_tmm)
        assert solver.auto_configure(1).backend is not None


class TestExplain:
    def test_explain_lists_the_applicable_settings(self, basic_tmm):
        solver = steady(basic_tmm, time_step=1.0)
        text = solver.explain()
        assert "tolerance" in text
        assert "time_step" in text  # set, but it does not apply

    def test_origin(self):
        solver = steady(make_chain_tmm())
        assert solver.origin("tolerance") == solvers.Origin.USER
        assert solver.origin("threads") == solvers.Origin.DEFAULT
        assert solver.origin("time_step") == solvers.Origin.DEFAULT
        with pytest.raises(ValueError):
            solver.origin("no_such_setting")

    def test_resolved_settings_match_initialize(self):
        solver = steady(make_chain_tmm(), backend=solvers.Backend.EIGEN)
        resolved = solver.resolved_settings()
        assert resolved.backend == solvers.Backend.EIGEN
        assert resolved.tolerance == 1e-8
        assert resolved.solver_type == solvers.SolverType.STEADY_STATE
