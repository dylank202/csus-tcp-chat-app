# csus-tcp-chat-app

## Console chat application utilizing C sockets and low-level TCP networking functions to manage multiple concurrent client-server interactions at once.

- Initially given only function headers and instructional comments - all functional code had to be written.
- Able to connect multiple users to one server and display, in real time, each user's message to all other users.
- Displays when a new user connects and when any user disconnects, sending a server message to all users.
- Tested and able to be used between multiple devices, and on multiple operating systems.
- Handles too many users connecting by rejecting new connections once the server is full (max size set manually).
