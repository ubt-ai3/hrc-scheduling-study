from schedule import *
from list_scheduler import *
from task_modelling import *

if __name__ == "__main__":

    t = task()

    operation_0 = operation()
    operation_1 = operation()
    operation_2 = operation()
    operation_3 = operation()
    operation_4 = operation()

    t.add_operation(operation_0)
    t.add_operation(operation_1)
    t.add_operation(operation_2)
    t.add_operation(operation_3)
    t.add_operation(operation_4)

    t.add_precedence(t.start_node(), operation_0)
    t.add_precedence(operation_0, operation_1)
    t.add_precedence(operation_1, t.end_node())
    t.add_precedence(t.start_node(), operation_2)
    t.add_precedence(operation_2, operation_1)
    t.add_precedence(t.start_node(), operation_3)
    t.add_precedence(operation_3 ,t.end_node())
    t.add_precedence(t.start_node(), operation_4)
    t.add_precedence(operation_4, t.end_node())

    t.initialize_node_states()

    t.draw_task_progress()

    scheduler = list_scheduler(t)

    scheduler.add_agent("human", {operation_0: 5, operation_1: 5, operation_2: 5, operation_3: 5, operation_4: 5})
    scheduler.add_agent("robot", {operation_0: 10, operation_1: 10, operation_2: 10, operation_3: 10, operation_4: 10})

    workflow = scheduler.generate_schedule()

    workflow.show()