/* parser.cc - Parser 
 * 
 * utilities to parse a request and populate request and response objects
 * 
 * This is not a complete file for code sharing concerns. Some parts are
 * omitted.
 *
 */


#include "parser.hh"
#include "misc.hh"

// for debug
#include<stdio.h>

/*
 * Constructor
 */
Parser::Parser(
	/* a_request: 'a' request to populate */
	HttpRequest * const a_request, 

	/* a_response: 'a' response to populate */
	HttpResponse * const a_response) : request(a_request), response(a_response)
	{
	}


/*
 * parse a raw http body sent from client
 * 
 * This method would populate request and response objects based on contents
 * of the http message coming from sock
 * Of course, response is not completely ready, but some sensible values are
 * populated in the response, which avoids later processing
 *
 * Return: whether parsing was successfull
 */
bool Parser::parse_request(
	const Socket_t& sock	/* a socket interface, an HTTPS socket is also valid */
)
{
	/* read request line from socket */
	std::string request_line = sock->readline();
	std::string line;

	std::string method;
	std::string uri;
	std::string ver;
	size_t space_index = std::string::npos;
	size_t space_index2 = std::string::npos;
	std::string temp1;
	std::string temp2;

	size_t colon_position = std::string::npos;
	std::string prev_field_name;
	std::string prev_field_value;

	/* 
	* whether there was a http header field before. 
	* This is used to check while unfolding headers 
	*/
	bool prev_header_exists = false;

	request_line = trim(request_line); /* pass by reference */

	space_index = request_line.find_first_of(' ');

	if(space_index == std::string::npos) {
		response->status_code = 400;
		return false;
	}
	
	method = request_line.substr(0, space_index);
	request->method = method;
	
	space_index2 = request_line.find_first_of(' ', space_index + 1);
	
	/* 
	 * there is no more space in this line, so invalid request line as there 
	 * are still things left
	 */
	if(space_index2 == std::string::npos)
	{
		response->status_code = 400;
		return false;
	}

	uri = request_line.substr(space_index + 1, space_index2 - space_index - 1);
	if(uri.empty())
	{
		response->status_code = 400;
		return false;
	}

	request->request_uri = uri;
	ver = request_line.substr(space_index2 + 1, std::string::npos);
	ver = trim(ver);
	if(ver.empty())
	{
		response->status_code = 400;
		return false;
	}

	ver.pop_back();
	ver.pop_back();

	request->http_version = ver;

	space_index = std::string::npos;
	space_index2 = std::string::npos;

	while(1)
	{
		line = sock->readline();
		line.pop_back();
		line.pop_back();

		if(line.empty())
		{
			break;
		}

		if(line[0] == '\t' || line[0] == ' ')
		{
			if(prev_header_exists)
			{
				line = ltrim(line);
				request->headers[prev_field_name] = prev_field_value + line;
			}
		}

		else
		{
			colon_position = line.find_first_of(':');
			if(colon_position == std::string::npos)
			{
				// updating response code with syntax error
				response->status_code = 400;
				return false; 
			}

			temp1 = line.substr(0, colon_position);
			temp2 = line.substr(colon_position + 1, std::string::npos);	/* copy everything till end of string */
			temp1 = trim(temp1);
			temp2 = trim(temp2);
			request->headers[temp1] = temp2;
			
		}
	}
	
	if( (request->headers["Authorization"]).empty() )
	{
		response->status_code = 401;	// unauthorized
		response->headers["WWW-Authenticate"] = "Basic realm=\"realm_of_cs252\"";
		return false;
	}

	else
	{
		temp1 = request->headers["Authorization"];

		/* 
		 * copy part after first word and space from the field of the header. 
		 * This part contains the credentials in base64 
		 */
		temp1 = temp1.substr(temp1.find_first_of(' ') + 1, std::string::npos);
		temp2 = std::string("a2phc2FuaTpiYW5hbmE=");
		if(temp1.compare(temp2) != 0)
		{
			response->status_code = 401;	// unauthorized 
			response->headers["WWW-Authenticate"] = "Basic realm=\"realm_of_cs252\"";
			return false;
		}
	}
	return true;
}
