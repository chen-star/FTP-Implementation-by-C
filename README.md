# FTP Realization

## Intro to FTP
**FTP**: File Transfer Protocol. An important protocol that provides computers with the ability to access files on remote machines.

- **FTP** Client:
 * Users interact with Client directly * Active open of control connection * Control connection uses ASCII plain-text * Send commands (over control connection) * Receive replies (over control connection) * Data connection used to transfer file data- **FTP** Server
 * System process * “Listens” for connection on well-known port 21 * Receive commands * Send replies

- **FTP** Proxy

 * Perform as both server and client
 * Receive requests from FTP client and then forward the requests to FTP server
 * After that, receive replies from server and forward the replies to client
 * Can also resolve the commands and modify if necessary
 * Cache mechanism
 * Handle error commands
