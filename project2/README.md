# CSCI 5105 Project 2: Replicated Bulletin Board
### By Jon Huhn (huhnx025) and Steven Storla (storl060)

# How to Run Each Component

## Compile
Every component, including client, server, and tests, is compiled in the default target of our Makefile.
```
$ make
```

## Client
A cleint can connect to the coordiator or any replica server at any time. Any number of clients can connect to any number of servers.
To connect to a server execute the following command `./client <server_host> <server_port>` where the server_host is the ip adress and the server_port is the port for that specific server. For more information on the functionality of the client please refer to **Client-Server Interface**.

## Server
The server application can be launched as either the coordinator or a replica. 
#### Coordinator
The coordinator is the facilitator for all servers and is intended to be running at all times. to launch the coordinator execute the following command `./server <port> <[seq|quo|ryw]>` sequential consistency, quorum consistency, and read-your-write consistency are seq, quo, and ryw respectively. When the coordinator is running, any replica server can connect and communicate with it via it's name and port. 
#### Replica
The replica server may connect to the coordinator at any time. the consistency of the replica server will be sent by the coordinator so it need not be specified. To launch a replica server, execute the following command <br />  `./server <port> <coordinator_ip> <coordinator_port>`.  For a client to connect to a replica server, they should use the port specified by the port argument and not the coordinator's port. 

The servers will all behave the same from the clients side regardless as to whether they are the coordinator or a replica. For more information on the servers please refer to the **Server-Server Interface** and **Communication** sections.


## Tests
#### Overview

We made automated tests to check our consistency policies for correctness. Each spawns a configurable number of servers and clients on the same machine. Clients are evenly distributed among the servers. The clients perform some concurrent operations and then the server states are checked for correctness. 
Sequential and quorum consistencies have a straight forward test though a sequence of posts and reads; furthermore, they have read and write throughput measurements in terms of requests/second and bytes/second. Read-you-write consistnacy is a bit trickier to test; we needed to emulate a client moving to a different sever and comparing their read to their previous write. To ensure that the post has not propagated to the server that has done the read, under read-you-write consistency, a two second propagation delay has been added.

#### How to run
The tests have the following structure `./test <[seq|quo|rwy]> <num_clients> <num_servers> (<ratio>)`<br />
* To test sequential consistency, execute the following `./test <seq> <num_clients> <num_servers>` This will run a sequence of posts and reads and compare the order of every read to insure they are same order across all servers. <br />

* To test quorum consistency, execute the following `./test <quo> <num_clients> <num_servers> <ratio>` where ratio is the desired ratio of readers to writers. NOTE the ratio should be in decimal notation not a fraction. Like  sequential consistency, this will run a sequence of posts and reads. <br />

* To test read-your-write consistency, execute the following `./test <ryw> <num_clients> <num_servers>` NOTE the max number or servers for this test is three and the max number or clients for this test is two; the max values are also strongly recommended and any values about the max will be corrected down to three and two respectively.  This test will post an article, and while the post is propagating to other servers, read before the propagation has arrived. This is to show that even if the value has yet to propagate to that specific server, the client will be able to still read their write. The test will also display some functionality to standard out. `<br />


# Design

## Bulletin Board Structure
Our articles contain only one text field for content. Each article has references to its parent and child articles, forming a tree with a default `root` article with ID 0. Here is an example of our bulletin board:

```
1: this is post 1
    2: this is reply 2 to 1
        3: this is reply 3 to 2
            5: this is reply 5 to 3
            7: this is reply 7 to 3
                8: this is reply 8 to 7
                    9: this is reply 9 to 8
        4: this is reply 4 to 2
            6: this is reply 6 to 4
                10: this is reply 10 to 6
```

## Communication
We implemented all of our communication in UDP messages with a custom protocol. Requests are of the form `command [args...]`. Every request a server gets launches a new thread to serve the request. Requests that must be forwarded between servers are prefixed with an "s" to avoid infinite loops where a server keeps forwarding a request to itself.

### Client-Server Interface

#### Post
```
post <parent_id> <contents>
```
Post or reply to an article on the bulletin board. On success, the server responds "ok" to the client. On failure, an error message is returned. R

#### Reply
Reply is considered the same as a post under a non-zero `<parent_id>`, so its protocol is the same as for post.

#### Read
```
read <offset> <count>
```
Read one page of articles on the bulletin board. To support paginated reads, each read command also takes an offset and count of articles to return. On success, the articles are returned. On failure, an error message is returned.

#### Choose
```
choose <article_id>
```
Read the contents of a single message on the bulletin board. On success, the article string is returned. On failure, an error message is returned.

### Server-Server Interface

#### Register
```
register <hostname>:<port>
```
Replicas must register with the coordinator before they can accept requests. The hostname/port combination uniquely identifies the server. On success, "ok" is returned by the coordinator. On failure, an error is returned and the requesting server exits.

#### Deregister
```
deregister <hostname>:<port>
```
Replicas deregister with the coordinator before they exit so the coordinator knows it will no longer respond to requests. On success, "ok" is returned by the coordinator. On failure, an error message is displayed and the server exits.

#### Version
```
version
```
Query the current version of the bulletin board on the server. On success, the version number is returned. On failure, an error message is returned.

#### Sync
```
sync
```
Tell the server to update to the latest version of the bulletin board. On success, "ok" is returned. On failure, an error message is returned.

# Statistics
After testing each of the consistencies ten times with ten clients and ten servers while the synthetic network delay is turned off, we get the following. Quorum consistency was the slowest in both reading and mostly in writing as well. It was tested at both ratio extremes of 1 and .1, and at an idel ratio of writers to readers. When the ratio parameter was 1, it had an average write throughput of 128 requests/second or 3.63 KB/second and a read throughput of 917 requests/second or 752.34 KB/second. When the ratio was changed to .1, the read throughput about doubled becoming comparable to read-your-write but had very bad write throughput average of 79 requests/second. The increse of read throughput is due to the small number of readers to compare. The optimal ratio seemed to be .6, having an average write throughput of 148.06 requests/second or 4.14 KB/second and a read throughput of 891 requests/second or 731.15 KB/second. Even with an optimized load for readers and writers, the consistency is still the slowest. This can be explained by the substantial overhead we have by calling synch; we could potentially achieve much better speeds if synch were to be optimized better. Sequential consistency was much faster at reading than read-your-write consistency, but much slower at writing. With an average write throughput of 806 requests/second or 22.56 KB/second and an average read throughput of 4722 requests/second or 3874.85 KB/sec, this can be explained by the fact that sequential must wait for all propagation to be complete before returning after every write. Lastly, we have read-your-write consistency with a much higher write throughput average of 2348 request/second or 65.76 KB/second and a decent read throughput of 1794 requests per/second or 1489.62 KB/second. This can be explained by having to go to the primary for every read operation. One last thing to also consider is the response time. For read-your-write consistency, the initial wait of blocking until all servers have been propagated the write are bypassed, and the response time is increased by roughly: 
([(network delay * number of servers) + time to read from primary] – time to read from server) making it ideal under certain situations.



