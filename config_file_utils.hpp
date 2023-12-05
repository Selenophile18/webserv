#pragma once

#include "server.hpp"

class webserver;

void	config_parser(webserver &w, const char *name);
void	listen_directive(std::string &, int, webserver &, int);
void	server_name_directive(const std::string &, int, webserver &, int);
void	error_page_directive(std::string &, size_t , webserver &, int);
void	client_max_body_size_directive(std::string &, size_t , webserver &, int);
void	location_directive(std::string &, size_t , webserver &, int);