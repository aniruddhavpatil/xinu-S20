# Assignment 4 Report

- The FUTURE_SHARED mode was used for computing the Nth Fibonacci number
- This is because of the nature of overlapping subproblems in the recursive formulation of the function.
- The threads are enqueued and suspended as the recursion stack grows.
- The threads are then resumed when the corresponding future is ready.
- The futures build up to the required Fibonacci's number.