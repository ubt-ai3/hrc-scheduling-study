import networkx as nx
import networkx.algorithms as nx_algorithms
import networkx.algorithms.dag as nx_dag_algorithms
import matplotlib.pyplot as plt
import typing
import numpy as np

class operation():

    def __init__(
        self) -> None:
        pass

class task:

    def __init__(self, operations=None, precedences: np.ndarray = None):

        self.precedence_graph = nx.DiGraph()

        self.precedence_graph.add_node(
            self.start_node(),
            state="done",
            node_name="start",
            is_operation=False)
        self.precedence_graph.add_node(
            self.end_node(),
            state="inactive",
            node_name="goal",
            is_operation=False)

        self.last_operation = None
        self.operation_counter = 0

        if operations:
            for op in operations:
                self.add_operation(op)

        if precedences is None:
            return
        for row in range(precedences.shape[1]):
            if not any(precedences[row, ]):
                self.add_precedence(self.start_node(), self.operations()[row])
                continue

            for column in range(precedences.shape[0]):
                if precedences[row, column] == 0:
                    continue

                self.add_precedence(operations[column], operations[row])

        for column in range(precedences.shape[0]):
            if not any(precedences[:, column]):
                self.add_precedence(self.operations()[column], self.end_node())
                continue

    def start_node(self) -> str:
        return "start_node"

    def end_node(self) -> str:
        return "end_node"

    def size(self) -> int:
        """
        :return: number of operation nodes in the precedence graph
        """
        return len([x for x, y in self.precedence_graph.nodes(data=True) \
                if y["is_operation"] == True])


    def add_operation(self, op: operation) -> None:
        """
        add an operation to the precedence graph
        :param operation: instance of operation class
        :return: nothing
        """
        self.precedence_graph.add_node(
            op,
            state="inactive",
            node_name="operation_" + str(self.operation_counter),
            is_operation=True)

        self.operation_counter += 1

    def add_precedence(self, from_node, to_node) -> None:
        """
        add edge to the precedence graph
        :param from_node: starting (operation) node
        :param to_node: end (operation) node
        :return: nothing
        """
        self.precedence_graph.add_edge(from_node, to_node)

    def initialize_node_states(self) -> None:
        for node in self.precedence_graph.nodes:
            nx.set_node_attributes(
                self.precedence_graph,
                {node: "inactive"},
                "state")

        nx.set_node_attributes(
            self.precedence_graph,
            {self.start_node(): "done"},
            "state")

        for successor in self.precedence_graph.successors(self.start_node()):
            nx.set_node_attributes(
                self.precedence_graph,
                {successor: "active"},
                "state")

    def operations(self) -> typing.List[operation]:
        """
        :return: all operations in a list
        """
        return [x for x, y in self.precedence_graph.nodes(data=True) \
                if y["is_operation"] == True]

    def valid_execution_order(self) -> typing.List[operation]:

        topological_sorting = list(nx_algorithms.topological_sort(self.precedence_graph))

        topological_sorting.remove(self.start_node())
        topological_sorting.remove(self.end_node())

        return topological_sorting

    def active_operations(self) -> typing.List[operation]:
        """
        :return: all active operations in a list
        """
        return [x for x, y in self.precedence_graph.nodes(data=True) \
                if y["state"] == "active" and y["is_operation"] == True]

    def inactive_operations(self) -> typing.List[operation]:
        """
        :return: all inactive operations in a list
        """
        return [x for x, y in self.precedence_graph.nodes(data=True) \
                if y["state"] == "inactive" and y["is_operation"] == True]

    def done_operations(self) -> typing.List[operation]:
        """
        all done/ finished operations in a list
        :return:
        """
        return [x for x, y in self.precedence_graph.nodes(data=True) \
                if y["state"] == "done" and y["is_operation"] == True]

    def is_done(self, op: operation) -> bool:
        """
        :param operation: instance of class operation
        :return: true if operation is done/ finished
        """
        return self.precedence_graph.nodes[op]["state"] == "done"

    def is_active(self, op: operation) -> bool:
        """
        :param operation: instance of class operation
        :return: true if operation is active
        """
        return self.precedence_graph.nodes[op]["state"] == "active"

    def is_inactive(self, op: operation) -> bool:
        """
        :param operation: instance of class operation
        :return: true if operation is inactive
        """
        return self.precedence_graph.nodes[op]["state"] == "inactive"

    def finish_operation(self, op: operation):
        """
        set operation to done and enable successors if all predecessors are done
        :param operation: instance of class operation
        :return: nothing
        """
        if self.precedence_graph.nodes[op]["state"] == "done":
            return

        if self.precedence_graph.nodes[op]["state"] != "active":
            raise Exception("Requested operation is not yet active!")

        nx.set_node_attributes(self.precedence_graph, {op: "done"}, "state")
        self.last_operation = op

        for successor in self.precedence_graph.successors(op):
            set_successor_active = True

            for predecessor in self.precedence_graph.predecessors(successor):
                if self.precedence_graph.nodes[predecessor]["state"] != "done":
                    set_successor_active = False
                    break

            if set_successor_active:
                nx.set_node_attributes(self.precedence_graph, {successor: "active"}, "state")

    def task_finished(self) -> bool:
        """
        :return: True if all operations are done
        """
        remaining_operations = self.active_operations()
        return len(remaining_operations) == 0

    def operation_label(self, operation) -> str:
        """
        name of operation
        :param operation: instance of class operation
        :return: str
        """
        return self.precedence_graph.nodes[operation]["node_name"]

    def draw_task_progress(self) -> None:
        """
        plot the precedence graph with marked inactive, active and done nodes
        :return: nothing
        """
        node_labels = {}
        for node in self.precedence_graph.nodes:
            node_labels[node] = self.precedence_graph.nodes[node]["node_name"]

        color_state_map = {"done": 'green', "inactive": 'red', "active": 'yellow'}

        pos = nx.planar_layout(self.precedence_graph)

        nx.draw(
            self.precedence_graph,
            pos,
            with_labels=True,
            labels=node_labels,
            node_color=[color_state_map[node[1]['state']]
                        for node in self.precedence_graph.nodes(data=True)])
        plt.show(block=True)

    # output: matrix P where P[e', e] == True
    # iff e' must be done after e
    def precedence_matrix(self, only_adjaceny: bool = False) -> np.array:

        num_operations = len(self.operations())

        matrix = np.full(
            (num_operations, num_operations),
            False)

        for i in range(num_operations):
            operation = self.operations()[i]

            predecessors = []
            if only_adjaceny:
                predecessors = [n for n in self.precedence_graph.predecessors(operation)
                    if n != self.start_node()]
            else:
                predecessors = [n for n in nx_dag_algorithms.ancestors(self.precedence_graph, operation)
                    if n != self.start_node()]

            for predecessor in predecessors:
                predecessor_index = self.operations().index(predecessor)
                matrix[i, predecessor_index] = True

        return matrix
