#include "server.hpp"	/*Include the definition of the Server class so that an instance can be created within this code*/
#include "request.hpp"	/*Include request classes for parsing HTTP requests and enable creation of instances*/
#include <iostream>	/*Enable standard input/output and output error logs on the console*/
#include <sys/socket.h>	/*Enables creation of communication sockets and acceptance of connections*/	
#include <netinet/in.h>	/*Enable network-related structures*/
#include <unistd.h>	/*Allows manipulation of file descriptors*/
#include <cstring>	/*Allows manipulation of buffers used in communication with C lang style strings*/
#include <fstream>	/*Read the contents of a file and makes it available to the client*/
#include <unordered_map> /*Enable storing MIME types (correspondence between file extension and Content-Type)*/
#include <sstream>	/*Enable buffer for strings when reading static files*/

//Mapping of MIME type
std::unordered_map<std::string, std::string> mime_types = {
	{".html", "text/html"},
	{".css", "text/css"},
	{".js", "application/javascript"},
	{".png", "image/png"},
	{".jpg", "image/jpeg"},
	{".jpeg", "image/jpeg"},
	{".gif", "image/gif"},
	{".ico", "image/x-icon"},
	{".txt", "text/plain"},
	{".json", "application/json"}
};

std::string get_mime_type(const std::string& path) {
	auto ext_pos = path.find_last_of(".");
	if (ext_pos != std::string::npos) {
		std::string ext = path.substr(ext_pos);
		if (mime_types.count(ext)) {
			return mime_types[ext];
		}
	}
	return "application/octet-stream";
}

//File serve function
void serve_static_file(const std::string& path, int client_socket) {
	std::ifstream file(path, std::ios::binary);
	if (!file.is_open()) {
		//response 404 when file not found
		std::cerr << "File not found: " << path << std::endl;
		std::string response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
		send(client_socket, response.c_str(), response.size(), 0);
		return;
	}

	//Read contents in file
	std::stringstream buffer;
	buffer << file.rdbuf();
	std::string body = buffer.str();

	//Get MIME type
	std::string mime_type = get_mime_type(path);

	//Prepare the response
	std::string response = "HTTP/1.1 200 OK\r\nContent-Type: " + mime_type + "\r\nContent-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;

	//Send to client
	ssize_t sent = send(client_socket, response.c_str(), response.size(), 0);
	if (sent < 0) {
		perror("send failed");
		close(client_socket);
		return;
	}
}

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
	    std::string uri = request.get_uri();

	    if (uri.find("..") != std::string::npos) {
		    std::cerr << "Invalid path detected: " << uri << std::endl;
		    std::string response = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
		    send(new_socket, response.c_str(), response.size(), 0);
		    continue;
	    }

	    //default file
	    if (uri == "/") {
		    uri = "/index.html";
	    }

	    //Create file path
	    std::string file_path = "public" + uri;
	    std::cout << "Serving file: " << file_path << std::endl;

	    //Serve static file
	    serve_static_file(file_path, new_socket);

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
        }

        close(new_socket);
    }
}
