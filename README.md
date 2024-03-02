**Name: Ionescu Matei-Ștefan**  
**Group: 333CAb**

# APD Homework #3 - BitTorrent

This program implements the BitTorrent protocol for peer-to-peer file sharing.
It will be using MPI to communicate between computers in the network.

## Peer
The peers are nodes in the decentralized network that are uploading and
downloading files. Thus, when creating a **Peer** class instance and calling
the `start()` function, two threads are created, one for managing uploads and
one for downloads. The `init()` function is also called in the `start()`
function to read the files owned by the peer and the files wanted by them.

#### Download Thread
The download thread function has two roles: communcating with the tracker and
downloading files. When the function starts it will notfy the tracker about
what files the peer owns and then it will wait for the tracker to send a
message back telling the peer that all the other peers sent their owned files
and downloads can be started. Then for each file the peer wants, a request will
be sent to the tracker and the tracker will send back a list of peers that own
segments from the wanted file. For each segment from the wanted file, a random
peer will be selected and a request for that segment will be sent to the
selected peer. After 10 segments downloaded, the peer will notify the tracker
that he has those segments and he is able to upload them to other peers. After
all segments from a file are downloaded, the peer will also notify the tracker.

#### Upload Thread
In the download thread, the client receives requests for files from other peers
and simulates sending the file back by sending an OK message.

## Tracker
The tracker is the node with the rank 0 in the network. It doesn't send files,
but acts as an intermediary connecting peers that want files to the ones which
own them. When the tracker starts, it will wait for all peers to send their
owned files and then send an OK message to notify them that they can start
downloading files. The tracker can accept 4 types of messages from peers:
- **file request** - When a **file request** is received, the tracker will
send to the peer the list of segments that the file is split into and for each
segment the peers that own it.

- **peer update** - A **peer update** is a message from a peer notifying the
tracker that it downloaded 10 segments. The tracker will add the peer to the
lists of owners for those segments.
	
- **download completed** - A peer will send a **download completed** message
when it has download all segments of a file. The tracker will upldate the lists
of owers for all segments of that file.

- **all downloads completed** - When an **all downloads completed** message is
sent to the tracker, the tracker will update the number of peers that are
downloading files.

When all peers completed their downloads, the tracker will send a poison value
to the upload thread of each peer, notifying them that there will be no more
uploads and they can end their execution.


## Notes
- In order to vary the peers from which each segment is downloaded as much as
possible, the peer for each segment is selected randomly. This is an efficient
method of balancing the upload load of each peer.
- The program passed all the tests on the checker, getting a score of 40/40p
