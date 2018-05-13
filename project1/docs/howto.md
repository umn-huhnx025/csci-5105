# Our Publish-Subscribe Implementation

## How to Run the Server

```
$ make && ./pubsub_server
```

## Server Commands

#### `get_list`

Get a list of group servers from the registry server.


## How to Run the Client

Our Makefile compiles both the client and the server, so there's no need to `make` again to run the client.

```
$ ./pubsub_client <group_server>
```

`<group_server>` can be either an IP address or hostname.

## Client Commands

The client will automatically `Join` the server specified on the command line. Our client runs as a very basic shell. Valid commands include the following:

#### `publish <Article>`

Publish the given article.

e.g. `pubsub> publish Sports;;;content`

#### `subscribe <Subscription>`

Subscribe to articles matching the given subscription.

e.g. `pubsub> subscribe Technology;;UMN;`

#### `unsubscribe <Subscription>`

Unsubscribe from articles matching the given subscription. This fails if the argument does not exactly match a current subscription for the client.

e.g. `pubsub> unsubscribe Technology;;UMN;`

#### `q`

Quit the client. The client will automatically `Leave` once it exits.

#### Other Notes

Successful commands produce no output. Error messages are shown on failure.

New articles look like the following:

```
*** New article: "Sports;;;content"
```

The client cannot explicitly `Ping` the group server. It spawns a new thread to `Ping` every 5 seconds in the background, and exits if no message is received from the group server.


# Google PubSub Client

## How to Run the Client
Instructions for running the client can be found [here](../google/README.md).

Our Google PubSub client works similarly to the client in our implementation. The project ID can be defined as an argument on the command line or in the `PUBSUB_PROJECT_ID` environment variable. If neither is present, it defaults to `csci-5105-project-1`. Our client spawns a basic shell with the following commands:

### Client Commands

Once the program is running, any of the following commands can be executed.

- **Create topic**: To create a new topic on the server type  `create_topic <topic name>`
- **Subscribe**: To subscribe to a topic type `subscribe <topic name>` Note you can only subscribe to topics that have been created. The client will  create a new thread to read in incoming messages.
- **Unsubscribe**: To unsubscribe type  `subscribe <topic name>` Note you can only subscribe to topics that have been created.
- **Publish**: To publish an article type `subscribe <topic name> <content>`Note you can only publish an article with a topic that has been created. The article will propagate to everyone subscribed and
will be acknowledged when processing of the message has been completed. This will tell the server that it does not need to be seen again.
- **Unit tests**:  Details will be provided in [tests.md](tests.md).

## Client Unit Tests

Run unit tests for the client with:

```
$ python client.py test
```

 More details on tests are provided [here](tests.md).
