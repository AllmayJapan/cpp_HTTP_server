#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <unordered_map>

class Request {
    public:
        //Constructor
        Request(const std::string& raw_request);

        //Parse method
        void parse();

        //Get method, URI, body
        std::string get_method() const;
        std::string get_uri() const;
        std::string get_body() const;
        
    private:
        std::string raw_request;
        std::string method;
        std::string uri;
        std::string http_version;
        std::unordered_map<std::string, std::string> headers;
        std::string body;
};

#endif // REQUEST_HPP
