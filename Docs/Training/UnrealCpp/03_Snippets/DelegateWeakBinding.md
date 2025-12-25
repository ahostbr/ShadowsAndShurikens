# Delegate weak-binding pattern

- Prefer AddUObject / AddWeakLambda
- Store FDelegateHandle where removal matters
- Remove in EndPlay / Deinitialize
- Avoid capturing raw pointers in lambdas without WeakObjectPtr
