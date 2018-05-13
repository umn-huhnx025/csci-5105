program PUBSUB_PROG {
  version PUBSUB_VERSION {
    bool JoinServer(string IP, int ProgID, int ProgVers) = 1;
    bool LeaveServer(string IP, int ProgID, int ProgVers) = 2;
    bool Join(string IP, int Port) = 3;
    bool Leave(string IP, int Port) = 4;
    bool Subscribe(string IP, int Port, string Article) = 5;
    bool Unsubscribe(string IP, int Port, string Article) = 6;
    bool Publish(string Article, string IP, int Port) = 7;
    bool PublishServer(string Article, string IP, int Port) = 8;
    bool Ping() = 9;
  } = 1;
} = 0x42424242;