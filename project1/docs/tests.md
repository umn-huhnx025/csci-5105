# Tests

## Our Publish-Subscribe Implementation

### Client
- Starting while server is down fails gracefully => RPC timeout
- Passing a bad server hostname on startup fails gracefully => RPC timeout
- Client joins server automatically on startup.
- Client leaves server automatically on exit.
- Client pings the server every 5 seconds.
- Client stops pinging on exit.
- Passing a bad article/subscription at the prompt displays an error message.
- Killing the server triggers the `ping` error message.
- Multiple clients connected to a single server work.
- Client and server work over different machines.
- Subscription mechanism works
  1. `publish Sports;;;c` => Client should not receive new article.
  1. `subscribe Sports;;;`
  1. `publish Sports;;;c` => Client should receive new article.
  1. `subscribe Sports;;;` => Client should see an error message since the subscription is a duplicate.
- Complex subscriptions work
  1. `subscribe Sports;Somewhere;;`
  1. `publish Sports;;;c` => Client should not receive new article.
  1. `publish Sports;Somewhere;;c` => Client should receive new article.
  1. `publish Sports;Somewhere;UMN;c` => Client should receive new article.
  1. `publish ;Somewhere;UMN;c` => Client should not receive new article.
  1. `publish ;Somewhere;;c` => Client should not receive new article.
- Unsubscribe mechanism works
  1. `subscribe Sports;;;`
  1. `publish Sports;;;c` => Client should receive new article.
  1. `unsubscribe Sports;;;`
  1. `publish Sports;;;c` => Client should not receive new article.
- Unsubscribe only accepts current subscriptions
  1. `unsubscribe Sports;;UMN;` => Client sees an error message.
  1. `subscribe Sports;;UMN;`
  1. `unsubscribe Sports;;;` => Client sees an error message.
  1. `unsubscribe ;;UMN;` => Client sees an error message.
  1. `unsubscribe Sports;Somewhere;UMN;` => Client sees an error message.
  1. [From another client] `unsubscribe Sports;;UMN;` => Client sees an error message.
  1. `unsubscribe Sports;;UMN;` => Completes successfully


### Server
- Server connects to registry server

#### Publish
article|OK?|reason
-|-|-
Sports b c d|FAIL|Invalid format
;;;content|FAIL|Invalid format for any article
;;;|FAIL|Invalid format for any article
a;b;c;content|FAIL|Invalid type
Sports;;;|FAIL|Published articles must have content
Politics;b;c;|FAIL|Published articles must have content
Lifestyle;a;b;c|OK|
Technology;;;c|OK|
;a;b;c|OK|
;;b;c|OK|
;b;;c|OK|

#### Subscribe
article|OK?|reason
-|-|-
Sports b c d|FAIL|Invalid format
;;;|FAIL|Invalid format for any article
;;;content|FAIL|Invalid format for any article
Entertainment;;;content|FAIL|Subscriptions can't have content
Business;b;c;content|FAIL|Subscriptions can't have content
Health;;;|OK|
Science;b;c;|OK|
;;c;|OK|


### Automated Tests

We have some automated tests on the client side and the server side. Server-side unit tests check the correctness of our validity checking for publications and subscriptions and the subscription matching algorithm. Client-side tests spawn `MAXCLIENT` automated clients that all concurrently join the server, subscribe to some articles, and publish some articles. Our implementation can handle up to about 1000 clients each with 1 subscription and publishing 1 article.

To run the server-side tests,

```
$ ./pubsub_server test
```

To run the client simulations,

```
$ ./pubsub_client <host> test
```

Parameters for the client simulation can be changed at the top of pubsub_client.c (`num_subs` and `num_pubs`, each per client) and in pubsub.h (`MAXCLIENT`, the number of clients to simulate).

Some results from our client simulations are in the table below. (Server running on kh4250, clients on kh1250)

`MAXCLIENT`|`num_subs`|`num_pubs`|number of articles sent|time (s)|errors
-|-|-|-|-|-
1|1|1|0|0.005|0
1|10|10|0|0.009|0
1|100|100|35|0.065|0
1|1000|1000|574|0.481|0
1|10000|10000|5803|6.216|0
10|1|1|0|0.021|0
10|10|10|16|0.028|0
10|100|100|3481|0.137|0
10|1000|1000|56706|3.900|0
100|1|1|0|0.142|0
100|10|10|230|0.114|0
100|100|100|299893|8.348|0
500|1|1|10|20.851|0


## Google PubSub Client

To run the unit tests, the command line argument `test` must be added when you start up the client. eg.
`python client.py test` There are four tests that will be ran to test the functionality of  **create topic**, **subscribe**, **unsubscribe** and **publish**. Each test will take the output from standard out and create an assertion of what is should be.

- test_ a _ create _ topic: This test will first create a topic and assert that there is no output, this is because it was created without errors. Next it will try to create the same topic and asserts that a '409 topic already exists' will  be sent to standard out.
- test _ b _ subscribe: This test will first subscribe to the topic that was created and assert that there is no output. Next it will try to subscribe to a topic that does not exist and assert that a '404 Subscription does not exist' will be sent to standard out.
- test _ c _ publish: This test will publish an article for the topic that was created and subscribed to, sleep so that the output can be printed, and then assets that **New message** output is sent to standard out with the correct content.
- test _ d _ unsubscribe: This test will fist unsubscribe to the topic it was subscribed to and assert that there is no output. Next it will subscribe to the same topic that was created and assert that there is no output. It then also unsubscribes to to a topic that does not exist and asserts a '404 Subscription does not exist' will  be sent to standard out.
