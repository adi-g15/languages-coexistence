#include <chrono>
#include <filesystem>
#include <iostream>
#include <pybind11/embed.h>
#include <thread>
#include <zmq.hpp>

#include "actions.hpp"
#include "payload.pb.h"

namespace py = pybind11;
namespace fs = std::filesystem;

using namespace std::chrono_literals;
using std::string;

string action_to_string(const AppliedAction action) {
	switch (action)
	{
		case AppliedAction::ENCODE:
			return "AppliedAction::ENCODE";
		case AppliedAction::HASH:
			return "AppliedAction::HASH";
		case AppliedAction::ENCRYPT:
			return "AppliedAction::ENCRYPT";
		case AppliedAction::SIGN:
			return "AppliedAction::SIGN";
		case AppliedAction::CERTIFICATE:
			return "AppliedAction::CERTIFICATE";
		default:
			return "";
	}
}

auto print_payload(const Payload& payload) {
	using std::cout, std::endl;

	cout << "{\n"
		 << "\taction  : \"" << action_to_string( payload.action() ) << "\"\n"
		 << "\tmessage : \"" << payload.original_message() << "\"\n"
		 << "\tpayload : \"" << payload.payload_str() << "\"\n"
		 << "\tmetadata: {";

	auto metadata = payload.metadata();
	if( metadata.empty() )	cout << "}";
	else cout << '\n';

	for(const auto &p : metadata) {
		cout << "\t\t" << p.first << ": \"" << p.second << "\",\n";
	}

	if( !metadata.empty() )	cout << "\t}";

	cout << "\n}\n";
}

auto process_request( zmq::message_t& request ) {
	auto payload = Payload();
	payload.ParseFromString( request.to_string() );

	std::cout << "Received payload is: \n";
	print_payload(payload);

	switch( payload.action() ) {
		case AppliedAction::ENCODE:
			std::cout << "Decoded string is:\n\"" << message::decode( payload.payload_str() ) << '\"' << std::endl;
			break;
		case AppliedAction::HASH:
			std::cout << "Received hash is: \"" << payload.payload_str() << '\"' << std::endl;
			break;
		case AppliedAction::ENCRYPT: {	// A block is needed in switch-case when initialising creating objects, such as strings vectors
			auto const& cipher = payload.payload_str();

			auto metadata = payload.metadata();
			auto key = metadata["key"];
			auto iv = metadata["iv"];

			auto plaintext_bytes = message::decrypt_bytes(
				util::hex_string_to_bytes(cipher),
				util::hex_string_to_bytes(key),
				util::hex_string_to_bytes(iv)
			);

			auto plaintext = std::string(
				reinterpret_cast<char*>(plaintext_bytes.data()),
				reinterpret_cast<char*>(plaintext_bytes.data()) + plaintext_bytes.size()
			);

			plaintext.push_back('\0');

			std::cout << "Decrypted content is:\n\""
					  << plaintext << "\"\n";
			break;
		}
		case AppliedAction::SIGN: {
			auto signed_bytes = util::hex_string_to_bytes( payload.payload_str() );
			auto publickey_bytes = util::hex_string_to_bytes( payload.metadata().at("public_key") );

			auto scoped_interpreter = py::scoped_interpreter{};

			                auto python_module_path = "lib";
	                auto original_dir = std::string(".");   // in case we change working directory, we will revert back with this

	                /* Depending on the current working directory,
	                 * the file may be present in the root or a directory below or inside a python_lib directory...
	                 * the simplest use case is running ./build/server instead of ./server with working directory as build/*/
	                if ( ! fs::exists("lib.py") ) {
	                        if ( fs::exists("python_lib/lib.py") ) {
	                                python_module_path = "python_lib.lib";
        	                } else if ( fs::exists("../python_lib/lib.py") ) {
                	                std::cerr << "[WARNING] python_lib/lib.rs found in parent directory...\n"
                        	                     "Changing the working directory to parent directory for importing the python lib" << std::endl;

                                	original_dir = fs::current_path();
                                	fs::current_path("..");
                                	python_module_path = "python_lib.lib";
                        	} else {
                        	        std::cerr << "[WARNING] lib.py or python_lib/lib.py not found !\n"
                        	                     "Next import statement MAY FAIL... !\n"
                         		             "...Bhagwan ka naam lekar aage badh rhe hai !" <<  std::endl;
                        	}
                	}

			auto signed_bytes_str = std::string( (char*)signed_bytes.data(), (char*)signed_bytes.data() + signed_bytes.size() );
			auto publickey_bytes_str = std::string( (char*)publickey_bytes.data(), (char*)publickey_bytes.data() + publickey_bytes.size() );

			auto python_lib = py::module::import( python_module_path );
			std::cout << std::endl << payload.original_message() << std::endl;
			auto pyobject = python_lib.attr("verify_signer")(
				py::bytes(signed_bytes_str),
				py::bytes(payload.original_message()),
				py::bytes(publickey_bytes_str)
				);

			bool verified = pyobject.cast<bool>();

			if ( verified ) {
				std::cout << "The message was signed by the provided key... Verified" << std::endl;
			} else {
				std::cerr << "VERIFICATION FAILED: Message was signed by someone else !!" << std::endl;
			}

			fs::current_path(original_dir);
			break;
		}
		case AppliedAction::CERTIFICATE: {
			break;
		}
		default:
			std::cerr << "Action not known !\n";
			std::cerr << request.to_string();
			return 1;
	}

	return 0;
}

int main () {
	auto context = zmq::context_t{1};	// init the zmq context with single IO thread
	auto socket = zmq::socket_t{context, zmq::socket_type::rep};	// 'rep' (reply) type socket

	socket.bind("tcp://*:15035");
	std::cout << "Bind socket to tcp://*:15035 succeeded...\n";

	while(true) {
		auto request = zmq::message_t{};

		auto _ret = socket.recv(request, zmq::recv_flags::none);
		process_request(request);

		socket.send(zmq::buffer(std::string("Received OK (200)")), zmq::send_flags::none);
	}

	return 0;
}

