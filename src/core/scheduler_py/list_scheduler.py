import typing
import scheduling

import numpy as np

from task_modelling import task
from schedule import schedule

class list_scheduler(scheduling.scheduler):

    def __init__(self, precedence_graph: task) -> None:

        super().__init__(precedence_graph)

    def _timediscrete_list_schedule(self) -> typing.List[np.array]:
        num_work_items = len(self.task_model.operations())
        num_workers = len(self.agents)

        list_schedule = []

        self.task_model.initialize_node_states()

        unassigned_workers = set(range(len(self.agents)))
        running_operations = {}
        current_timestep = 0

        B = self._capabilities_matrix()
        S = self._synchronous_operations_vector()
        D = self._duration_matrix()
        T = self._agent_availability_matrix()

        while not self.task_model.task_finished():
            # create space for the assignment in current_timestep
            list_schedule.append(np.full((num_work_items, num_workers), 0))

            # assign workers
            active_operations = [self.task_model.operations().index(o) for o in self.task_model.active_operations()]
            not_yet_started = [o for o in active_operations if o not in running_operations]

            for e in not_yet_started:
                capable_workers = [w for w in unassigned_workers if B[e, w]]
                available_workers = []

                # we are scheduling in a non-preemptive manner, i.e.
                # e can only be assigned to some worker e iff e is 
                # availabe for at least D[e, w] timesteps from now
                for w in capable_workers:
                    p = len(list_schedule) - 1
                    
                    ts = [T[w, p_] for p_ in range(p, min(T.shape[1], p + D[e, w]))]
                    available_timesteps = sum(ts)

                    if available_timesteps >= min(D[e, w], max(0, T.shape[1] - p)):
                        available_workers.append(w)

                # non-synchronous work item (greedily pick the
                # worker with lowest processing time for e)
                if not S[e] and len(available_workers) > 0:
                    w = min(available_workers, key=lambda x: D[e, x])
                    running_operations[e] = [(w, D[e, w])]
                    unassigned_workers.remove(w)
                #synchronous work item
                elif S[e]:
                    # we need two workers with equal duration for e
                    suited_workers = [[(w0, w1) for w1 in available_workers if w0 != w1 and D[e, w0] == D[e, w1]] for w0 in available_workers]
                    suited_workers = [pair for pair in suited_workers if len(pair) > 0]

                    if len(suited_workers) > 0 and len(suited_workers[0]) > 0:
                        (w0, w1) = suited_workers[0][0]
                        running_operations[e] = [(w0, D[e, w0]), (w1, D[e, w1])]

                        unassigned_workers.remove(w0)
                        unassigned_workers.remove(w1)

            # proceed with running work items
            for e in running_operations:
                for i in range(len(running_operations[e])):
                    (w, remaining_duration) = running_operations[e][i]
                    list_schedule[current_timestep][e, w] = 1

                    running_operations[e][i] = (w, remaining_duration - 1)

            # update finished tasks and free workers
            finished_operations = set()
            for e in running_operations:
                for assigned_worker in running_operations[e]:
                    (w, remaining_duration) = assigned_worker

                    if remaining_duration == 0:
                        self.task_model.finish_operation(
                            self.task_model.operations()[e])
                    
                        unassigned_workers.add(w)
                        finished_operations.add(e)
            
            for e in finished_operations:
                running_operations.pop(e)

            current_timestep += 1

        return list_schedule

    def _validate_features(self):
        # all features that can be expressed with scheduling.agent
        # objects and which are available via the generic scheduler
        # interface are supported by this scheduler 
        pass

    def _solve(self) -> schedule:
        return self._build_schedule(self._timediscrete_list_schedule())