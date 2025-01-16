#include "server.hpp"
#include "request.hpp"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

Server::Server(int port) : port(port) {}

void Server::run() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    //Create the communication socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
       perror("socket failed");
       exit(EXIT_FAILURE);
    }

    //Configure socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr= INADDR_ANY;
    address.sin_port = htons(port);

    //Bind the socket to the port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    //Start listening for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    std::cout << "Listening on port " << port << "..." << std::endl;
    
    //Send a simple HTTP response
    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("accept failed");
            exit(EXIT_FAILURE);
        }

        char buffer[4096] = {0};
        ssize_t bytes_read = read(new_socket, buffer, 4096);
        if (bytes_read <= 0) {
            close(new_socket);
            continue;
        }

        std::string raw_request(buffer, bytes_read);
        Request request(raw_request);
        request.parse();

        //Method-by-Method Processing
        if (request.get_method() == "GET") {
	    std::string body = "Hello, GET!";
            std::string response = "HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;

	    ssize_t total_sent = 0;
	    ssize_t bytes_to_send = response.size();

	    while (total_sent < bytes_to_send) {
		    ssize_t sent = send(new_socket, response.c_str() + total_sent, bytes_to_send - total_sent, 0);
		    if (sent < 0) {
			    perror("send failed");
			    break;
		    }
		    total_sent += sent;
	    }
	    if (total_sent == bytes_to_send) {
		    std::cout << "Response fully sent (" << total_sent << "bytes)." << std::endl;
	    } else {
		    std::cerr << "Response partially sent (" << total_sent << "/" << bytes_to_send << "bytes)." << std::endl;
	    }
        } else if (request.get_method() == "POST") {
            std::string response_body = "Received: " + request.get_body();
            std::string response = "HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(response_body.size()) + "\r\n\r\n" + response_body;
            send(new_socket, response.c_str(), response.size(), 0);
        } else {
	    std::cerr << "Unsupported method: " << request.get_method() << std::endl;

            std::string response = "HTTP/1.1 405 Method Not Allowed\r\nContent-Length: 0\r\n\r\n";
	    ssize_t sent = send(new_socket, response.c_str(), response.size(), 0);
	    if (sent < 0) {
		    perror("send failed");
	    } else {
		    std::cout << "405 Method Not Allowed response sent." << std::endl;
	    }
            send(new_socket, response.c_str(), response.size(), 0);
        }

        close(new_socket);
    }
}
