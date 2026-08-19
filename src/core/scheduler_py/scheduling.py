import typing
import numpy as np

from itertools import combinations
from task_modelling import operation
from schedule import *
from task_modelling import task

class agent():
    
    def __init__(self, 
        name: str, 
        durations: typing.Dict[operation, int],
        capability_indicators: typing.Dict[operation, float],
        agent_availability: typing.List[bool]) -> None:
        
        self.name = name
        self.durations = durations
        self.capability_indicators = capability_indicators
        self.agent_availability = agent_availability

class scheduler():

    def __init__(self, precedence_graph: task) -> None:
        self.task_model = precedence_graph
        self.agents = []

        self.synchronous_operations = {}
        for op in precedence_graph.operations():
            self.synchronous_operations[op] = False

    # adds an agent to the scheduling problem; each agent is characterized by
    #
    # agent_name: string identifier of the agent
    #
    # operation_durations: a dict specifying the (integer-valued) time each 
    #       operation takes if carried out by the agent; must contain a value for
    #       each operation in self.task_model.operations(); a "None" entry for
    #       some operation o indicates that the agent can not perform o at all
    #
    # capability_indicators: a dict specifying how well the agent is suited
    #       for each operation by providing a real value in [0; 1] for each
    #       operation in self.task_model.operations(); lower values indicate
    #       lower suitability of assigning respective operations to this agent;
    #       if no capability_indicators are provided, a value of 1 is assumed
    #       for all operations alike
    #
    # agent_availability: a list specifying, for each timestep, whether the
    #       agent is available for task assignment; we assume that this is a
    #       large time series which goes far beyond the task duration considered
    #       by this scheduling instance; if no agent availability list is provided, 
    #       the agent is assumed to be available at all times
    def add_agent(
        self,
        agent_name: str,
        operation_durations: typing.Dict[operation, int],
        capability_indicators: typing.Dict[operation, float] = None,
        agent_availability: typing.List[bool] = None) -> None:

        if capability_indicators == None:
            capability_indicators = {}
            for e in self.task_model.operations():
                capability_indicators[e] = 1.

        if agent_availability == None:
            agent_availability = []

        for op in self.task_model.operations():
            if not op in operation_durations:
                raise Exception(
                    "operation_durations must contain a value for each operation!")
            if not op in capability_indicators:
                raise Exception(
                    "capability_indicators must contain a value for each operation!")

        self.agents.append(
            agent(
                agent_name, 
                operation_durations,
                capability_indicators,
                agent_availability))

    def set_operation_collaborative(self, op: operation) -> None:
        if not op in self.task_model.operations():
            raise Exception("This operation is not part of the task to be scheduled!")

        self.synchronous_operations[op] = True

    def _duration_matrix(self) -> np.array:
        D = np.full(
            (len(self.task_model.operations()), len(self.agents)), 1)

        for op in self.task_model.operations():
            e = self.task_model.operations().index(op)

            for w in range(len(self.agents)):
                if self.agents[w].durations[op] != None:
                    D[e, w] = self.agents[w].durations[op]

        return D

    def _capabilities_matrix(self) -> np.array:
        B = np.full(
            (len(self.task_model.operations()), len(self.agents)), True)

        for op in self.task_model.operations():
            e = self.task_model.operations().index(op)

            for w in range(len(self.agents)):
                if self.agents[w].durations[op] == None:
                    B[e, w] = False

        return B

    def _capability_indicators_matrix(self) -> np.array:
        A = np.full(
            (len(self.task_model.operations()), len(self.agents)), 1.)

        for op in self.task_model.operations():
            e = self.task_model.operations().index(op)

            for w in range(len(self.agents)):
                A[e, w] = self.agents[w].capability_indicators[op]

        return A

    def _agent_availability_matrix(self) -> np.array:
        # We assume that the agent.agent_availability vectors
        # are large time series ranging far beyond the horizon
        # of the task considered by this scheduling instance;
        # i. e. the longest of these time series prescribes the
        # dimensions of the T matrix
        max_availability_vector_len = max([len(a.agent_availability) for a in self.agents])

        T = np.full(
            (len(self.agents), max_availability_vector_len), True)

        # fill a matrix storing the availability over time
        # of one agent per row; if the agent_availability 
        # vector specified for some agent is shorter than 
        # the maximum time series length, just assume that 
        # the worker will be available during any further 
        # time steps (as these will likely not be occupied 
        # by the schedule, and the user could have specified 
        # otherwise by preparing a longer agent_availability vector)
        for w in range(len(self.agents)):
            for p in range(len(self.agents[w].agent_availability)):
                T[w, p] = self.agents[w].agent_availability[p]
            for p in range(len(self.agents[w].agent_availability), max_availability_vector_len):
                T[w, p] = True

        return T

    def _synchronous_operations_vector(self) -> np.array:
        S = np.full(
            (len(self.synchronous_operations)), True)

        for op in self.task_model.operations():
            e = self.task_model.operations().index(op)

            S[e] = self.synchronous_operations[op]

        return S

    def _build_schedule(self, discrete_schedule: typing.List[np.array]) -> schedule:
        num_timepoints = len(discrete_schedule)
        num_work_elements, num_workers = np.shape(discrete_schedule[0])
        
        intervals = {}

        for w in range(num_workers):
            
            worker_intervals = []
            
            for e in range(num_work_elements):

                interval_start = None

                for p in range(num_timepoints):
                    if discrete_schedule[p][e, w] > 0.99\
                        and interval_start == None:

                        interval_start = p

                    elif discrete_schedule[p][e, w] <= 0.99 and interval_start != None:
                        worker_intervals.append(
                            time_interval(
                                interval_start, p, e, 
                                self.task_model.operations()[e]))

                        interval_start = None

                if interval_start != None:
                    worker_intervals.append(
                        time_interval(
                            interval_start, num_timepoints, e, 
                            self.task_model.operations()[e]))

                    interval_start = None

            intervals[self.agents[w].name] = worker_intervals
            intervals[self.agents[w].name].sort(key=lambda interval: interval.t_begin)

        return schedule(intervals)

    def _validate_problem_instance(self) -> None:

        for e in self.task_model.operations():
            
            capable_agents = set()

            for w in self.agents:
                if w.durations[e] != None:
                    capable_agents.add(w)

            # there must be at least one agent capable
            # of each non-synchronous work item
            if not self.synchronous_operations[e]:
                if len(capable_agents) < 1:
                    raise Exception("Scheduling infeasible!")

            # at least two agents mus be capable of any
            # synchronous work item; we further assume
            # that these agents require the same amount
            # of time for respective work items
            if self.synchronous_operations[e]:
                if len(capable_agents) < 2:
                    raise Exception("Scheduling infeasible!")
                else:
                    num_valid_combinations = 0

                    agent_combinations = combinations(capable_agents, 2)

                    for combination in agent_combinations:
                        if combination[0].durations[e] == combination[1].durations[e]:
                            num_valid_combinations += 1

                    if num_valid_combinations < 1:
                        raise Exception("Scheduling infeasible!")

    def _validate_features(self):
        # some solvers may support only a subset of all
        # features relevant to the hrc scheduling problem
        # depending on their implementation (in particular, 
        # considering collaborative operations or limited
        # agent availability over time may not be feasible
        # with some milp formulations); this method is
        # intended to be implemented by child classed to
        # throw an exception if the given scheduling
        # instance would require features that the 
        # implementation does not support (e.g.: agents with
        # limited availability or operations with the
        # "collaborative" flag set were specified, but
        # the scheduler implementation does not implement
        # corresponding features)
        raise Exception(
            "Child classes must implement this method!")

    def _solve(self) -> schedule:
        raise Exception(
            "Child classes must implement this method!")

    def generate_schedule(self) -> None:
        self._validate_features()
        self._validate_problem_instance()
        return self._solve()