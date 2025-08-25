#ifndef HTTP_HANDLER_H
#define HTTP_HANDLER_H

#include <string>
#include <map>
#include <vector>

struct HttpRequest {
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;
};

struct HttpResponse {
    int status_code;
    std::string status_text;
    std::map<std::string, std::string> headers;
    std::string body;
};

class HttpHandler {
public:
    HttpHandler();
    
    HttpRequest parse_request(const std::string& raw_request);
    std::string build_response(const HttpResponse& response);
    
    HttpResponse handle_upload(const HttpRequest& request);
    HttpResponse handle_download(const HttpRequest& request);
    HttpResponse handle_list_files(const HttpRequest& request);
    HttpResponse handle_delete_file(const HttpRequest& request);
    
    static std::string get_mime_type(const std::string& filename);
    static std::string url_encode(const std::string& str);
    static std::string url_decode(const std::string& encoded);
    
private:
    std::vector<std::string> parse_headers(const std::string& header_text);
    std::pair<std::string, std::string> parse_header_line(const std::string& line);
};

#endif // HTTP_HANDLER_H 