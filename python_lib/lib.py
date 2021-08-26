from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives import asymmetric, serialization
from cryptography.hazmat.primitives.asymmetric import padding
from cryptography.exceptions import InvalidSignature

# https://cryptography.io/en/latest/hazmat/primitives/asymmetric/rsa/#signing 
def sign_bytes(message_str):
    private_key = asymmetric.rsa.generate_private_key(
                public_exponent=65537,
                key_size=4096       # 1024 and below are considered breakable now :(
            )

    signed_bytes = private_key.sign(
            bytes(message_str, "utf-8"),
            padding.PSS(
                    mgf=padding.MGF1(hashes.SHA512()),
                    salt_length=padding.PSS.MAX_LENGTH
                ),
                hashes.SHA512()
            )

    public_key_bytes = private_key.public_key().public_bytes(encoding=serialization.Encoding.PEM, format=serialization.PublicFormat.SubjectPublicKeyInfo)

    return (signed_bytes, public_key_bytes)

def verify_signer(signature, message, publickey_bytes):
    public_key = serialization.load_pem_public_key(publickey_bytes)

    try:
        public_key.verify(
                signature,
                message,
                padding.PSS( mgf=padding.MGF1(hashes.SHA512()), salt_length=padding.PSS.MAX_LENGTH ),
                hashes.SHA512()
                )
        return True
    except InvalidSignature:
        return False

