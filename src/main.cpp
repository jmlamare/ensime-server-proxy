#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <boost/bind.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>

using boost::asio::ip::tcp;

typedef boost::shared_ptr<tcp::socket> socket_ptr;

const int max_length = 1024;

template <class Ostream, class Arg> class osmanip {
  public:
    typedef Ostream ostream_type;
    typedef Arg argument_type;
    osmanip(Ostream& (*pf)(Ostream&, Arg), Arg arg) : pf_(pf) , arg_(arg) { ; }

  protected:
    Ostream&     (*pf_)(Ostream&, Arg);
    Arg          arg_;

  template <class OstreamBis, class ArgBis> 
  friend OstreamBis& operator<< (OstreamBis& ostr, const osmanip<OstreamBis, ArgBis>& manip);

};

template <class Ostream, class Arg> Ostream&  operator<< (Ostream& ostr, const osmanip<Ostream, Arg>& manip) {
   (*manip.pf_)(ostr, manip.arg_);
   return ostr;
}

template <class charT> class Tag : public osmanip<std::basic_ostream<charT>, std::basic_string<charT> > 
{
public:
  Tag(std::basic_string<charT> a = "") : 
  osmanip<std::basic_ostream<charT>, std::basic_string<charT> >(fct_, a) { }
  
  osmanip<std::basic_ostream<charT>, std::basic_string<charT> >& operator() (std::basic_string<charT> a) {
    this->arg_ = a;
    return *this;
  }
  
private:
  static std::basic_ostream<charT>& fct_ (std::basic_ostream<charT>& str, std::basic_string<charT> a) { 
    return str << a;
  }
};



bool check(boost::system::error_code &error, size_t expected_len, size_t actual_len, const char* step) {
  // std::cerr << "step " << step << std::endl;
  if( actual_len < expected_len ) {
    std::cerr << "actual_len=" << actual_len << ";expected_len=" << expected_len << std::endl;
    return true;
  } else if (error == boost::asio::error::eof) {
    std::cerr << "EOF" << std::endl;
    return true; // Connection closed cleanly by peer.
  } else if (error) {
    throw boost::system::system_error(error); // Some other error.
  } else {
    return false;
  }
}

void session(socket_ptr server_sock, socket_ptr client_sock)
{
  try
  {
    for (;;)
    {
      boost::system::error_code error;
      char header[7];
      header[6]=0;
      
      size_t length = server_sock->read_some(boost::asio::buffer(header, 6), error);
      if( check(error, 6, length, "read_req_hdr") ) break;
      
      size_t request_len = (size_t)std::strtol(header, NULL, 16);
      
      char request[request_len  + 1];
      length = server_sock->read_some(boost::asio::buffer(request, request_len), error);
      if( check(error, request_len, length, "read_req_txt") ) break;
      
      length = boost::asio::write(*client_sock, boost::asio::buffer(header, 6));
      if( check(error, 6, length, "write_req_hdr") ) break;
      length = boost::asio::write(*client_sock, boost::asio::buffer(request, request_len));
      if( check(error, request_len, length, "write_req_txt") ) break;
      
      request[request_len] = 0;
      std::cerr << request << std::endl;
      
      length = boost::asio::read(*client_sock, boost::asio::buffer(header, 6));
      if( check(error, 6, length, "read_res_hdr") ) break;
      
      size_t response_len = (size_t)std::strtol(header, NULL, 16);
      char response[response_len + 1];
      length = boost::asio::read(*client_sock, boost::asio::buffer(response, response_len));
      if( check(error, response_len, length, "read_res_txt") ) break;
      
      boost::asio::write(*server_sock, boost::asio::buffer(header, 6));
      if( check(error, 6, length, "write_res_hdr") ) break;
      boost::asio::write(*server_sock, boost::asio::buffer(response, response_len));
      if( check(error, response_len, length, "write_res_txt") ) break;
      
      response[response_len] = 0;
      std::cout << response << std::endl << "######" << std::endl;
    }
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception in thread: " << e.what() << std::endl;
  }
}


int main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      std::cerr << "Usage: " << argv[0] << " path " << std::endl;
      return 1;
    }
    
    int client_port, server_port;
    {
      std::ostringstream port_path;
      port_path << argv[1] << "/.ensime_cache/port";
      
      std::fstream port_file(port_path.str(), std::fstream::in | std::fstream::out);
      port_file >> client_port;
      server_port = client_port + 100;
      port_file.seekg(0, std::ios_base::beg);
      port_file << server_port; // server_endpoint.port();
      port_file.close();
    }
    
    
    boost::asio::io_service io_service;
    tcp::endpoint server_endpoint = tcp::endpoint(tcp::v4(), server_port);
    tcp::acceptor a(io_service, server_endpoint);
    std::cout << server_endpoint << std::endl;
    

    
    for (;;)
    {
      socket_ptr server_sock(new tcp::socket(io_service));
      a.accept(*server_sock);
      
      std::ostringstream client_port_stream;
      client_port_stream << client_port;
      
      tcp::resolver resolver(io_service);
      tcp::resolver::query query(tcp::v4(), "localhost", client_port_stream.str());
      tcp::resolver::iterator iterator = resolver.resolve(query);

      socket_ptr client_sock(new tcp::socket(io_service));
      boost::asio::connect(*client_sock, iterator);
      
      boost::thread(session, std::move(server_sock), std::move(client_sock)).detach();
    }
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}

