#include "request.hpp"
#include <sstream>
#include <iostream>

Request::Request(const std::string& raw_request) : raw_request(raw_request) {}

void Request::parse() {
    std::istringstream stream(raw_request);
    std::string line;

    //Analyze stringline
    std::getline(stream, line);
    std::istringstream request_line(line);
    request_line >> method >> uri >> http_version;

    //Analyze header
    while (std::getline(stream, line) && line != "\r") {
        auto colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = line.substr(0, colon_pos);
            std::string value = line.substr(colon_pos + 1);
            headers[key] = value;
        }
    }

    //Analyze body
    if (method == "POST" && headers.find("Content-Length") != headers.end()) {
        int content_length = std::stoi(headers["Content-Length"]);
        body.resize(content_length);
        stream.read(&body[0], content_length);
    }
}

std::string Request::get_method() const {
    return method;
}

std::string Request::get_uri() const {
    return uri;
}

std::string Request::get_body() const {
    return body;
}
