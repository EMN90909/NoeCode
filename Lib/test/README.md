# Standard-library testing policy

Every public library function must gain positive, boundary and failure-path tests before being called stable. Until import semantics are implemented, seed library sources are validated directly by compiler conformance tests rather than pretending module discovery exists.
