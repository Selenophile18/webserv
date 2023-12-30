#include "Request.hpp"
# include <cstdio>
/*  constructors destructors  */
Request::Request() {
	_size_read = 0;
	_chunk_read = 0;
	_chunks_size = 0;
	_status_code = 0;
	_request_headers = "";
	_remaining = NULL;
	_remaining_size = 0;
	_file.open("original.mp4", std::ios::out | std::ios::app);
	_headers_read = false;
	_end_of_request = false;
}

Request::~Request() {
	_file.close();
}

/*  setters  */
/*	getters	*/
std::map<std::string, std::string>	Request::get_headers() const {
	return (_headers);
}
const request_line	&Request::get_request_line() const {
	return (_request_line);
}
long	Request::get_size_read() const {
	return (_size_read);
}
std::fstream	&Request::get_file() {
	return (_file);
}
bool	Request::get_end_of_request() const {
	return (_end_of_request);
}
unsigned int	Request::get_status_code() const {
	return (_status_code);
}

/*	additional functions	*/
bool	Request::is_req_well_formed(void) {
	// Check if Transfer-Encoding header exist and is different to “chunked”.
	if (_headers.find("Transfer-Encoding") != _headers.end() && _headers["Transfer-Encoding"] != "chunked") {
		_status_code = 501;
		return (false);
	}
	// Check if Transfer-Encoding not exist, Content-Length not exist and the method is Post
	if (_headers.find("Transfer-Encoding") == _headers.end() && _headers.find("Content-Length") == _headers.end() && _request_line.method == "POST") {
		_status_code = 400;
		return (false);
	}
	// Check the request uri contain a character not allowed.
	if (_request_line.uri.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 ._~:/?#[]@!$&'()*+,;=%") != std::string::npos) {
		_status_code = 400;
		return (false);
	}
	// Check if the request uri contain more the 2048 chars.
	if (_request_line.uri.length() > 2048) {
		_status_code = 414;
		return (false);
	}
	return (true);
}
void	Request::split_request(char *buffer, ssize_t bytesread) {
	ssize_t	i = 0;
	while (i < bytesread && !_headers_read)
	{
		if (buffer[i] == '\r')
			if (buffer[i + 1] == '\n')
				if (buffer[i + 2] == '\r')
					if (buffer[i + 3] == '\n')
					{
						_headers_read = true;
						this->parse_request();
						// parse for error codes
						if (!is_req_well_formed()) {
							// std::cout << "debug: request not well formed" << std::endl;
							_end_of_request = true;
							return ;
						}
						i += 4;
						break ;
					}
		_request_headers += buffer[i];
		i++;
	}
	if (!_file.good())
		std::cout << "debug: file not open!" << std::endl;
	
	// Check if the request is chunked.
	if (get_headers().find("Transfer-Encoding") != get_headers().end() && get_headers()["Transfer-Encoding"] == "chunked") {
		if (!_chunks_size) {
			// Check if there is a remaining from the previous buffer.
			if (_remaining)	{
				// Create a temporaty tmp to store the buffer.
				char *tmp = new char[bytesread - i];
				// Copy the buffer into tmp.
 				ssize_t	j = 0;
				ssize_t k = 0;
				while (j < bytesread - i) {
					tmp[j] = buffer[i + j];
					j++;
				}
				// Create a new buffer with the size of the remaining + the size of the tmp.
				buffer = new char[_remaining_size + bytesread - i];
				// Copy the remaining into the new buffer.
				j = 0;
				while (j < _remaining_size) {
					buffer[j] = _remaining[j];
					j++;
				}
				// Copy the tmp into the new buffer.
				while (k < bytesread - i) {
					buffer[j] = tmp[k];
					j++;
					k++;
				}
				// Update the bytesread.
				bytesread += _remaining_size;
				// Update the index.
				i = 0;
				// Skip the \r\n Before the chunck size.
				if (buffer[i] == '\r' && buffer[i + 1] == '\n') {
					i += 2;
				}
				// Check for the end of the request.
				if (_remaining[i] == '0' && _remaining[i + 1] == '\r' && _remaining[i + 2] == '\n') {
					_end_of_request = true;
					return ;
				}
				// Delete the allocated buffers.
				delete [] tmp;
				delete [] _remaining;
				_remaining = NULL;
			}
			// Allocate for line aka the buffer holder of the size of the next chunk.
			ssize_t k = i;
			while (k < bytesread) {
				if (buffer[k] == '\r' && buffer[k + 1] == '\n') {
					break;
				}
				k++;
			}
			ssize_t line_size = k - i;
			char *line = new char[line_size + 1];
			bzero(line, line_size + 1);
			k = 0;
			// Copy the next chunck size into line.
			while (k < line_size + 1) {
				if (buffer[i] == '\r' && buffer[i + 1] == '\n') {
					i += 2;
					break;
				}
				line[k] = buffer[i];
				i++;
				k++;
			}
			// Convert the size of the next chunck from hex to decimal.
			std::stringstream hex;
			hex << std::hex << line;
			hex >> _chunks_size;
		}
		// Put the chunk into the file.
		while (_chunk_read < _chunks_size && i < bytesread) {
				_file.put(buffer[i]);
				_file.flush();
				_chunk_read++;
				_size_read++;
				i++;
		}
		// Check if the chunk is complete and be ready to the next one.
		if (_chunk_read  == _chunks_size && i < bytesread) {
			// reset the variables for the next chunk.
			_chunk_read = 0;
			_chunks_size = 0;
			// Allocate for a buffer which will hold the remaining of the buffer.
			_remaining = new char[bytesread - i];
			_remaining_size = bytesread - i;
			int j = 0;
			// Copy the remaining into the buffer _remaining.
			while(i < bytesread) {
				_remaining[j] = buffer[i];
				j++;
				i++;
			}
			// Check if the remaining is the end of the request.
			if (_remaining_size >= 5) {
				if (_remaining[0] == '\r' && _remaining[1] == '\n' && _remaining[2] == '0' && _remaining[3] == '\r' && _remaining[4] == '\n') {
					_end_of_request = true;
					return ;
				}
			}
		}
	}
	// If the request is not chunked.
	else {
		if (bytesread - i)	{
			_file.write(buffer + i, bytesread - i);
			_file.flush();
			_size_read += bytesread - i;
		}
		// Check if the request is complete.
		if (get_size_read() == atol(get_headers()["Content-Length"].c_str())) {
			_end_of_request = true;
			return ;
		}
	}
}

void	Request::parse_request() {
	if (_request_headers.empty())
		return ;
	std::istringstream ss(_request_headers);
	std::string line;
	getline(ss, line);
	line = line.substr(0, line.length() - 1);
	_request_line.method = line.substr(0, line.find(' '));
	line.erase(0, line.find(' ') + 1);
	_request_line.uri = line.substr(0, line.find(' '));
	line.erase(0, line.find(' ') + 1);
	_request_line.version = line.substr(0, line.find(' '));
	while (getline(ss, line))
	{
		line = line.substr(0, line.length() - 1);
		_headers[line.substr(0, line.find(':'))] = line.substr(line.find(':') + 2);
	}
	_request_headers.clear();
}
