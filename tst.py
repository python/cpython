from typing import NamedTuple

class Employee(NamedTuple):
    """Represents an employee."""
    name: str
    id: int = 3
    def __repr__(self) -> str:
        return f'<Employee {self.name}, id={self.id}>'
