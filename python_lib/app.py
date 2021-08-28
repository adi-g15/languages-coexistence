import lib
from flask import Flask, request, jsonify
import json
import base64
import hashlib

# __name__ is the name of the application's module or package... used by Flask to determine location of resources such as templates or static files
app = Flask(__name__)

# Expects object of flask.Request class
def parse_body(request):
    body = request.get_data( parse_form_data=True ) # http://stackoverflow.com/questions/36711930/ddg#36722667
    body_dict = json.loads(body)
    return body_dict

@app.route("/")
def namaste_world():
    return "<em>Namaste World!</em>"

"""
A REST API responding with JSON object is basically just return a JSON string... that's it

@request body - JSON - {message_bytes: "base64 encoded string"}
@return {signature,public_key}
"""
@app.route("/digital_signature", methods=['POST'])
def get_digital_signature():
    body = parse_body(request)

    # https://www.base64encoder.io/python/
    message_bytes = base64.b64decode( body["message_bytes"] )

    # Rust FFI: https://bheisler.github.io/post/calling-rust-in-python/
    message_hash = hashlib.sha512( message_bytes ).hexdigest()

    signature_bytes, public_key_bytes = lib.sign_bytes(
            bytes(message_hash)
            )

    return jsonify({
            "signature_bytes": base64.b64encode(signature_bytes),
            "public_key_bytes": base64.b64encode(public_key_bytes)
        })

@app.route("/verify_signature", methods=['POST'])
def verify_signature():
    body = parse_body(request)
    message_bytes = base64.b64decode( body["message_bytes"] )
    signature = base64.b64decode( body["signature_bytes"] )
    public_key = base64.b64decode( body["public_key_bytes"] )

    message_hash = hashlib.sha512( message_bytes ).hexdigest()

    # Returning status codes: http://stackoverflow.com/questions/7824101/ddg#7824605
    if lib.verify_signer(signature, bytes(message_hash), public_key) == True:
        return "{\"verified\": \"true\"}", 202
    else:
        return "", 400

