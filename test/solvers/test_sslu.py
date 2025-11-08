import pycanha_core as pcc
import numpy as np

# Create a basic model to be solved
tmm = pcc.ThermalMathematicalModel("test_model")

# Create the nodes
node_10 = pcc.Node(10)
node_15 = pcc.Node(15)
node_20 = pcc.Node(20)
node_25 = pcc.Node(25)
env_node = pcc.Node(99)

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

sslu_solver = pcc.solvers.SSLU(tmm)

sslu_solver.MAX_ITER = 100
sslu_solver.abstol_temp = 1e-6

sslu_solver.initialize()
sslu_solver.solve()

calculated_temps = np.array(
    [
        node_10.T,
        node_15.T,
        node_20.T,
        node_25.T,
    ]
)

expected_temps = np.array([132.38706, 306.56526, 111.78443, 200.32387, 3.14999])


assert np.allclose(calculated_temps, expected_temps, atol=1e-2)
