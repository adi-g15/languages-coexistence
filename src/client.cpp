#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <argparse.hpp>
#include <cppcodec/base64_rfc4648.hpp>
#include <fmt/core.h>
#include <pybind11/embed.h>
#include <zmq.hpp>

#include "actions.hpp"
#include "grpc.pb.h"
#include "payload.pb.h"
#include "rust_cxx_interop.h"

extern "C" {
	#include <cert.h>
}

/*Programming with libcurlpp - https://raw.githubusercontent.com/jpbarrette/curlpp/master/doc/guide.pdf*/

namespace py = pybind11;
namespace fs = std::filesystem;

constexpr const char* PYTHON_SERVER_FLASK = "http://localhost:5000/";
constexpr const char* CPP_SERVER_ZMQ = "tcp://localhost:15035";
constexpr const char* JAVASCRIPT_SERVER_GRPC = "rpc://localhost:3000";

using std::string;

string to_lower_case(string&& str) {
	string lower(str);
	std::for_each(lower.begin(), lower.end(), [](char& c){ c = tolower(c); });

	return lower;
}

string encode_payload( const string &action, const string& msg ) {
	auto payload = Payload();
	payload.set_original_message(msg);
	
	if ( action == "encode" ) {
		payload.set_action(AppliedAction::ENCODE);
		
		auto str = message::encode(msg);
		payload.set_payload_str(str);

	} else if ( action == "hash" ) {
		payload.set_action(AppliedAction::HASH);

		auto str = message::hash(msg);
		payload.set_payload_str(str);
	} else if ( action == "encrypt" ) {
		payload.set_action(AppliedAction::ENCRYPT_ASYMMETRIC);

		py::scoped_interpreter scoped_interpreter{};

		auto python_module_path = "lib";
		auto original_dir = std::string(".");	// in case we change working directory, we will revert back with this

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

		auto python_lib = py::module::import(python_module_path);
		auto pyobject = python_lib.attr("sign_bytes")(py::bytes(msg));

		// Intentionally not handling exceptions... to keep it simple plus it's a client will just fail, and will neverthless run each time again
		auto pair = pyobject.cast<py::tuple>();
		auto signed_bytes = pair[0].cast<py::bytes>();	// bytes; format used when signing: utf-8
		auto public_key = pair[1].cast<py::bytes>();	// bytes; encoding: DER, format: SubjectPublicKeyInfo

		auto cstr_signed_bytes = signed_bytes.cast<std::string>();
		auto cstr_publickey_bytes = public_key.cast<std::string>();

		auto hexstring_signed_bytes = util::bytes_to_hex_string(
			std::vector<uint8_t>( (uint8_t*)cstr_signed_bytes.data(), (uint8_t*)cstr_signed_bytes.data() + cstr_signed_bytes.size() )
			);
		auto hexstring_publickey_bytes = util::bytes_to_hex_string(
			std::vector<uint8_t>( (uint8_t*)cstr_publickey_bytes.data(), (uint8_t*)cstr_publickey_bytes.data() + cstr_publickey_bytes.size() )
			);

		payload.set_payload_str(hexstring_signed_bytes);
		payload.mutable_metadata()->insert({"public_key", hexstring_publickey_bytes});

		fs::current_path(original_dir);
	} else if ( action == "encrypt_symmetric" ) {
		payload.set_action(AppliedAction::ENCRYPT_SYMMETRIC);

		auto encrypted_bytes = message::encrypt_bytes({
			reinterpret_cast<const uint8_t*>(msg.data()),
			reinterpret_cast<const uint8_t*>(msg.data()) + msg.size()
		});
		payload.set_payload_str( util::bytes_to_hex_string( encrypted_bytes.message ));
		auto metadata = payload.mutable_metadata();
		metadata->insert({
			{"key", util::bytes_to_hex_string(encrypted_bytes.key )},
			{"iv", util::bytes_to_hex_string(encrypted_bytes.IV) }
		});
	} else if ( action == "sign" ) {
		using base64 = cppcodec::base64_rfc4648;
		payload.set_action(AppliedAction::SIGN);

		auto request_body = fmt::format(
			"{\"message_bytes\":\"{}\"",
			base64::encode(
				std::vector<uint8_t>(
					reinterpret_cast<const uint8_t*>(msg.data()),
					reinterpret_cast<const uint8_t*>(msg.data()) + msg.size()
				)
			)
		);

		rust_ffi::post_request(
			rust::String(std::string(PYTHON_SERVER_FLASK) + "digital_signature"),
			rust::String(request_body)
			);
		// TODO: Use API call to get Digital Signature from python server
	} else if ( action == "get_certificate" ) {
		payload.set_action(AppliedAction::CERTIFICATE);

		/*Try with gRPC, if request times out or fails, try calling the C function*/
		// grpc::CreateChannel(JAVASCRIPT_SERVER_GRPC, grpc::InsecureChannelCredentials());

		auto cert = get_certificate(/*msg.data()*/);

	} else throw std::runtime_error("No Such Action !");

	return payload.SerializeAsString();
}

int main (int argc, const char *argv[]) {
	/*Create a simple arg parser to take two args: action, message*/
	argparse::ArgumentParser program("client");

	program.add_argument("action")
		.help("Chose action among:\n\tencode,\n\thash,\n\tsign,\n\tencrypt (default assymmetric),\n\tencrypt_symmetric,\n\tget_certificate");

	program.add_argument("message")
		.help("The message data (a string, can pipe a file)");

	try {
		program.parse_args(argc, argv);
	} catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
		std::clog << program;
		exit(0);
	}

	auto action = to_lower_case( program.get<string>("action") );
	auto message = program.get<string>("message");

	/*Now, we create the zmq context, and send the message*/
	auto context = zmq::context_t{1};
	/*This is a req (request) socket*/
	auto socket = zmq::socket_t{context, zmq::socket_type::req};

	socket.connect( CPP_SERVER_ZMQ );

	auto payload = encode_payload(action, message);

	std::clog << "Sending payload..." << std::endl;
	socket.send(zmq::buffer(payload), zmq::send_flags::none);
}
