I need to translate the graphene codebase in `../finite_crystal_elasticity/` (including graphene simulator and prepro code) to a C++ version with exactly the same functionalities. 

# Rule to follow

1. You are doing a **Scientific Computing Project**. Any logical, algorithmic, or numerical error will possibly cause fatal errors. Therefore, you need to strictly follow the mathematics/physics theory in the given references. 

2. Carefully record the implementation of each step in documentation files (`./document`), especially experience learned from error/bugs. Update `./AGENT.md` and other related file if the structure of the project folder is changed. Generally, accumulate any useful experience into documentations for future re-use.

3. Before doing anything, check documentation to see if there is any existing experience in documentation.

4. Strictly refer the description in references as much as you can. Do not use heuristics ways to bypass computational steps.

5. For each implementation step, find way to numerically verify the correctness of implemented modules.
