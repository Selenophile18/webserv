#include "Cgi.hpp"

std::string generateUUID() {
    // Seed for the random number generator
    std::srand(static_cast<unsigned>(time(0)));

    const char hex_chars[] = "0123456789abcdef";

    std::string uuid;

    // Generate 32 random hexadecimal characters
    for (int i = 0; i < 32; ++i) {
        uuid += hex_chars[std::rand() % 16];
    }

    // Insert hyphens at appropriate positions
    uuid.insert(8, 1, '-');
    uuid.insert(13, 1, '-');
    uuid.insert(18, 1, '-');
    uuid.insert(23, 1, '-');

    return uuid;
}

Cgi::Cgi(): _outfile(-1){}

Cgi::Cgi(std::map<std::string, std::string> headers, Response *response)
:_response(response)   {
    this->_extension_map = _response->get_location().cgi;
	this->is_complete = false;
	this->is_timeout = false;
	this->is_running = false;
	setup_env(headers);
}

Cgi::~Cgi() {
}

int Cgi::get_outfile() const {
    return this->_outfile;
}

void	Cgi::setup_env(std::map<std::string, std::string> headers)
{
	std::string	temp;

	this->_headers["AUTH_TYPE"] = "";
    this->_headers["CONTENT_LENGTH"] = headers["Content-Length"];
    this->_headers["CONTENT_TYPE"] = headers["Content-Type"];
    this->_headers["PATH_INFO"] = this->_response->get_old_uri();
    this->_headers["PATH_TRANSLATED"] = this->_response->get_uri();
    this->_headers["QUERY_STRING"] = this->_response->get_query();
    this->_headers["REMOTE_HOST"] = headers["Host"];
    this->_headers["REMOTE_USER"] = "";
    // this->_headers["HTTP_COOKIE"] = _client.req.header_map["Cookie"];;
    this->_headers["REMOTE_IDENT"] = "";
    this->_headers["REQUEST_METHOD"] = this->_response->get_method();
    this->_headers["GATEWAY_INTERFACE"] = "CGI/1.1";
    // this->_headers["SCRIPT_NAME"] = th.req.current_location.cgi_path;
    // this->_headers["SCRIPT_FILENAME"] = _client.req.current_location.root + "/" + _client.req.current_location.cgi_path;
    this->_headers["SERVER_NAME"] = "";
    this->_headers["SERVER_PROTOCOL"] = "HTTP/1.1";
    std::stringstream ss;
    ss << _response->get_client()->get_server().get_port();
    std::string tmp;
    ss >> tmp;
    this->_headers["SERVER_PORT"] = tmp;
    this->_headers["SERVER_SOFTWARE"] = "webserv/1.0";
    this->_headers["REDIRECT_STATUS"] = "200";
    this->_headers["REQUEST_SCHEME"] = "http";
    this->_headers["HTTPS"] = "off";
    this->_headers["SERVER_SIGNATURE"] = "webserv/1.0";
	_env = new char*[_headers.size() + 1];
	int i = 0;
	std::map<std::string, std::string>::iterator it;
	for (it = this->_headers.begin(); it != this->_headers.end(); it++, i++)
	{
		temp = (it->first) + "=" + (it->second);
		_env[i] = strdup(temp.c_str());
	}
    _env[i] = 0;
}

void	Cgi::run(const std::string &bin)
{
	const char 	*av[3];

	dup2(this->_outfile, 1);
    close(this->_outfile);
    av[0] = bin.c_str();
    av[1] = this->_response->get_uri().c_str();
    av[2] = 0;
    // for (int i = 0; i != 2; i++)    {
    //     std::cerr << av[i] << std::endl;
    // }
    execve(bin.c_str(), (char *const *)av, this->_env);
    std::cerr << "not executed" << std::endl;
    exit(-1);
}

void	Cgi::php_setup()
{
	std::cout << "php setup" << std::endl;
	std::map<std::string, std::string>::iterator    it = this->_extension_map.find("php");
	if (it == this->_extension_map.end())
	{
		throw std::runtime_error("No extension!");
		return ;
	}
	else
	{
		int check = access((it->second).c_str(), F_OK && X_OK);
		if (check)
		{
			this->_response->set_status_code(500);
			return ;
		}
        std::string	file_name = generateUUID();
        file_name = ".cache/" + file_name;
        this->_response->set_file_name(file_name);
        umask(0);
        this->_outfile = open(file_name.c_str(), O_CREAT | O_RDWR, 0666);
        if (_outfile == -1) {
            std::cerr << "file not open" << std::endl;
            this->_response->set_status_code(403);
            return;
        }
		this->is_running = true;
		this->_start = clock();
		this->_pid = fork();
		if (!this->_pid)
			run("/usr/bin/php");
        else    {
            int status;
            close(this->_outfile);
            waitpid(this->_pid, &status, 0);
            if (WIFEXITED(status))   {
                if (WEXITSTATUS(status) != 0) {
                    std::cout << WEXITSTATUS(status) << std::endl;
                    this->_response->set_status_code(500);
                }
            }
        }
	}
}

void	Cgi::py_setup()
{
	std::cout << "py setup" << std::endl;
	std::map<std::string, std::string>::iterator    it = this->_extension_map.find("py");
	if (it == this->_extension_map.end())
	{
		throw std::runtime_error("No extension!");
		return ;
	}
	else
	{
		int check = access((it->second).c_str(), F_OK && X_OK);
		if (check)
		{
			this->_response->set_status_code(500);
			return ;
		}
        std::string	file_name = generateUUID();
        file_name = ".cache/" + file_name;
        _response->set_file_name(file_name);
        umask(0);
        this->_outfile = open(file_name.c_str(), O_CREAT | O_RDWR, 0666);
        if (_outfile == -1) {
            std::cerr << "file not open" << std::endl;
            _response->set_status_code(403);
            return;
        }
		this->is_running = true;
		this->_start = clock();
		this->_pid = fork();
		if (!this->_pid)
			run("/usr/bin/python3");
        else    {
            int status;
            waitpid(this->_pid, &status, WNOHANG);
            if (WIFEXITED(status))   {
                if (WEXITSTATUS(status) != 0) {
                    std::cout << WEXITSTATUS(status) << std::endl;
                    _response->set_status_code(500);
                }
            }
        }
	}
}