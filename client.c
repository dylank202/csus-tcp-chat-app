#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

#define PORT "12345"
#define BUFFER_SIZE 1024




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
 * Function: initialize_connection
 * -------------------------------
 * Establishes a connection to the chat server.
 *
 * server_hostname: a string representing the hostname or IP address of the server.
 * port: a string representing the port number on which the server is listening.
 *
 * returns: a socket file descriptor for the established connection, or -1 if an error occurred.
 *
 * Task: Complete this function to resolve the given hostname and port to an address
 * and establish a TCP connection. Use getaddrinfo to obtain the server's address and
 * connect to the first address you can.
 */
int initialize_connection(const char *server_hostname, const char *port) {
    struct addrinfo* res;
    int sockfd;
    struct sockaddr their_addr;

    if ((getaddrinfo(server_hostname, port, NULL, &res)) == -1) { 
        perror("getaddrinfo");
        exit(1);   
    }

    if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    their_addr = *(res->ai_addr);

    if (connect(sockfd, &their_addr, sizeof their_addr) == -1) {
        perror("connect");
        exit(1);
    }


    return sockfd;
}


/**
 * Function: handle_server_message
 * -------------------------------
 * Reads and processes messages received from the server.
 *
 * sock: the socket file descriptor connected to the server.
 *
 * Task: Implement this function to continuously listen for messages from the server
 * and print them to stdout. This function will be called whenever the select mechanism
 * indicates that the server socket has data to be read. Ensure the message is null-terminated
 * before printing.
 */
void handle_server_message(int sock) {
    int numbytes;
    char msg[BUFFER_SIZE];


    if ((numbytes = recv(sock, msg, BUFFER_SIZE - 1, 0)) == -1) {
        perror("recv");
        close(sock);
        exit(1);
    }
    msg[numbytes] = '\0';
    if (numbytes == 0) {
        fprintf(stdout, "Remote connection closed.\n");
        close(sock);
        exit(0);
    } else {
        fprintf(stdout, "%s\n", msg);
    }
}

/**
 * Function: handle_user_input
 * ---------------------------
 * Sends a user's message to the server.
 *
 * sock: the socket file descriptor connected to the server.
 * username: the user's username, which should be prepended to the message.
 *
 * Task: Implement this function to read a line of input from stdin, prepend the username
 * to it, and send it to the server. This function will be called whenever the select mechanism
 * indicates that there is user input to be read from stdin. Ensure proper formatting of the message
 * for readability by other clients.
 */
void handle_user_input(int sock, const char *username) {
    char msg[BUFFER_SIZE];
    char finalmsg[BUFFER_SIZE];     // message to send out
    int len;
    fgets(msg, sizeof(msg), stdin);
    msg[strcspn(msg, "\n")] = '\0';

    strcpy(finalmsg, username);
    strcat(finalmsg, ": ");
    strcat(finalmsg, msg);

    len = strlen(finalmsg);

    //if (send(sock, finalmsg, len, 

    if (sendall(sock, finalmsg, &len) == -1) {
        perror("sendall");
        printf("Sent %d bytes.\n", len);
        printf("%d", errno);
    }

}

// The main function and any additional helper functions or definitions as necessary.


int main(int argc, char *argv[]) {

    fd_set read_fds;

        // Step 1: Validate command-line arguments.
    // TODO: Ensure there are exactly three arguments: the executable name, the server's hostname, and the username.

    if (argc != 3) {
        fprintf(stderr, "usage: server hostname, username\n");
        exit(1);
    }

    // TODO: Call initialize_connection with the server's hostname and port number to establish a connection.

    // socket file descriptor
    int sfd = initialize_connection(argv[1], PORT);
    if (sfd == -1) {
        printf("Connection initialization failed.\n");
        exit(1);
    } else {
        printf("Connection initialization successful.\n");
    }

    // Step 3: Main loop - Use select() to multiplex between stdin (user input) and the socket (incoming messages).
    while (1) {

        // TODO: Inside the loop, do the following:

        // a. Clear and set up the readfds set for select() with both stdin and the server socket.
        FD_ZERO(&read_fds);
        FD_SET(sfd, &read_fds);
        FD_SET(0, &read_fds);

        // b. Call select() to wait for activity on either stdin or the socket.
        if (select(sfd+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(1);
        }

        // c. If select indicates activity on the server socket, call handle_server_message to process it.
        if (FD_ISSET(sfd, &read_fds)) {
            handle_server_message(sfd); 
        } 


        // d. If select indicates activity on stdin, call handle_user_input to read the user's message and send it to the server.
        if (FD_ISSET(0, &read_fds)) {
            handle_user_input(sfd, argv[2]);
        }

    }


    // Step 4: Clean up and close the socket before exiting.
    // TODO: Properly close the socket descriptor when the program is terminating.

    close(sfd);

    return 0; // Exit the program.
}