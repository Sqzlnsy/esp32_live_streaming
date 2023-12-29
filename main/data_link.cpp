/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include "asio.hpp"
#include <string>
#include <iostream>
extern "C" {
#include "spsc_rb.h"
}

#define CONFIG_EXAMPLE_PORT "2022"

using asio::ip::tcp;

class session
    : public std::enable_shared_from_this<session> {
public:
    session(tcp::socket socket, RingBuffer *ringBuffer)
        : socket_(std::move(socket)), ringBuffer_(ringBuffer)
    {
    }

    void start()
    {
        std::cout << "Session created Successfully" << std::endl;
        do_read();
    }

private:
    void do_read()
    {
        auto self(shared_from_this());
        socket_.async_read_some(asio::buffer(data_, max_length),
        [this, self](std::error_code ec, std::size_t length) {
            if (!ec) {
                data_[length] = 0;
                writeToBuffer(ringBuffer_, data_, length); // Write to ring buffer
                do_write(length);
            }
        });
    }

    void do_write(std::size_t length)
    {
        auto self(shared_from_this());
        asio::async_write(socket_, asio::buffer(data_, length),
        [this, self](std::error_code ec, std::size_t length) {
            if (!ec) {
                do_read();
            }
        });
    }

    tcp::socket socket_;
    RingBuffer *ringBuffer_;
    enum { max_length = 10240 };
    char data_[max_length];
};

class server {
public:
    server(asio::io_context &io_context, short port, RingBuffer *ringBuffer)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)), ringBuffer_(ringBuffer)
    {
        do_accept();
    }

private:
    void do_accept()
    {
        acceptor_.async_accept(
        [this](std::error_code ec, tcp::socket socket) {
            if (!ec) {
                std::make_shared<session>(std::move(socket), ringBuffer_)->start();
            }

            do_accept();
        });
    }

    tcp::acceptor acceptor_;
    RingBuffer *ringBuffer_; 
};


extern "C" void data_link_task(void *pvParameters)
{
    RingBuffer *ringBuffer = (RingBuffer *)pvParameters;

    asio::io_context io_context;

    server s(io_context, std::atoi(CONFIG_EXAMPLE_PORT), ringBuffer);

    std::cout << "ASIO engine is up and running" << std::endl;

    io_context.run();
}