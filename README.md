# backuperator-daemon

## Dependencies
### crypto++
Used for encryption, hashing and key derivation. crypto++ is included as a sub-
module, and must be built before the daemon will compile properly. (It is
strongly suggested that the static library is used, instead of dynamically
linking against it.)

### Boost
C++ class library used for various convenience functions, including networking.
We assume that Boost is installed system-wide.

### CTPL
Used to provide simple thread pools throughout the project. This is a header-
only library, so no compilation is required.

### Google protobuf
Google's Protocol Buffers (protobuf) library is used as a way to serialize data
that's exchanged over the network connection. Like Boost, we assume that the
protobuf library is installed system-wide.

### Google glog
`glog` provides a simple, yet feature-rich logging API. It's assumed that glog
is installed system-wide.
