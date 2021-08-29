const PROTO_PATH = __dirname + "/../proto/grpc.proto";
const grpc = require("@grpc/grpc-js");
const ProtoLoader = require("@grpc/proto-loader");

/*Why these options only?... As stated by the grpc docs, this is to be similar to ProtoLoader.load behaviour*/
const package_definition = ProtoLoader.loadSync(
	PROTO_PATH,
	{
		keepCase: true,
		longs: String,
		enums: String,
		defaults: true,
		oneofs: true
	}
)
const package_descriptor = grpc.loadPackageDefinition( package_definition );

/*Now we start the actual working... this `package_descriptor` has the complete package heirarchy*/
const grpc_service = package_descriptor.grpc_service;
console.debug("gRPC service -> ", grpc_service);

function get_certificate_impl(certificate_payload) {
	// certificate_payload is object of package_descriptor.CertificatePayload
	let certificate = package_descriptor.Certificate;
	console.log(certificate_payload);

	return certificate;
}

const server = new grpc.Server();

server.addService( grpc_service.service, {
	getCertificate: (call, callback) => callback(null, get_certificate_impl(call.request))	// we send the request body
});

server.bindAsync("0.0.0.0:5000", grpc.ServerCredentials.createInsecure(), () => {
	console.log("Listening on 0.0.0.0:5000 ...");
	server.start();
});
