"""Test that Python callbacks stored in C++ no longer create uncollectable cycles.

ThermalMathematicalModel and CallbackRegistry hold Python callables inside C++
std::function members. Their bindings implement tp_traverse/tp_clear so that
reference cycles through those members (callback -> ... -> model -> callback)
are visible to Python's cyclic garbage collector and get collected instead of
leaking until interpreter shutdown.
"""

import gc
import weakref

import pycanha_core as pcc


class TestTmmCallbackCycles:
    def test_python_apply_formulas_cycle_is_collected(self):
        tmm = pcc.tmm.ThermalMathematicalModel("model")

        class Holder:
            def __init__(self, model):
                self.model = model

            def apply(self):
                pass

        holder = Holder(tmm)
        tmm.python_apply_formulas = holder.apply
        probe = weakref.ref(holder)

        del tmm, holder
        gc.collect()

        assert probe() is None

    def test_extern_callback_cycles_are_collected(self):
        tmm = pcc.tmm.ThermalMathematicalModel("model")

        class Holder:
            def __init__(self, model):
                self.model = model

            def cb(self):
                pass

        holder = Holder(tmm)
        tmm.python_extern_callback_solver_loop = holder.cb
        tmm.python_extern_callback_transient_time_change = holder.cb
        tmm.python_extern_callback_transient_after_timestep = holder.cb
        probe = weakref.ref(holder)

        del tmm, holder
        gc.collect()

        assert probe() is None

    def test_assigned_callback_still_fires(self):
        tmm = pcc.tmm.ThermalMathematicalModel("model")
        hits = []
        tmm.python_apply_formulas = lambda: hits.append(1)

        tmm.python_apply_formulas()

        assert hits == [1]


class TestCallbackRegistryCycles:
    def test_registry_callback_cycle_is_collected(self):
        tm = pcc.tmm.ThermalModel("model")
        registry = tm.callbacks

        class Holder:
            def __init__(self, tm, registry):
                self.tm = tm
                self.registry = registry

            def cb(self, context):
                pass

        holder = Holder(tm, registry)
        registry.solver_loop = holder.cb
        registry.time_change = holder.cb
        registry.after_timestep = holder.cb
        probe = weakref.ref(holder)

        del tm, registry, holder
        gc.collect()

        assert probe() is None

    def test_registry_callback_still_fires(self):
        tm = pcc.tmm.ThermalModel("model")
        seen = []
        tm.callbacks.solver_loop = lambda context: seen.append(context.tm.name)

        tm.callbacks.invoke_solver_loop()

        assert seen == ["model"]
