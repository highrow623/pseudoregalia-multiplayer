#pragma once

#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/bind.hpp>

#include "Settings.hpp"

namespace UdpSocket
{
    using boost::asio::ip::udp;

    // A simple wrapper around boost udp sockets with a similar interface to wswrap
    template<size_t SEND, size_t RECV>
    class UdpSocket
    {
    public:
        // this will still be fired if the message doesn't fit inside the buffer, so use the len param to handle that
        // before reading from the buffer
        typedef std::function<void(const boost::array<uint8_t, RECV>&, size_t)> on_recv_handler;
        typedef std::function<void(const std::string&)> on_err_handler;

        UdpSocket(const std::string& address, const std::string& port, on_recv_handler on_recv, on_err_handler on_err)
            : _socket(_io_service), _resolver(_io_service), _endpoint(*_resolver.resolve({ udp::v4(), address, port }))
            , _on_recv(on_recv), _on_err(on_err)
        {
            _socket.open(udp::v4());
        }

        void Send(const boost::array<uint8_t, SEND>& buf, size_t len = SEND)
        {
            auto shared_buf = std::make_shared<boost::array<uint8_t, SEND>>(buf);
            // the buffer doesn't take ownership of the shared_ptr, so we pass it to HandleSend so it stays alive until
            // the callback fires
            _socket.async_send_to(boost::asio::buffer(*shared_buf, len), _endpoint,
                boost::bind(&UdpSocket::HandleSend, this, shared_buf, boost::asio::placeholders::error));
        }

        void Poll()
        {
            _io_service.poll();
            // TODO is the reset necessary?
            _io_service.reset();
        }

    private:
        boost::asio::io_service _io_service;
        udp::socket _socket;
        udp::resolver _resolver;
        udp::endpoint _endpoint;
        udp::endpoint _sender_endpoint;
        boost::array<uint8_t, RECV> _recv_buf{};
        bool _started_receive = false;

        on_recv_handler _on_recv;
        on_err_handler _on_err;

        void StartReceive()
        {
            _socket.async_receive_from(boost::asio::buffer(_recv_buf), _sender_endpoint,
                boost::bind(&UdpSocket::HandleReceive, this, boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
        }

        void HandleReceive(const boost::system::error_code& error, std::size_t len)
        {
            if (!error || error == boost::asio::error::message_size)
            {
                _on_recv(_recv_buf, len);
            }
            else
            {
                std::string message = error.message();
                _on_err("recv: " + message);
            }
            StartReceive();
        }

        void HandleSend(std::shared_ptr<boost::array<uint8_t, SEND>>, const boost::system::error_code& error)
        {
            if (error)
            {
                std::string message = error.message();
                _on_err("send: " + message);
                return;
            }

            if (!_started_receive)
            {
                StartReceive();
                _started_receive = true;
            }

        }
    };
} // namespace UdpSocket
