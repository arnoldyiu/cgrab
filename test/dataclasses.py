from dataclasses import dataclass
from enum import Enum


# ! Enums are great for assigning names to fixed, immutable values.
# !! Make sure to type the names in all capitals.
class MacroToKcal(Enum):
    PROTEIN = 4.0
    CARBS = 4.0  # noqa: PIE796
    FATS = 9.0

    # ! @property: access class methods like regular attributes
    @property
    def kcal_per_gram(self) -> float:
        return self.value


# ! @dataclass(frozen=True): prevent modification after instantiating the object
@dataclass(frozen=True)
class Macros:
    protein: float
    carbohydrates: float
    fats: float


@dataclass(frozen=True)
class Food:
    name: str
    macros: Macros

    @property
    def calories(self) -> float:
        p = self.macros.protein * MacroToKcal.PROTEIN.kcal_per_gram
        c = self.macros.carbohydrates * MacroToKcal.CARBS.kcal_per_gram
        f = self.macros.fats * MacroToKcal.FATS.kcal_per_gram

        return p + c + f


chicken = Food("chicken", Macros(30, 0, 9))
avocado = Food("avocado", Macros(3, 13, 22))
rice = Food("white rice", Macros(2.7, 28, 0.3))

for food_item in (chicken, avocado, rice):
    print(
        f"{food_item.calories} kcal in {food_item.name} "
        f"({food_item.macros.protein}g P/"
        f"{food_item.macros.carbohydrates}g C/"
        f"{food_item.macros.fats}g F)"
    )
