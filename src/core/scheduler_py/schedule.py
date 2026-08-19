import typing
from xmlrpc.client import Boolean
from task_modelling import operation
from task_modelling import task
import matplotlib.pyplot as plt
from matplotlib import cm
from matplotlib.ticker import MaxNLocator

class time_interval():

    def __init__(
        self, t_begin: int, t_end: int, 
        operation_id: int, op: operation) -> None:

        self.t_begin = t_begin
        self.t_end = t_end
        self.operation_id = operation_id
        self.operation = op

class schedule():
    
    def __init__(self, timelines: typing.Dict[str, typing.List[time_interval]]) -> None:
        
        self.agent_timelines = timelines

        operations = set()
        for agent in timelines:
            
            # make sure that intervals are sorted ascending by start time:
            self.agent_timelines[agent].sort(key=lambda interval: interval.t_begin)

            for interval in timelines[agent]:
                operations.add(interval.operation_id)

        self.num_operations = len(operations)

    def show(self) -> None:
        fig, ax = plt.subplots()
        y_tick_labels = []

        viridis_colors = cm.get_cmap("viridis")

        for i, agent in enumerate(self.agent_timelines):

            y_tick_labels.append(agent)

            for interval in self.agent_timelines[agent]:
                interval_color = viridis_colors(interval.operation_id / self.num_operations)
                ax.barh(
                    y=i, width=interval.t_end - interval.t_begin, 
                    left = interval.t_begin, color=interval_color)

                plt.text(
                    interval.t_begin + (interval.t_end - interval.t_begin) / 2, 
                    i, interval.operation_id)
        

        ax.xaxis.set_major_locator(MaxNLocator(integer=True))

        ax.set_yticks(range(len(self.agent_timelines)))
        ax.set_yticklabels(y_tick_labels)
        ax.invert_yaxis()

        plt.waitforbuttonpress()

    def validate(
        self, 
        precedence_graph: task, 
        operation_durations: typing.Dict[str, typing.Dict[operation, int]],
        collaborative_operations: typing.Dict[operation, bool]) -> bool:
        
        agents = self.agent_timelines.keys()

        # each non-synchronous (synchronous) operation  
        # must be assigned to one (two) agent(s) 
        for op in precedence_graph.operations():
            
            op_intervals = [i for agent in agents for i in self.agent_timelines[agent] if i.operation == op]

            if not collaborative_operations[op] and len(op_intervals) != 1:
                return False
            elif collaborative_operations[op] and len(op_intervals) != 2:
                return False

        for agent in agents:

            timeline = self.agent_timelines[agent]

            # intervals of each agent may not overlap;
            # this validation step is based on the assumption
            # that intervals are sorted ascending by their
            # start time as enforced in schedule.__init__            
            for i in range(len(timeline) - 1):
                if timeline[i].t_end > timeline[i + 1].t_begin:
                    return False

            for interval in timeline:
                
                # interval lengths must match the duration of
                # the corresponding work item and worker:                
                interval_length = interval.t_end - interval.t_begin

                if interval_length != operation_durations[agent][interval.operation]:
                    return False

                # collaborative work items require intervals
                # of identical length for two of the agents:
                if collaborative_operations[interval.operation]:

                    other_agents = [a for a in self.agent_timelines.keys() if a != agent]
                    partner_interval = [
                        i for a in other_agents 
                          for i in self.agent_timelines[a] 
                            if i.operation == interval.operation and\
                               i.t_begin == interval.t_begin and\
                               i.t_end == interval.t_end]

                    if len(partner_interval) == 0:
                        return False

        # if work element e' must be done after e
        # then the start time of e' must be later
        # than the end time of e
        precedence_matrix = precedence_graph.precedence_matrix(True)
        
        for successor_id in range(len(precedence_graph.operations())):
            for predecessor_id in range(len(precedence_graph.operations())):
                if precedence_matrix[successor_id, predecessor_id]:

                    successor_op = precedence_graph.operations()[successor_id]
                    predecessor_op = precedence_graph.operations()[predecessor_id]

                    successor_intervals = [i for agent in agents for i in self.agent_timelines[agent] if i.operation == successor_op]
                    predecessor_intervals = [i for agent in agents for i in self.agent_timelines[agent] if i.operation == predecessor_op]

                    if successor_intervals[0].t_begin < predecessor_intervals[0].t_end:
                        return False

        return True        