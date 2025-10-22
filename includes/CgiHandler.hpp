#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include <string>
#include <map>
#include "HttpResponse.hpp"
#include "HttpRequest.hpp"

class CgiHandler 
{
public:
    CgiHandler();
    ~CgiHandler();

    CgiHandler(HttpResponse &res) : response(res) {}
    void ExecuteCgi (const std::string& scriptName, std::string pathCgi, const HttpRequest& request);
    
private:
    HttpResponse &response;
    void parseCgiOutput(const std::string& output);
};




#endif