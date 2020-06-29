from .libprio import Prio_init, Prio_clear
from contextlib import ContextDecorator


class PrioContext(ContextDecorator):
    """Add a helper for managing necessary libprio state. Library functions
    will segfault if not run within this context."""

    def __enter__(self):
        Prio_init()

    def __exit__(self, *exc):
        Prio_clear()
