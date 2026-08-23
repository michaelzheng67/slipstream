A single-instrument execution algorithm in C++ that validates trades made against a rolling VWAP window based on quotes submitted by market participants.

build:
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

tests:
ctest --test-dir build --output-on-failure

Measurement plan

Latency (p50, p90, p99. We care about tail time here as the tail can kill our trading strategy)

- tick to order. measuring the time it takes for the first byte to come from the server to the OE
client, and have the client respond with its own payload
- jitter. taking both mean and deviation of tick to order, as well as time between consecutive packets,
we are able to have a more reliable system
- component level tracing. i.e. serializing / deserializing, queueing delays, app logic, etc

Throughput

- messages per second, at both sustained and peak burst (i.e. 5x, 10x normal load)
- behaviour under load spikes (i.e. linear latency, massive spike after X data volume)
- queue depth, backpressure (specifically for the networking portion of client <-> server)

Resources

- CPU, memory, NIC utilization. both at startup and during sustained running of the system
- thread utilization characteristics. time spent in cpu vs. I/O
- lock contention / context switches in the hot path

Scalability

- max concurrent connections
- connection setup / teardown costs