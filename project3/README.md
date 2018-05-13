# CSCI 5105 Project 3: Fault-Tolerant DFS based on xFS
### By Jon Huhn (huhnx025) and Steven Storla (storl060)

# How to Run Each Component

## Compile
Every component, including client, server, and tests, is compiled in the default target of our Makefile.
```
$ make
```

## Tracker
```
$ ./tracker port
```

Run the tracker on the specified port.

## Peer
```
$ ./peer port tracker_host tracker_port [shared_directory]
```

Run the peer on the specified port and connected to the tracker instance at tracker_host and tracker_port. Optionally, specify the root of the shared directory, which defaults to `~/share`.

The peer will spawn an interactive shell. Of the available commands, `find`, and `update` are only used for testing and not expected to be used by themselves under normal use.

### `get filename`

Retrieve the specified file. This is a wrapper around `Find`, `SelectPeer`, and `Download`.

### `find filename`

List the nodes on which the specified file can be found, according to the tracker.

### `update`

Force the client to send an updated file list to the tracker.

# Design

## Communication
We implemented all of our communication in UDP messages with a custom protocol. Requests are of the form `command [args...]`. Every request a server gets launches a new thread to serve the request.

### Peer-Tracker Interface

The tracker makes the following functions available to each peer:

#### `find filename`

List the nodes on which the specified file can be found.

#### `update peer_name file_list`

Update the tracker's file index. Note that our implementation does not handle file deletions, so if a file is not present in a subsequent update request, it will still be present in the tracker's index.

### Peer-Peer Interface

Peers make the following functions available to each other:

#### `load`

Return the current load on the peer in terms of number of concurrent downloads currently being processed by it.

#### `download filename`

Retrieve the file from the specified server.

## Peer Selection

For each peer that has a requested file, a cost is computed according as 100 * load + latency in ms. The peer with the lowest cost is chosen to download the file from.

## Fault Tolerance

### Tracker Failure
If the tracker fails and restarts, it must recover lost state from the peers that were connected to it. We put this responsibility on the peers, since the tracker has no way to know who was connected to it before it crashed. Every second, peers send heartbeat-like messages to the tracker in the form of `update` requests. If the tracker does not respond within five seconds, the peer goes into recovery mode, where it continues to send `update` requests every second through a non-blocking socket where it does not expect an immediate response. When the tracker restarts, it will receive and respond to all of the peers' `update` requests, both signaling that it is back up and recovering its lost state. Our implementation has a limitation where if a server goes down and immediately recovers within the one-second window of the heartbeat from a peer, its state will not be complete until that peer sends its next `update` request, so a poorly timed `find` request may not find a file that actually exists. In one second, the request should produce the expected result, however.

### Sending Peer Failure
TODO ~ STEVEN


## Implementation Notes
- Our implementation assumes the last packet of a file will have fewer than `PACKET_SIZE` bytes, so files exactly some multiple of `PACKET_SIZE` bytes long will not behave properly.
- We compute a SHA1 hash on each file for its checksum. For some files, the hash contained a null byte that messed up the file transfer. Our automated tests do not use such files however.

# Tests
We made automated tests to test the behavior of the system. We have three primary sets of tests: correctness of file transfer mechanism, ability for peers to recover from tracker failure, and ability for peers to recover from sender failure.

Our file transfer tests test the following:
- Basic, one block file transfers work
  - Files have the same name and path relative to shared directories
  - Files have the same contents
- Large file transfers work
- Directories are created as needed
- Checksums correctly identify mangled files

Our tracker recovery tests test the following:
- Peers are able to detect when the tracker is unavailable
- Peers are able to detect when the tracker recovers
- When the tracker recovers, it restores its state from the peers

Our sending peer recovery tests test the following:
- A receiving peer is able to detect when a sending peer is unavailable.
- The receiving peer resends the request to a different peer if one is available, otherwise it retries the same peer.

## Running the Tests
We provided a test shell script that prepares the shared file system and a test binary that sets up a sample system with some peers and a tracker. Both use the folders `~/share{1,2}` and the shell script recursively deletes all files in these folders before running tests. Please edit the shell script and the constants defined at test.cpp:14-15 if you want to use different directories before running the tests.

To compile and run all of the tests:
```
$ ./test.sh
```

# Performance and Analysis
TODO
