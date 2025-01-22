#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <errno.h>

#define PORT 12345
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 30


int sendall(int s, char *buf, int *len) {
    int total = 0, bytesleft = *len, n;

    while (total < *len) {
        n = send(s, buf + total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total;

    return n == -1 ? -1 : 0;
}

/**
 * Function: initialize_server
 * ----------------------------
 * Initializes the server to accept client connections.
 *
 * returns: The socket file descriptor of the listening server socket, or -1 if an error occurred.
 *
 * Task: Implement this function to create a server socket that listens on PORT. This involves
 * creating a socket, setting it to allow multiple connections, binding it to the correct port,
 * and starting to listen for client connections.
 */
int initialize_server() {
    int sockfd;
    struct sockaddr_in my_addr;
    int yes = 1;

    if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return -1;
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        return -1;
    }

    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(PORT);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    memset(my_addr.sin_zero, '\0', sizeof my_addr.sin_zero);

    if (bind(sockfd, (struct sockaddr*) &my_addr, sizeof my_addr) == -1) {
        perror("bind");
        return -1;
    }
    if (listen(sockfd, MAX_CLIENTS) == -1) {
        perror("listen");
        return -1;
    }


    return sockfd;
}

/**
 * Function: accept_new_connection
 * -------------------------------
 * Accepts a new client connection.
 *
 * master_socket: The socket file descriptor of the server's listening socket.
 * client_socket: Array of client sockets.
 * max_clients: Maximum number of clients that can connect to the server.
 *
 * Task: Complete this function to accept a new client connection, add the new client socket
 * to the client_socket array, and print out the connection details. Make sure to handle the
 * case where the server is full and cannot accept more clients.
 */
void accept_new_connection(int master_socket, int client_socket[], int max_clients) {
    int full = 0;
    if (client_socket[max_clients - 1] != 0) {
        full = 1;
    }

    if (full) {
        struct sockaddr_in their_addr1;

        socklen_t addrlen1;

        addrlen1 = sizeof their_addr1;
        int tempsock = 0;

        if ((tempsock = accept(master_socket, (struct sockaddr*) &their_addr1,
                &addrlen1)) == -1) {
            perror("accept");
        } else {

            char msg[115] = "Server: new connection from ";
            strcat(msg, inet_ntoa(their_addr1.sin_addr));
            strcat(msg, " attempted to join, but the server was full\0");
            int len = strlen(msg);
            int j = 0;
            for (j = 0; j < max_clients; j++) {
                if (client_socket[j] != 0) {
                    if (sendall(client_socket[j], msg, &len) == -1) {
                        perror("sendall");
                    }
                }
            }
            printf("Server: new connection from %s attempted to join, but the server was full\n",
                    inet_ntoa(their_addr1.sin_addr));
        }

        char msg[100] = "Connection refused: server full.\0";
        int len = strlen(msg);
        if (sendall(tempsock, msg, &len) == -1) {
            perror("sendall");
            printf("Sent %d bytes.\n", len);
        }

        close(tempsock);

        return;

    }

    int maxclient = 0;
    while (maxclient < max_clients) {
        if (client_socket[maxclient] != 0) {
            maxclient++;
        } else {
            break;
        }
    }

    client_socket[maxclient] = master_socket;

    struct sockaddr_in their_addr;
    socklen_t addrlen;

    addrlen = sizeof their_addr;
    if ((client_socket[maxclient] = accept(master_socket, (struct sockaddr*) &their_addr,
            &addrlen)) == -1) {
        perror("accept");
    } else {

        char msg[115] = "Server: new connection from ";
        char buffer[5] = "";
        strcat(msg, inet_ntoa(their_addr.sin_addr));
        strcat(msg, " on socket ");
        snprintf(buffer, 5, "%d",client_socket[maxclient]);
        strcat(msg, buffer);
        //strcat(msg, itoa(client_socket[maxclient]));
        int len = strlen(msg);
        int j = 0;
        for (j = 0; j < maxclient; j++) {
            if (client_socket[j] != 0) {
                if (sendall(client_socket[j], msg, &len) == -1) {
                    perror("sendall");
                }
            }
        }

        printf("Server: new connection from %s on socket %d\n",
                inet_ntoa(their_addr.sin_addr), client_socket[maxclient]);
    }
}



/**
 * Function: send_to_all_other_clients
 * -----------------------------------
 * Sends a message to all clients except the sender.
 *
 * sender_sock: Socket descriptor of the client that sent the message.
 * client_sockets: Array of client sockets.
 * max_clients: Maximum number of clients.
 * msg: Message to be broadcast.
 *
 * Task: Write this function to loop through all connected clients and send the message
 * to each client except the sender. Ensure that the message is properly formatted and
 * sent correctly.
 */
void send_to_all_other_clients(int sender_sock, int client_sockets[], int max_clients, char *msg) {
    int j = 0;
    int len = strlen(msg);

    int maxclient = 0;
    while (maxclient < max_clients) {
        if (client_sockets[maxclient] != 0) {
            maxclient++;
        } else {
            break;
        }
    }


    for (j = 0; j < maxclient; j++) {
        if (client_sockets[j] != sender_sock && client_sockets[j] != 0) {
            if (sendall(client_sockets[j], msg, &len) == -1) {
                perror("sendall");
            }
        }
    }

}


/**
 * Function: handle_client_activity
 * --------------------------------
 * Handles activity from an existing client connection.
 *
 * client_socket: Array of client sockets.
 * read_fds: The set of socket descriptors ready to be read.
 * max_clients: Maximum number of clients that can connect to the server.
 *
 * Task: Implement this function to read a message from a client socket indicated as ready in
 * read_fds, and broadcast that message to all other connected clients. If a client has
 * disconnected, close the socket and remove it from the client_socket array.
 */
void handle_client_activity(int client_socket[], fd_set read_fds, int max_clients) {
    int nbytes;
    int i = 0, target = 0;
    char buf[BUFFER_SIZE];


    while (i < max_clients) {
        if (FD_ISSET(client_socket[i], &read_fds)) {
            target = i;
            break;
        }
        i++;
    }
    // TODO: socket i, or socket client_socket[i]? same as in accpt_new_cnnctn
    if ((nbytes = recv(client_socket[target], buf, sizeof buf, 0)) <= 0) {
        if (nbytes == 0) {

            char msg[115] = "Server: socket ";
            char buffer[5] = "";
            snprintf(buffer, 5, "%d",client_socket[target]);
            strcat(msg, buffer);
            strcat(msg, " closed connection\0");
            int len = strlen(msg);
            int j = 0;
            for (j = 0; j < max_clients; j++) {
                if (client_socket[j] != 0 && client_socket[j] != client_socket[target]) {
                    if (sendall(client_socket[j], msg, &len) == -1) {
                        perror("sendall");
                    }
                }
            }

            printf("Server: socket %d closed connection\n", client_socket[target]);
        } else {
            perror("recv");
        }
        close(client_socket[target]);
        FD_CLR(client_socket[target], &read_fds);

        for (i = target; i < max_clients; i++) {
            if (i < max_clients - 1) {
                client_socket[i] = client_socket[i + 1];
            } else {
                client_socket[i] = 0;
            }
            if (client_socket[i] == 0) {
                break;
            }
        }

    } else {
        buf[nbytes] = '\0';
        send_to_all_other_clients(client_socket[target], client_socket, max_clients, buf);
        printf("%s\n", buf);

    }

}

// Additional helper functions or definitions as necessary.


int main() {
    int master_socket, client_socket[MAX_CLIENTS], max_sd;
    fd_set readfds;

    // Step 1: Initialize the server to start listening for connections.
    // TODO: Call initialize_server and check for errors.

    if ((master_socket = initialize_server()) == -1) {
        perror("initialize_server");
        exit(1);
    } else {
        printf("Server initialization successful.\n");
    }

    // Initialize client_socket array to 0.
    // TODO: Zero out the client_socket array to indicate no clients are connected initially.

    int i = 0;
    while (i < MAX_CLIENTS) {
        client_socket[i] = 0;
        i++;
    }

    max_sd = master_socket;

    // Main server loop.
    while (1) {
        // Step 2: Clear and setup file descriptor sets.
        // TODO: Use FD_ZERO and FD_SET on readfds and add master_socket and all client sockets.
        FD_ZERO(&readfds);
        FD_SET(master_socket, &readfds);

        max_sd = master_socket;
        i = 0;
        while (i < MAX_CLIENTS) {
            FD_SET(client_socket[i], &readfds);
            if (client_socket[i] > max_sd) {
                max_sd = client_socket[i];
            }
            i++;
        }


        // Step 3: Wait for activity on any socket using select.
        // TODO: Use select to wait for activity and check for errors or interruptions.
        if (select(max_sd+1, &readfds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(1);
        }


        // Step 4: Check for new connections on the master_socket.
        // TODO: If there's activity on master_socket, call accept_new_connection.
        if (FD_ISSET(master_socket, &readfds)) {
            accept_new_connection(master_socket, client_socket, MAX_CLIENTS);
        }

        // Step 5: Handle IO activity from clients.
        // TODO: Loop through client sockets, checking each for activity. If any, call handle_client_activity.
        i = 0;
        while (i < MAX_CLIENTS) {
            if (FD_ISSET(client_socket[i], &readfds) && client_socket[i] != 0) {
                handle_client_activity(client_socket, readfds, MAX_CLIENTS);
            }
            i++;
        }

        // Step 6: Clean up disconnected clients.
        // TODO: Inside handle_client_activity, ensure proper cleanup of disconnected client sockets.
    }

    // Clean up before shutting down the server.
    // TODO: Close all active client sockets and the master socket before exiting.
    close(master_socket);
    i = 0;
    while (i < MAX_CLIENTS) {
        close(client_socket[i]);
        i++;
    }

    return 0;
}