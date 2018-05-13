# Design Document

## Features

We implemented the core PubSub functionality including the following:
- Client RPC interface to group server with functions to `Join`, `Leave`, `Subscribe`, `Unsubscribe`, `Publish`, and `Ping`.
- Send new articles from the server to subscribed clients via UDP.
- Interface between group server and registry server to `Register`, `Deregister`, and `GetList`.
- Group server will respond to ping messages from the registry server.
    - Note: We tested our communication with the test registry server. The address of the registry server can be changed [here](../pubsub_svc.cpp), lines 23-24.
- Multiple clients can connect to the same server instance.
- Group server and clients work across multiple hosts.

We did not implement `JoinServer`, `LeaveServer`, or `PublishServer`.

## Article and Subscription Format
Articles and subscriptions are strings each with 4 fields, separated by semi-colons, up to a total length of `MAXSTRING`, which is defined as 120. The format is defined as `<type>;<originator>;<org>;contents`. Valid types include ​Sports, Lifestyle, Entertainment, Business, Technology, Science, Politics, or Health. Each of the other fields may contain arbitrary strings.

## Server
The server provides the following RPC interface to the client via UDP. The arguments `IP` and `Port` identify the endpoint of the client where the server can send new articles. These arguments are required for each function so the server can uniquely identify which client called the function. Each function returns an integer value, where 0 indicates success and any other value indicates failure.

#### `Join(string IP, int Port)`
`Join` registers a client with the group server. Clients do not `Join` the group server manually. The client will automatically `Join` the group server specified at startup on the command line. The server maintains a registry of currently connected clients as an array of `Client` objects up to length `MAXCLIENT`, which is defined as 10. An error will be returned to the client when the client is already registered with the server or there is no room for more clients. This function is idempotent.

#### `Leave(string IP, int Port)`
`Leave` removes the client from the registry. Clients will automatically `Leave` the server specified at startup when it exits. An error will be returned to the client if the client is not found in the registry. This function is idempotent.

#### `Subscribe(string IP, int Port, string Article)`
`Subscribe` tells the group server that the client wants to receive articles matching some description, specified by `Article`. ​For subscriptions, at least one of the first three fields of `Article` must be present, and the last field must be left blank. Each `Client` object has a list of subscriptions. The limit on the number of subscriptions a client may have is only bounded by the capacity of main memory. An error is returned to the client when the subscription format is invalid, the client cannot be found, or the client already has a description matching `Article`. This function is idempotent.

#### `Unsubscribe(string IP, int Port, string Article)`
`Unsubscribe` removes a subscription from the client's list of subscriptions, so that client will no longer receive articles corresponding to that topic. `Article` must exactly match a client's subscription, otherwise an error is returned to the client. Errors are also returned when `Article` does not satisfy the subscription format requirements or the client cannot be found. This function is idempotent.

#### `Publish(string Article, string IP, int Port)`
`Publish` tells the group server to push the specified article to any client subscribed to it. The group server iterates over its client registry, checking if each client is subscribed to the article. If so, it sends the article via UDP to the client. Since clients do not receive articles published before they subscribe to them, the server does not need to keep a record of all published articles. Published articles must have content and at least one other field not be blank. An error is returned if the article is not in the proper format or the server is unable to send the article to at least one subscribed client.


#### Subscription Matching
When a new article is published, the server iterates over each registered client,  determining whether or not the client should receive the new article. For each of a client's subscriptions, if the subscription matches the article's fields exactly, or a subset of the article's fields, then the client is sent the new article. For example, a client subscribed to `Sports;;UMN;` would receive the article `Sports;Somewhere;UMN;content` but not the article `Sports;;;content`.

## Client
Our client runs as a very basic shell. The group server name or IP is passed at the command line. The client spawns two additional threads: one to ping the group server every 5 seconds, and another to listen for new articles from the server. On the main thread, the client automatically `Join`s the group server and begins accepting user input. The commands available are listed [here](client.md). Once the user quits, the client automatically `Leave`s and the process terminates.


## Google PubSub Client

Our Google PubSub client is written in Python 3. It functions like the client in our implementation. For more details on how to use the client, read [howto.md](howto.md). The biggest issue we ran into with the Google API was figuring out the asynchrony mechanisms to display new messages to subscribed clients. Another issue we considered was how to uniquely identify clients so that each client can have a subscription to a particular topic. We ended up appending the client's hostname and PID to the subscription. This way, multiple clients on the same host can subscribe to the same topic, and clients on multiple hosts can subscribe to the same topic. Once the client exits, it deletes all of its subscriptions, so the same host/PID combination can be used again.

Compared to implementing publish-subscribe from scratch, the Google PubSub client was much simpler to use. While it took us at least a week to write our own implementation, we finished the Google PubSub client in a couple of days. Using a Python interface is much easier to get started with than dealing with all of the low-level details of C++. It did take some more effort to figure out how to structure the client since we did not write everything from scratch.

### Features
We implemented **create topic**, **subscribe**, **unsubscribe**, **publish**, and **unit tests**. All request processing is handled by Google's API and error outputs are logged to the terminal. Documentation for the Google API and error messages can be found at [https://googlecloudplatform.github.io/google-cloud-python/latest/pubsub/index.html](https://googlecloudplatform.github.io/google-cloud-python/latest/pubsub/index.html)


### Design Differences

The biggest difference between Google's PubSub and the from-scratch implementation we made was the ease of development. With Google's PubSub system, all we needed to worry about was programming the actual functions, this is not the case with Linux RPC's. With RPC's There were many steps in getting the client and server to communicate; most of the work for this project was the network programming, not the design of the system features. With Google's PubSub, this was all taken care of for us, all we needed to worry about was designing the system features.
