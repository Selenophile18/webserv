/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   response.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aharrass <aharrass@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/12/14 15:29:02 by aharrass          #+#    #+#             */
/*   Updated: 2023/12/22 21:56:25 by aharrass         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "method_utils.hpp"
#include "../Server.hpp"

#define DIREC 0
#define FILE 1
#define NOT_FOUND -1

class   Response    {
    private:
        int _status_code;
        std::string _status_line;
        std::string _response_body;
        std::string _uri;
        Server  _serv;
        const location *_location;

        Response();

    public:
        Response(int status_code, std::string uri, Server serv);
        ~Response();

        void    pars_uri();
        void match_uri();
        // void get();
        // void setStatusLine();

        const std::string &get_uri() const;
};