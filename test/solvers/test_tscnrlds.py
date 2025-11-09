import pycanha_core as pcc
import numpy as np

# Create a basic model to be solved
tmm = pcc.tmm.ThermalMathematicalModel("test_model")

# Create the nodes
node_10 = pcc.tmm.Node(10)
node_15 = pcc.tmm.Node(15)
node_20 = pcc.tmm.Node(20)
node_25 = pcc.tmm.Node(25)
env_node = pcc.tmm.Node(99)

# Set initial temperatures (in Kelvin)
init_temp = 273.15

node_10.T = init_temp
node_15.T = init_temp
node_20.T = init_temp
node_25.T = init_temp
env_node.T = 3.15


# Set thermal capacities (in J/K)
node_10.capacity = 2e5
node_15.capacity = 2e5
node_20.capacity = 2e5
node_25.capacity = 2e5

# Set dissipation
node_15.qi = 500.0

# Set env node as boundary
env_node.type = pcc.NodeType.BOUNDARY

# Add nodes to the model
tmm.add_node(node_10)
tmm.add_node(node_15)
tmm.add_node(node_20)
tmm.add_node(node_25)
tmm.add_node(env_node)

# Create couplings between nodes (in W/K)
tmm.add_conductive_coupling(10, 15, 0.1)
tmm.add_conductive_coupling(20, 25, 0.1)

# Create radiative couplings
tmm.add_radiative_coupling(10, 99, 1.0)
tmm.add_radiative_coupling(20, 99, 1.0)
tmm.add_radiative_coupling(15, 25, 0.2)
tmm.add_radiative_coupling(15, 99, 0.8)
tmm.add_radiative_coupling(25, 99, 0.8)

tscnrlds_solver = pcc.solvers.TSCNRLDS(tmm)

tscnrlds_solver.MAX_ITERS = 100
tscnrlds_solver.abstol_temp = 1e-6
tscnrlds_solver.set_simulation_time(0.0, 100000.0, 1000.0, 10000.0)

tscnrlds_solver.initialize()
tscnrlds_solver.solve()
solver_results = tmm.thermal_data.get_table("TSCNRLDS_OUTPUT")
calculated_times = solver_results[:, 0]
calculated_temps = solver_results[:, 1:]

expected_times = np.array(
    [
        0.0,
        10000.0,
        20000.0,
        30000.0,
        40000.0,
        50000.0,
        60000.0,
        70000.0,
        80000.0,
        90000.0,
        100000.0,
    ]
)

expected_temps = np.array(
    [
        [273.14999, 273.14999, 273.14999, 273.14999, 3.14999],
        [259.03552, 283.85105, 258.98241, 262.06791, 3.14999],
        [247.56014, 291.67014, 247.37629, 253.45623, 3.14999],
        [237.98527, 297.25685, 237.62266, 246.62735, 3.14999],
        [229.83503, 301.16946, 229.26392, 241.11244, 3.14999],
        [222.78667, 303.85891, 221.98896, 236.58283, 3.14999],
        [216.61234, 305.67267, 215.57742, 232.80415, 3.14999],
        [211.14591, 306.86934, 209.86801, 229.60718, 3.14999],
        [206.26295, 307.63674, 204.73939, 226.86828, 3.14999],
        [201.86811, 308.10888, 200.09819, 224.49601, 3.14999],
        [197.88691, 308.38019, 195.87117, 222.42185, 3.14999],
    ]
)


assert np.allclose(calculated_times, expected_times, atol=1e-1)
assert np.allclose(calculated_temps, expected_temps, atol=1e-2)
